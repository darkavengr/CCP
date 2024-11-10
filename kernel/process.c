/*  	CCP Version 0.0.1
	(C) Matthew Boote 2020-2023

	This file is part of CCP.

	CCP is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	CCP is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with CCP.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <elf.h>
#include <stdint.h>
#include <stddef.h>
#include "kernelhigh.h"
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "memorymanager.h"
#include "process.h"
#include "signal.h"
#include "bootinfo.h"
#include "debug.h"

size_t last_error_no_process=0;
PROCESS *processes=NULL;
PROCESS *processes_end=NULL;
PROCESS *currentprocess=NULL;
EXECUTABLEFORMAT *executableformats=NULL;
size_t highest_pid_used=0;
MUTEX process_mutex;
size_t signalno;
size_t thisprocess;
char *saveenv=NULL;
size_t tickcount;
size_t entrypoint;
char *tempfilename[MAX_PATH];
char *tempargs[MAX_PATH];
size_t *stackinit;

/*
* Execute program
*
* In: filename	Filename of file to execute
      argsx	Arguments
      flags	Flags
*
* Returns -1 on error, doesn't return on success if it is a foreground process. Returns 0 on success if it is a background process.
*
*/

size_t exec(char *filename,char *argsx,size_t flags) {
size_t handle;
void *kernelstackbase;
PROCESS *next;
PROCESS *oldprocess;
size_t stackp;
PROCESS *lastprocess;
PSP *psp=NULL;
size_t *stackinit;
char *fullpath[MAX_PATH];

disablemultitasking(); 
disable_interrupts();

oldprocess=currentprocess;					/* save current process pointer */

/* add process to process list and find process id */

if(processes == NULL) {  					/* first process */
	processes=kernelalloc(sizeof(PROCESS));			/* add entry to beginning of list */

	if(processes == NULL) {					/* return if can't allocate */
		loadpagetable(getppid());
		freepages(highest_pid_used);
		enablemultitasking();	

		close(handle);
		return(-1);
	}

processes_end=processes;					/* save end of list */
lastprocess=processes_end;			/* save current end */

next=processes;
}
else								/* not first process */
{
	processes_end->next=kernelalloc(sizeof(PROCESS));	/* add entry to end of list */
	if(processes_end->next == NULL) {			/* return error if can't allocate */
		loadpagetable(getppid());
		freepages(highest_pid_used);
		enablemultitasking();	

		close(handle);
		setlasterror(NO_MEM);
		return(-1);
	}

	lastprocess=processes_end;			/* save current end */
	next=processes_end->next;		/* get a copy of end, so that the list is not affected if any errors occur */
}

getfullpath(filename,fullpath);		/* get full path to executable */

strncpy(tempfilename,fullpath,MAX_PATH);		/* save filename and arguments into variables that are not affected by any stack change */
strncpy(tempargs,argsx,MAX_PATH);			   

/* fill struct with process information */

strncpy(next->filename,fullpath,MAX_PATH);	/* process executable filename */
strncpy(next->args,argsx,MAX_PATH);		/* process arguments */

getcwd(next->currentdirectory);		/* current directory */
next->maxticks=DEFAULT_QUANTUM_COUNT;	/* maximum number of ticks for this process */
next->ticks=0;				/* start at 0 for number of ticks */
next->parentprocess=getpid();		/* parent process ID */
next->pid=highest_pid_used;		/* process ID */
next->errorhandler=NULL;		/* error handler, NULL for now */
next->signalhandler=NULL;		/* signal handle, NULL for now */
next->childprocessreturncode=0;		/* return code from child process */
next->flags=flags;			/* process flags */
next->lasterror=0;			/* last error */
next->next=NULL;

/* Create kernel stack for process */

next->kernelstacktop=kernelalloc(PROCESS_STACK_SIZE);	/* allocate stack */
if(next->kernelstacktop == NULL) {	/* return if unable to allocate */
	currentprocess=oldprocess;	/* restore current process pointer */

	loadpagetable(getpid());
	freepages(highest_pid_used);

	enablemultitasking();
	kernelfree(lastprocess->next);	/* remove process from list */

	lastprocess->next=NULL;		/* remove process */
	processes_end=lastprocess;

	close(handle);
	return(-1);
}

next->kernelstackbase=next->kernelstacktop;
next->kernelstacktop += PROCESS_STACK_SIZE;			/* top of kernel stack */
next->kernelstackpointer=next->kernelstacktop;			/* intial kernel stack address */

//DEBUG_PRINT_HEX(next->kernelstacktop);

/* Enviroment variables are inherited
* Part one of enviroment variables duplication
*
* copy the variables to a buffer in kernel memory where it is visible to both parent and child process
*/

saveenv=NULL;

if(currentprocess != NULL) {
	saveenv=kernelalloc(ENVIROMENT_SIZE);		/* allocate a buffer to save enviroment variables in */
	if(saveenv == NULL) {
		currentprocess=oldprocess;	/* restore current process pointer */

		loadpagetable(getpid());
		freepages(highest_pid_used);

		enablemultitasking();
		if(lastprocess->next != NULL) kernelfree(lastprocess->next);	/* remove process from list */

		lastprocess->next=NULL;		/* remove process */
		processes_end=lastprocess;

		close(handle);
		return(-1);
	}

	memcpy(saveenv,currentprocess->envptr,ENVIROMENT_SIZE);		/* copy enviroment variables */
}

page_init(highest_pid_used);				/* intialize page directory */	
loadpagetable(highest_pid_used);			/* load page table */

currentprocess=next;					/* use new process */

highest_pid_used++;

/* Part two of enviroment variables duplication */

/* allocate enviroment block at the highest user program address minus enviroment size */

currentprocess->envptr=alloc_int(ALLOC_NORMAL,getpid(),ENVIROMENT_SIZE,END_OF_LOWER_HALF-ENVIROMENT_SIZE);

if(saveenv != NULL) {
	memcpy(currentprocess->envptr,saveenv,ENVIROMENT_SIZE);		/* copy from saved enviroment buffer */
	kernelfree(saveenv);
}

/* allocate user mode stack below enviroment variables */

stackp=alloc_int(ALLOC_NORMAL,getpid(),PROCESS_STACK_SIZE,(END_OF_LOWER_HALF-PROCESS_STACK_SIZE-ENVIROMENT_SIZE));

if(stackp == NULL) {
	currentprocess=oldprocess;	/* restore current process pointer */
	kernelfree(next->kernelstacktop);	/* free kernel stack */

	if(lastprocess->next != NULL) kernelfree(lastprocess->next);	/* remove process from list */

	lastprocess->next=NULL;		/* remove process */
	processes_end=lastprocess;

	loadpagetable(getpid());
	freepages(highest_pid_used);
	enablemultitasking();	
	return(-1);
}

next->stackbase=stackp;					/* add user mode stack base and pointer to new process entry */
next->stackpointer=stackp+PROCESS_STACK_SIZE;

/* duplicate stdin, stdout and stderr */

if(getpid() != 0) {
	dup_internal(stdin,-1,getppid(),getpid());
	dup_internal(stdout,-1,getppid(),getpid());
	dup_internal(stderr,-1,getppid(),getpid());
}

processes_end=next;						/* save last process address */

enable_interrupts();

entrypoint=load_executable(tempfilename);			/* load executable */
disable_interrupts();

if(entrypoint == -1) {					/* can't load executable */
	kernelfree(next->kernelstacktop);	/* free kernel stack */

	if(lastprocess->next != NULL) kernelfree(lastprocess->next);	/* remove process from list */

	lastprocess->next=NULL;		/* remove process */
	processes_end=lastprocess;

	currentprocess=lastprocess;	/* restore current process */

	loadpagetable(getpid());
	enablemultitasking();
	return(-1);
}

initializekernelstack(currentprocess->kernelstacktop,entrypoint,currentprocess->kernelstacktop-PROCESS_STACK_SIZE); /* initializes kernel stack */

/* create psp */ 

ksnprintf(psp->commandline,"%s %s",MAX_PATH,next->filename,next->args);

psp->cmdlinesize=strlen(psp->commandline);

if((flags & PROCESS_FLAG_BACKGROUND)) {			/* run process in background */
	currentprocess=oldprocess;	/* restore current process pointer */			/* restore previous process */

	kernelfree(next->kernelstacktop);	/* free kernel stack */

	if(lastprocess->next != NULL) kernelfree(lastprocess->next);	/* remove process from list */

	lastprocess->next=NULL;		/* remove process */
	processes_end=lastprocess;

	loadpagetable(getpid());
	enablemultitasking();
	enable_interrupts();

	return;
}
else
{
	initializestack(currentprocess->stackpointer,PROCESS_STACK_SIZE);	/* intialize user mode stack */

	enablemultitasking();
	enable_interrupts();

	switch_to_usermode_and_call_process(entrypoint);		/* switch to user mode, enable interrupts, and call process */
}

return;
}

