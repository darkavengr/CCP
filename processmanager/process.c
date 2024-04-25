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
#include "../header/kernelhigh.h"
#include "../header/errors.h"
#include "mutex.h"
#include "../filemanager/vfs.h"
#include "../memorymanager/memorymanager.h"
#include "process.h"
#include "../devicemanager/device.h"
#include "signal.h"
#include "../header/bootinfo.h"
#include "../header/debug.h"

extern initializestack(void *ptr,size_t size);
extern get_stack_top(void);
extern get_stack_pointer(void);
extern irq_exit;

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
PROCESS *last;
PSP *psp=0;
size_t *stackinit;
char *fullpath[MAX_PATH];

disablemultitasking(); 
disable_interrupts();

/* add process to process list and find process id */

if(processes == NULL) {  
	processes=kernelalloc(sizeof(PROCESS));

	if(processes == NULL) {
		loadpagetable(getppid());
		freepages(highest_pid_used);
		enablemultitasking();	

		close(handle);
		return(-1);
	}

processes_end=processes;

next=processes;
}
else
{
	processes_end->next=kernelalloc(sizeof(PROCESS));
	if(processes_end->next == NULL) {
		loadpagetable(getppid());
		freepages(highest_pid_used);
		enablemultitasking();	

		close(handle);
		setlasterror(NO_MEM);
		return(-1);
	}

next=processes_end->next;
}

getfullpath(filename,fullpath);

/* create struct */
strcpy(next->filename,fullpath);
strcpy(next->args,argsx);
strcpy(tempfilename,fullpath);
strcpy(tempargs,argsx);

getcwd(next->currentdir);		/* current directory */
next->maxticks=DEFAULT_QUANTUM_COUNT;
next->ticks=0;
next->parentprocess=getpid();
next->next=NULL;
next->pid=highest_pid_used;
next->errorhandler=NULL;
next->signalhandler=NULL;
next->rc=0;
next->flags=0;
next->lasterror=0;
next->ticks=0;
next->maxticks=DEFAULT_QUANTUM_COUNT;
next->next=NULL;


next->kernelstacktop=kernelalloc(PROCESS_STACK_SIZE);
if(next->kernelstacktop == NULL) {
	currentprocess=oldprocess;
	loadpagetable(getpid());
	freepages(highest_pid_used);
	enablemultitasking();

	close(handle);
	return(-1);
}

next->kernelstacktop += PROCESS_STACK_SIZE;			/* top of kernel stack */
next->kernelstackpointer=next->kernelstacktop;			/* intial kernel stack address */

/* Enviroment variables are inherited
* Part one of enviroment variables duplication
*
* copy the variables to a buffer in kernel memory where it is visible to both parent and child process
*/

saveenv=NULL;

if(currentprocess != NULL) {
	saveenv=kernelalloc(ENVIROMENT_SIZE);
	memcpy(saveenv,currentprocess->envptr,ENVIROMENT_SIZE);
}

page_init(highest_pid_used);				/* intialize page directory */	
loadpagetable(highest_pid_used);				/* load page table */

currentprocess=next;				/* use new process */
	
highest_pid_used++;

/* Part two of enviroment variables duplication
*
* copy variables from buffer */

currentprocess->envptr=alloc_int(ALLOC_NORMAL,getpid(),ENVIROMENT_SIZE,KERNEL_HIGH-ENVIROMENT_SIZE-1);

if(saveenv != NULL) {
	memcpy(currentprocess->envptr,saveenv,ENVIROMENT_SIZE);
	kernelfree(saveenv);
}

/* allocate user mode stack */
stackp=alloc_int(ALLOC_NORMAL,getpid(),PROCESS_STACK_SIZE,KERNEL_HIGH-PROCESS_STACK_SIZE-ENVIROMENT_SIZE);
if(stackp == NULL) {
	currentprocess=oldprocess;
	loadpagetable(getpid());
	freepages(highest_pid_used);
	enablemultitasking();	
	return(-1);
}

next->stackbase=stackp;
next->stackpointer=stackp+PROCESS_STACK_SIZE;

/* duplicate stdin, stdout and stderr */

if(getpid() != 0) {
	dup_internal(stdin,getppid(),getpid());
	dup_internal(stdout,getppid(),getpid());
	dup_internal(stderr,getppid(),getpid());
}

processes_end=next;						/* save last process address */

enable_interrupts();

entrypoint=load_executable(tempfilename);			/* load executable */
disable_interrupts();

if(entrypoint == -1) {
	kernelfree(next);

	currentprocess=oldprocess;			/* restore previous process */

	loadpagetable(getpid());
	enablemultitasking();
	return(-1);
}

/* create process heap */