/*
* Terminate process
*
* In: process	Process ID
*
* Returns -1 on error doesn't return on success
*
*/

size_t kill(size_t process) {
PROCESS *last;
PROCESS *next;
PROCESS *nextprocess;
char *b;
size_t oldprocess;

disablemultitasking();

lock_mutex(&process_mutex);			/* lock mutex */

shut();						/* close open files for process */

oldprocess=getpid();				/* save current process ID */

next=processes;
last=next;
	
while(next != NULL) {

	if(next->pid == process) break;
	last=next;
	next=next->next;
}

if(next == NULL) {				/* return if process was not found */
	setlasterror(INVALID_PROCESS);

	unlock_mutex(&process_mutex);			/* unlock mutex */

	enablemultitasking();
	return(-1);
}

if(processes == NULL) {
	enable_interrupts();
	shutdown(0);
}

while((next->flags & PROCESS_BLOCKED) != 0) ;;		/* process is being waited on */

if(next == processes) {			/* start */
	processes=next->next;
}
else if(next->next == NULL) {		/* end */
	 last->next=NULL;
}
else						/* middle */
{
	 last->next=next->next;	
}

if(processes == NULL) {		/* no processes */
	kprintf_direct("kernel: No more processes running, system will shut down\n");
	shutdown(_SHUTDOWN);
}
else
{
	kernelfree(next);
	freepages(oldprocess);
}


unlock_mutex(&process_mutex);			/* unlock mutex */

if(process == oldprocess) {
	currentprocess->ticks=currentprocess->maxticks;	/* force task to end of timeslot */

	enablemultitasking();

	yield();			/* switch to next process if killing current process */

}

enablemultitasking();
return;
}