currentprocess->heapaddress=alloc_int(ALLOC_NORMAL,getpid(),INITIAL_HEAP_SIZE,-1);	/* allocate process heap */
if(currentprocess->heapaddress == NULL) {
	currentprocess=oldprocess;			/* restore previous process */

	loadpagetable(getpid());
	enablemultitasking();
	return(-1);
}

currentprocess->heapend=currentprocess->heapaddress+INITIAL_HEAP_SIZE;		/* end of heap */

initializekernelstack(currentprocess->kernelstacktop,entrypoint,currentprocess->kernelstacktop-PROCESS_STACK_SIZE);	/* initial kernel stack */

/* create psp */ 
ksprintf(psp->commandline,"%s %s",next->filename,next->args);

psp->cmdlinesize=strlen(psp->commandline);

if((flags & PROCESS_FLAG_BACKGROUND)) {			/* run process in background */
	currentprocess=oldprocess;			/* restore previous process */

	loadpagetable(getpid());
	enablemultitasking();
	enable_interrupts();

	return;
}
else
{
	initializestack(currentprocess->stackpointer,PROCESS_STACK_SIZE);	/* intialize user mode stack */

	enablemultitasking();

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

shut();			/* close open files for process */

oldprocess=getpid();

next=processes;
last=next;
	
while(next != NULL) {

	if(next->pid == process) break;
	last=next;
	next=next->next;
}

if(next == NULL) {
	setlasterror(INVALID_PROCESS);		/* invalid process */

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

	sendsignal(-1,SIGTERM);		/*  send terminate signal */

	kprintf_direct("Waiting %d seconds for processes to terminate\n",SHUTDOWN_WAIT);
	kwait(SHUTDOWN_WAIT);
	
	if(processes != NULL) kprintf_direct("processes did not terminate in time (too bad)\n");

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
      argsone			Function code of function to call
*
* Returns -1 on error or function return value on success
*
*/

size_t dispatchhandler(void *argsix,void *argfive,void *argfour,void *argthree,void *argtwo,size_t argone) {
char *b;
char x;
size_t count;
FILERECORD findbuf;
size_t highbyte;
size_t lowbyte;

argone=argone & 0xffff;				/* get 16 bits */

highbyte=(argone & 0xff00) >> 8;
lowbyte=(argone & 0xff);

switch(highbyte) {		/* function */
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

	case 0x4c:			/* terminate process */
		return(exit(lowbyte));

	case 0x50:			/* get pid */
		return(getpid());

	case 0x3e:			/* close */
		return(close((size_t) argtwo));  

	case 0x3c:			/* create */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(create((char *) argfour));
	  
	case 0x41:                     /* delete */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(unlink((char *) argfour));
	 
	case 0x4e:			/* find first file */
		if((argfour >= KERNEL_HIGH) || (argtwo >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(findfirst(argfour,argtwo));	
	
	case 0x4f:			/* find next file*/
		if((argfour >= KERNEL_HIGH) || (argtwo >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(findnext(argfour,argtwo));
	  
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
	 
	case 0x3f:			/* read from file or device */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(read((size_t)  argtwo,(char *) argfour,(size_t)  argthree));
	 
	case 0x40:			/* write to file or device */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(write((size_t)  argtwo,(char *) argfour,(size_t)  argthree));
	  
	case 0x3b:			/* change directory */
		if(argfour >= KERNEL_HIGH) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(chdir((char *) argfour)); 
	
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


	case 0x56:			/* rename file or directory */
		if((argfive >= KERNEL_HIGH) || (argsix >= KERNEL_HIGH)) {		/* invalid argument */
			setlasterror(INVALID_ADDRESS);
			return(-1);
		}

		return(rename((char *) argfive,(char *) argsix));

	case 0x65:			/* get process information */
		memcpy(argtwo,currentprocess,sizeof(PROCESS));
		break; 
	  
	case 0x30:			/* get version */
		return(CCPVER);
	 
	case 0x4d:			/* get last error */
		return(getlasterror());

	case 0x8c:			/* set signal handler */
		signal(argfour);

	case 0x8d:			/* send signal */
		return(sendsignal((size_t) argtwo,argfour));

	case 0x8a:			/* wait for process to change state */
		return(wait((size_t)  argtwo));

}

	switch(argone) {
	 
		case 0x3d01:		/* open file or device */
			if(argfour >= KERNEL_HIGH) {		/* invalid argument */
				setlasterror(INVALID_ADDRESS);
				return(-1);
			}

			return(open((char *) argfour,_O_RDONLY));

		case 0x3d02:
			if(argfour >= KERNEL_HIGH) {		/* invalid argument */
				setlasterror(INVALID_ADDRESS);
				return(-1);
			}

			return(open((char *) argfour,_O_RDWR));

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
	 
		case 0x4800:			/* allocate memory from heap*/
			return(alloc((size_t)  argtwo));

		case 0x4801:			/* allocate memory from page frame allocator*/	
			return(alloc_int(ALLOC_NORMAL,getpid(),(size_t) argtwo,-1));

		case 0x4900:			/* free from heap */
			free(argfour);
			return;

		case 0x4901:			/* free from slab allocation */
			free_internal(getpid(),(size_t) argtwo,0);
			return;

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

	 	case 0x4401:			/* send commands to device */
	  		return(ioctl((size_t) argtwo,(unsigned long) argthree,(void *) argsix));

	 	case 0x4b00:			/* create process and load executable */
			if((argfour >= KERNEL_HIGH) || (argtwo >= KERNEL_HIGH)) {		/* invalid argument */
				setlasterror(INVALID_ADDRESS);
				return(-1);
			}

	  		return(exec(argfour,argtwo,FALSE));
	 
	 	case 0x4b02:
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

	 		return(load_kernel_module(argfour));

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
	 if(getblockdevice(boot_info->drive,&blockdevice) == -1) return(-1);

	 b=dir;						/* create directory name */
	 *b++=(uint8_t) blockdevice.drive+'A';
	 *b++=':';
	 *b++='\\';
	 *b++=0;

	 setlasterror(NO_ERROR);
	 return(NO_ERROR);
}

strcpy(dir,currentprocess->currentdir);

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

/* findfirst (and other other filesystem calls) use currentprocess->currentdir to get the
	  full path of a file, but this function checks that a new current directory is valid,
	  so it must copy the new current directory to currentprocess->currentdir to make sure that 
	  it will be used to validate it.

	  If it fails, the old path is restored
*/

strcpy(savecwd,currentprocess->currentdir);		/* save current directory */
memcpy(currentprocess->currentdir,fullpath,3);		/* get new path */

/* check if directory exists */

if(findfirst(fullpath,&chdir_file_record) == -1) {		/* path doesn't exist */
	strcpy(currentprocess->currentdir,savecwd);		/* restore old current directory */
	return(-1);		
}

if((chdir_file_record.flags & FILE_DIRECTORY) == 0) {		/* not directory */
	setlasterror(NOT_DIRECTORY);
	return(-1);
}

strcpy(currentprocess->currentdir,fullpath);	/* set directory */

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
	if(currentprocess == NULL) return(last_error_no_process);

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
if(currentprocess == NULL) {
	strcpy(buf,"<unknown>");
	return;
}

strcpy(buf,currentprocess->filename);
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
if(currentprocess == NULL) return;

strcpy(buf,currentprocess->args);
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
if(currentprocess == NULL) return(0);

return(currentprocess->parentprocess);
}

/*
* Get process return code
*
* In: nothing
*
* Returns process return code
*
*/

size_t getreturncode(void) {
if(currentprocess == NULL) return(0);

return(currentprocess->rc);
}

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


if(next->signalhandler != NULL) {
	disablemultitasking();
	signalno=signal;		/* save signal number so that when the process address space changes it won't be lost */

	thisprocess=getpid();	/* get current process */

	loadpagetable(process);			/* switch to process address space */

	next->signalhandler(signalno);	/*call signal handler */
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
* Returns -1 on error or 0 on success
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
* Register executable format.
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

	 if(strcmpi(next->name,format->name)) {		/* Return error if format exists */
	  setlasterror(DRIVER_ALREADY_LOADED);
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

handle=open(filename,_O_RDONLY);		/* open file */
if(handle == -1) return(-1);		/* can't open */

if(read(handle,buffer,MAX_PATH) == -1) {
	close(handle);
	return(-1);
}

close(handle);

/* find format */

next=executableformats;

while(next != NULL) {
	if(memcmp(next->magic,buffer,next->magicsize) == 0) {		/* found format */
	 return(next->callexec(filename));				/* call via function pointer */
	}

	next=next->next;
}

setlasterror(INVALID_BINFMT);
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
* Returns: Nothing
*
*/
void increment_tick_count(void) {
tickcount++;
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

newticks=get_tick_count()+ticks;

while(get_tick_count() < newticks) ;;
}

/*
* Get heap address
*
* In: nothing
*
* Returns: Heap address
*
*/

HEAPENTRY *GetUserHeapAddress(void) {
return(currentprocess->heapaddress);
}


/*
* Get user heap end
*
* In: nothing
*
* Returns: Heap size
*
*/

HEAPENTRY *GetUserHeapEnd(void) {
return(currentprocess->heapend);
}

/*
* Set user heap end
*
* In: User heap end
*
* Returns: Nothing
*
*/

void SetUserHeapEnd(HEAPENTRY *end) {
currentprocess->heapend=end;
}

/*
* Set user heap address
*
* In: User heap address
*
* Returns: Nothing
*
*/

void SetUserHeapAddress(HEAPENTRY *heap) {
currentprocess->heapaddress=heap;
}