/*
* Exit from process
*
* In: val	Return value
*
* Returns -1 on error, doesn't return on success
*
*/

size_t exit(size_t val) {
	 kill(getpid());			/* kill current process */
	 return(val);	
}

/*
* Shutdown
*
* In: shutdown_status	Shutdown or restart
*
* Returns nothing
*
*/

void shutdown(size_t shutdown_status) { 
	kprintf_direct("Shutting down:\n");
	kprintf_direct("Sending SIGTERM signal to all processes...\n");

	sendsignal(-1,SIGTERM);		/*  send terminate signal to all processes */

	kprintf_direct("Waiting %d seconds for processes to terminate\n",SHUTDOWN_WAIT);
	kwait(SHUTDOWN_WAIT);
	
	if(processes != NULL) kprintf_direct("kernel: Processes did not terminate in time (too bad)\n");

	if(shutdown_status == _SHUTDOWN) {
		kprintf_direct("It is now safe to turn off your computer\n");

		disable_interrupts();
		while(1) halt();		/* loop */
	}
	else
	{
		restart();
	}
	
}
	
/*
* Wait for process to change state
*
* In: pid	Process ID
*
* Returns -1 on error or 0 on success
*
*/

size_t wait(size_t pid) {
PROCESS *next;
size_t oldflags;
size_t newflags;

lock_mutex(&process_mutex);			/* lock mutex */

next=processes;
		
while(next != NULL) {
	if(next->pid == pid) {			/* found process */

	 unlock_mutex(&process_mutex);			/* unlock mutex */

	 oldflags=next->flags;
	
	 while(next->flags == oldflags) ;;		/* wait for process to change state */

	 setlasterror(NO_ERROR);
	 return(0);
	}

	next=next->next;
}

setlasterror(INVALID_PROCESS);		/* invalid process */
return(-1);
}

/*
* Dispatcher
*
* In: argsix			\ 
      argfive			|
      argfour			| Arguments to pass to functions
      argthree			|
      argtwo		        /
      argsone			Function code
*
* Returns -1 on error or function return value on success
*
*/

size_t dispatchhandler(size_t ignored1,size_t ignored2,size_t ignored3,size_t ignored4,void *argsix,void *argfive,void *argfour,void *argthree,void *argtwo,size_t argone) {
char *b;
char x;
size_t count;
FILERECORD findbuf;
size_t highbyte;
size_t lowbyte;

argone=argone & 0xffff;				/* get 16 bits */

highbyte=(argone & 0xff00) >> 8;
lowbyte=(argone & 0xff);

switch(highbyte) {
	case 0:			/* exit */
		kill(getpid());
		break;

	case 9:			/* output string */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		kprintf_direct((char *) argfour);
		return;

	case 0x30:			/* get version */
		return(GetVersion());

	case 0x31:
		return(yield());
		
	case 0x39:			/* create directory */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(mkdir((char *) argfour));

	case 0x3a:			/* remove directory */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(rmdir((char *) argfour));

	case 0x3b:			/* change directory */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(chdir((char *) argfour)); 

	case 0x3c:			/* create */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(create((char *) argfour));
	 
	case 0x3e:			/* close */
		return(close((size_t) argtwo));  

  	 
	case 0x3f:			/* read from file or device */
		//if(argfour >= KERNEL_HIGH) {		/* invalid argument */
		//	setlasterror(INVALID_ADDRESS);
		//	return(-1);
		//}

		return(read((size_t)  argtwo,(char *) argfour,(size_t)  argthree));
	 
	case 0x40:			/* write to file or device */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(write((size_t)  argtwo,(char *) argfour,(size_t)  argthree));
	case 0x41:                     /* delete */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(unlink((char *) argfour));

	case 0x45:			/* duplicate file handle */
		return(dup(argtwo));

	case 0x46:			/* duplicate file handle and replace existing handle */
		return(dup2(argtwo,argthree));

	case 0x47:			/* get current directory */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(getcwd((char *) argfour));

	case 0x48:			/* allocate memory */
		return(alloc((size_t)  argtwo));

	case 0x49:			/* free memory */
		free(argfour);
		return;

	case 0x4a:			/* resize memory */
		return(realloc_user(argfour,(size_t) argtwo));

	case 0x4c:			/* terminate process */
		return(exit(lowbyte));

	case 0x4e:			/* find first file */
		if((argfour >= KERNEL_HIGH) || (argtwo >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(findfirst(argfour,argtwo));	
		
	case 0x4d:			/* get last error */
		return(getlasterror());

	case 0x4f:			/* find next file */
		if((argfour >= KERNEL_HIGH) || (argtwo >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(findnext(argfour,argtwo));

	case 0x50:			/* get process ID */
		return(getpid()); 
	
	case 0x56:			/* rename file or directory */
		if((argfive >= KERNEL_HIGH) || (argsix >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(rename((char *) argfive,(char *) argsix));

	case 0x65:			/* get process information */
		memcpy(argtwo,currentprocess,sizeof(PROCESS));
		break; 
	  
	case 0x8c:			/* set signal handler */
		signal(argfour);

	case 0x8d:			/* send signal */
		return(sendsignal((size_t) argtwo,argfour));

	case 0x8a:			/* wait for process to change state */
		return(wait((size_t)  argtwo));
	
	case 0x93:			/* create pipe */
		return(pipe());
}

switch(argone) {
	 
	case 0x3d01:		/* open file or device */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(open((char *) argfour,O_RDONLY));

	case 0x3d02:
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(open((char *) argfour,O_RDWR));

	case 0x4200:		/* set file position */
		return(seek((size_t)  argtwo,argfour,SEEK_SET));

	case 0x4201:		/* seek */
		return(seek((size_t)  argtwo,argfour,SEEK_CUR));

	case 0x4202:		/* seek */
		return(seek((size_t)  argtwo,argfour,SEEK_END));

	case 0x4300:			/* get file attributes */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		findfirst(argfour,&findbuf);

		return(findbuf.attribs);

	case 0x4301:			/* set file attributes */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(chmod(argfour,(size_t)  argtwo));

	case 0x4401:			/* send commands to device */
		return(ioctl((size_t) argtwo,(unsigned long) argthree,(void *) argsix));

	case 0x4b00:			/* create process and load executable */
		if((argfour >= KERNEL_HIGH) || (argtwo >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(exec(argfour,argtwo,FALSE));
	 
	case 0x4b01:
		if((argfour >= KERNEL_HIGH) || (argtwo >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(exec(argfour,argtwo,TRUE));

	case 0x4b03:				/* load module */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(load_kernel_module(argfour,argtwo));	 	

	case 0x5700:			/* get file time and date */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		findfirst(argfour,&findbuf);
		return;
	 
	case 0x5701:				/* set file time and date */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(touch((size_t) argtwo,(size_t) argfour,(size_t) argfive,(size_t) argsix));

	
	case 0x7000:				/* allocate DMA buffer */
		return(dma_alloc(argtwo));
	    	
	case 0x7002:			/* restart */
		shutdown(_RESET);
		break;

	case 0x7003:			/* shutdown */
		shutdown(_SHUTDOWN);
		break;

	case 0x7004:			/* get file position */
		return(tell(argtwo));
	   
	case 0x7009:			/* find first process */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(findfirstprocess((PROCESS *) argtwo));

	case 0x700A:			/* find next process */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(findnextprocess((PROCESS *) argfour,(PROCESS *) argtwo));

	case 0x700B:			/* add block device */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

	 	return(add_block_device(argfour));

	case 0x700C:			/* add char device */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(add_character_device(argfour));

	case 0x700D:			/* remove block device */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(remove_block_device(argfour));

	case 0x700E:			/* remove character device */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(remove_character_device(argfour));

	case 0x7018:				/* get file size */
		return(getfilesize(argtwo));

	case 0x7019:			/* terminate process */
		return(kill((size_t)  argtwo));

	case 0x7028:				/* get next free drive letter */
		return(allocatedrive());

	case 0x7029:				/* get character device */
		if((argfour >= KERNEL_HIGH) || (argtwo >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(findcharacterdevice(argtwo,argfour));

	case 0x702a:				/* get block device from drive number */
		if((argfour >= KERNEL_HIGH) || (argtwo >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(getblockdevice(argtwo,argfour));

	case 0x702b:				/* get block device from name */
		if((argfour >= KERNEL_HIGH) || (argtwo >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(getdevicebyname(argtwo,argfour));

	case 0x702d:				/* register filesystem */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(register_filesystem(argfour));

	case 0x702e:				/* lock mutex */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

	 	return(lock_mutex(argfour));

	case 0x702f:				/* unlock mutex */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(unlock_mutex(argfour));

	case 0x7030:				/* get enviroment address */
		return(getenv());

	case 0x7031:				/* unload module */
		return(unload_kernel_module(argfour));

	case 0x2524:				/* set critical error handler */
		if(argtwo >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(set_critical_error_handler(argtwo));

	default:
		setlasterror(INVALID_FUNCTION);
		return(-1);
	}
}

/*
* Get current directory
*
* In: char *dir	Buffer for directory name
*
* Returns -1 on error or 0 on success
*
*/

size_t getcwd(char *dir) {
char *buf[MAX_PATH];
char *b;
BLOCKDEVICE blockdevice;
BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;

/*
 * usually the directory is got from the current process struct
 * but if there are no processes running,the current directory
 * must be built up on the fly
 */

if(currentprocess == NULL) {			/* no processes, so get from boot drive */
	 b=dir;						/* create directory name */
	 *b++=(uint8_t) boot_info->drive+'A';
	 *b++=':';
	 *b++='\\';
	 *b++=0;

	 setlasterror(NO_ERROR);
	 return(NO_ERROR);
}

strncpy(dir,currentprocess->currentdirectory,MAX_PATH);

setlasterror(NO_ERROR);
return(NO_ERROR);
}

/*
* Set current directory
*
* In: char *dirname			Directory to change to    
*
* Returns -1 on error or function return value on success
*
*/

size_t chdir(char *dirname) {
char *fullpath[MAX_PATH];
char *savecwd[MAX_PATH];
FILERECORD chdir_file_record;

getfullpath(dirname,fullpath);

/* findfirst (and other other filesystem calls) use currentprocess->currentdirectory to get the
	  full path of a file, but this function checks that a new current directory is valid,
	  so it must copy the new current directory to currentprocess->currentdirectory to make sure that 
	  it will be used to validate it.

	  If it fails, the old path is restored
*/

strncpy(savecwd,currentprocess->currentdirectory,MAX_PATH);		/* save current directory */
memcpy(currentprocess->currentdirectory,fullpath,3);		/* get new path */

/* check if directory exists */

if(findfirst(fullpath,&chdir_file_record) == -1) {		/* path doesn't exist */
	strncpy(currentprocess->currentdirectory,savecwd,MAX_PATH);		/* restore old current directory */
	return(-1);		
}

if((chdir_file_record.flags & FILE_DIRECTORY) == 0) {		/* not directory */
	setlasterror(NOT_DIRECTORY);
	return(-1);
}

strncpy(currentprocess->currentdirectory,fullpath,MAX_PATH);	/* set directory */

setlasterror(NO_ERROR);  
return(NO_ERROR);				/* no error */
}

/*
* Get process ID
*
* In: Nothing
*
* Returns process ID
*
*/

size_t getpid(void) {
if(currentprocess == NULL) return(0);

return(currentprocess->pid);
}

/*
* Set last error
*
* In: err	Error number
*
* Returns nothing
*
*/

void setlasterror(size_t err) {
if(currentprocess == NULL) {
		last_error_no_process=err;
		return;
}

currentprocess->lasterror=err;
}

/*
* Get last error
*
* In: Nothing
*
* Returns last error number
*
*/

size_t getlasterror(void) {
	if(currentprocess == NULL) return(last_error_no_process);		/* if there are no processes, use temporary variable */

	return(currentprocess->lasterror);
}

/*
* Get process filename
*
* In: char *buf	Buffer for filename
*
* Returns nothing
*
*/

size_t getprocessfilename(char *buf) {
if(currentprocess == NULL) {			/* if there are no processes, return "<unknown>" */
	strncpy(buf,"<unknown>",MAX_PATH);
	return;
}

strncpy(buf,currentprocess->filename,MAX_PATH);		/* get filename */
}

/*
* Get process arguments
*
* In: char *buf	Buffer for arguments
*
* Returns nothing
*
*/

void getprocessargs(char *buf) {
if(currentprocess == NULL) return;		/* return if there are no processes */

strncpy(buf,currentprocess->args,MAX_PATH);
}

/*
* Get parent process ID
*
* In: nothing
*
* Returns parent process ID
*
*/

size_t getppid(void) {
if(currentprocess == NULL) return(0);			/* return 0 if there are no processes */

return(currentprocess->parentprocess);
}

/*
* Get process return code
*
* In: nothing
*
* Returns: process return code
*
*/

size_t getreturncode(void) {				/* return 0 if there are no processes */
if(currentprocess == NULL) return(0);

return(currentprocess->childprocessreturncode);
}

/*
* Increment process ticks
*
* In: nothing
*
* Returns: Nothing
*
*/

void increment_process_ticks(void) {
if(currentprocess == NULL) return;

currentprocess->ticks++;
}

/*
* Set signal handler
*
* In: void *handler	Handler function to call on signal receipt
*
* Returns 0
*
*/

void signal(void *handler) {
currentprocess->signalhandler=handler;
return;
}

/*
* Send signal to process
*
* In: process	Process to send signal to
	      size_t signal	Signal to send
*
* Returns -1 on error or 0 on success
*
*/

size_t sendsignal(size_t process,size_t signal) {
PROCESS *next;
next=processes;

while(next != NULL) {
	if(next->pid == process) break;
	next=next->next;
}

if(next == NULL) {
	setlasterror(INVALID_PROCESS);		/* invalid process */
	return(-1);
}


if(next->signalhandler != NULL) {		/* if the process has a signal handler */
	disablemultitasking();
	signalno=signal;			/* save signal number so that when the process address space changes it won't be lost */

	thisprocess=getpid();			/* get current process */

	loadpagetable(process);			/* switch to process address space */

	next->signalhandler(signalno);		/*call signal handler */
	loadpagetable(thisprocess);		/* switch back to original address space */

	enablemultitasking();
}

setlasterror(NO_ERROR);
return(0);
}

/*
* Set critical error handler
*
* In: void *addr	Function to call
*
* Returns nothing
*
*/

size_t set_critical_error_handler(void *addr) {
currentprocess->errorhandler=addr;
return;
}

/*
* Call critical error handler
*
* In: name    Error message to display
	      drive	Drive number if invoked on error from block device
	      flags	Flags
	      error	Error number
*
* Returns -1 on error or critical error handler on success
*
*/

size_t call_critical_error_handler(char *name,size_t drive,size_t flags,size_t error) {
if(currentprocess == NULL) return(-1);

if(currentprocess->errorhandler == NULL)  return(-1);

return(currentprocess->errorhandler(name,drive,flags,error));
}

/*
* Initialize process manager
*
* In: nothing
*
* Returns nothing
*
*/

void processmanager_init(void) {
processes=NULL;
currentprocess=NULL;
executableformats=NULL;
highest_pid_used=0;
last_error_no_process=0;

initialize_mutex(&process_mutex);
}

/*
* Block process
*
* In: pid	PID of process to block
*
* Returns -1 on error or 0 on success
*
*/

size_t blockprocess(size_t pid) {
PROCESS *next;

next=processes;
	
while(next != NULL) {
	if(next->pid == pid) {			/* found process */
		next->flags |= PROCESS_BLOCKED;			/* block process */
	
		next->blockedprocess=currentprocess;	/* save process descriptor */

		if(pid == getpid()) yield();		/* switch to next process if blocking current process */
		return(0);
	}
	
	next=next->next;
}

setlasterror(INVALID_PROCESS);		/* invalid process */
return(-1);
}

/*
* Unblock process
*
* In: pid		PID of process to unblock
*
* Returns -1 on error or 0 on success
*
*/

size_t unblockprocess(size_t pid) {
PROCESS *next;
PROCESS *bp;

next=processes;
	
while(next != NULL) {
	if(next->pid == pid) {			/* found process */
		next->flags &= ~PROCESS_BLOCKED;			/* unblock process */

		if((pid != getpid()) && (next->blockedprocess != NULL)) {
			bp=next->blockedprocess;
			next->blockedprocess=NULL;

			switch_task_process_descriptor(bp);		/* switch to blocked process */
	
			return;
		}

		return(0);
	}
	
	next=next->next;
}

setlasterror(INVALID_PROCESS);		/* invalid process */
return(-1);
}


/*
* Get enviroment variables address
*
* In:  Nothing
*
* Returns  enviroment variables address
*
*/
char *getenv() {
return(currentprocess->envptr);
}

/*
* Save kernel stack pointer for current process
*
* In:  new_stack_pointer	stack pointer
*
* Returns: Nothing
*
*/
void save_kernel_stack_pointer(size_t new_stack_pointer) {
if(currentprocess == NULL) return;

currentprocess->kernelstackpointer=new_stack_pointer;
}

/*
* Get kernel stack pointer for current process
*
* In:  Nothing
*
* Returns:  stack pointer or NULL if there are no processes running
*
*/
size_t get_kernel_stack_pointer(void) {
if(currentprocess == NULL) return(NULL);

return(currentprocess->kernelstackpointer);
}

/*
* Get top of kernel stack
*
* In:  Nothing
*
* Returns: Address of top of kernel stack or NULL if there are no processes running
*
*/
size_t get_kernel_stack_top(void) {
if(currentprocess == NULL) return(NULL);

return(currentprocess->kernelstacktop);
}

/*
* Get bottom of kernel stack
*
* In:  Nothing
*
* Returns: Bottom address of kernel stack or NULL if there are no processes running
*
*/
size_t get_kernel_stack_base(void) {
if(currentprocess == NULL) return(NULL);

return(currentprocess->kernelstackbase);
}

/*
* Set bottom of kernel stack
*
* In:  New address
*
* Returns: Nothing
*
*/
void set_kernel_stack_base(void *stackptr) {
if(currentprocess == NULL) return;

currentprocess->kernelstackbase=stackptr;
}

/*
* Get pointer to processes list
*
* In:  Nothing
*
* Returns: Pointer to processes list
*
*/

PROCESS *get_processes_pointer(void) {
return(processes);
}

/*
* Find first process
*
* In: processbuf	Pointer to buffer to hold process information
*
* Returns -1 on error or 0 on success
*
*/

PROCESS *findfirstprocess(PROCESS *processbuf) {
memcpy(processbuf,processes,sizeof(PROCESS));		/* get first process */

return(processes->next);
}

/*
* Find next process
*
* In: previousprocess	Pointer to previous process returned from call to get_processes_pointer() or findnextprocess()
*     processbuf	Pointer to buffer to hold process information
*
* Returns NULL on error or pointer to next process on success
*
*/

PROCESS *findnextprocess(PROCESS *previousprocess,PROCESS *processbuf) { 
if(previousprocess->next == NULL) return(NULL);		/* end of processes */

memcpy(processbuf,previousprocess->next,sizeof(PROCESS));		/* get next process */

return(previousprocess->next);
}

/*
* Update current process pointer
*
* In:  Pointer to process
*
* Returns: Nothing
*
*/

void update_current_process_pointer(PROCESS *ptr) {
currentprocess=ptr;
}

/*
* Get current process pointer
*
* In:  Nothing
*
* Returns: current process pointer
*
*/
PROCESS *get_current_process_pointer(void) {
return(currentprocess);
}

/*
* next process pointer
*
* In:  Nothing
*
* Returns: next process pointer or NULL if no current process exists
*
*/
PROCESS *get_next_process_pointer(void) {
if(currentprocess == NULL) return(NULL);

return(currentprocess->next);
}

/*
* Register executable format
*
* In:  format	Pointer to executable format struct
*
* Returns: current process pointer
*
*/
size_t register_executable_format(EXECUTABLEFORMAT *format) {
EXECUTABLEFORMAT *next;
EXECUTABLEFORMAT *last;

if(executableformats == NULL) {				/* first in list */
	executableformats=kernelalloc(sizeof(EXECUTABLEFORMAT));		/* add first */
	if(executableformats == NULL) return(-1);

	next=executableformats;
}
else						
{
	/* Find end of list and check if executable format exists */

	next=executableformats;

	while(next != NULL) {
	 last=next;

	 if(strncmpi(next->name,format->name,MAX_PATH)) {		/* Return error if format exists */
	  setlasterror(MODULE_ALREADY_LOADED);
	  return(-1);
	 }

	 next=next->next;
	}

/* add new format entry to end */

	last->next=kernelalloc(sizeof(EXECUTABLEFORMAT));
	if(last->next == NULL) return(-1);

	next=last->next;
}

memcpy(next,format,sizeof(EXECUTABLEFORMAT));		/* copy data to entry */

next->next=NULL;

setlasterror(NO_ERROR);
return(0);
}

/*
* Load executable
*
* In:  filename		Name of file to load
*
* Returns: 0 on success or -1 on error
*
*/
size_t load_executable(char *filename) {
EXECUTABLEFORMAT *next;
char *buffer[MAX_PATH];
size_t handle;

/* read magic number */

handle=open(filename,O_RDONLY);		/* open file */
if(handle == -1) return(-1);		/* can't open */

if(read(handle,buffer,MAX_PATH) == -1) {
	close(handle);
	return(-1);
}

close(handle);

/* find format and call handler */

next=executableformats;

while(next != NULL) {
	if(memcmp(next->magic,buffer,next->magicsize) == 0) {		/* found format */
		return(next->callexec(filename));				/* call via function pointer */
	}

	next=next->next;
}

setlasterror(NOT_IMPLEMENTED);
return(-1);
}


/*
* Reset process tick counter
*
* In:  Nothing
*
* Returns: Nothing
*
*/
void reset_process_ticks(void) {
 currentprocess->ticks=0;
}

/*
* Get process tick count
*
* In:  Nothing
*
* Returns: Process tick count
*
*/
size_t get_tick_count(void) {
 return(tickcount);
}

/*
* Increment process tick counter
*
* In:  Nothing
*
* Returns: New tick count
*
*/
size_t increment_tick_count(void) {
return(++tickcount);
}

size_t get_max_tick_count(void) {
return(currentprocess->maxticks);
}

/*
* Wait for x ticks
*
* In:  ticks	Number of ticks to wait
*
* Returns: Nothing
*
*/
void kwait(size_t ticks) {
size_t newticks;

newticks=get_tick_count()+ticks;			/* Get last tick */

while(get_tick_count() < newticks) ;;			/* until tickcount is at last tick */
}

size_t GetVersion(void) {
return((CCP_MAJOR_VERSION << 24) | (CCP_MINOR_VERSION << 16) | (CCP_RELEASE_VERSION << 8));
}

