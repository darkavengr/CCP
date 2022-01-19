/*  CCP Version 0.0.1
    (C) Matthew Boote 2020

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
#include "../header/errors.h"
#include "../devicemanager/device.h"
#include "../filemanager/vfs.h"
#include "../memorymanager/memorymanager.h"
#include "process.h"
#include "signal.h"
#include "../header/bootinfo.h"
#include "mutex.h"

#define KERNEL_HIGH (1 << (sizeof(size_t) *8)-1)


size_t exec(char *filename,char *argsx,size_t flags);
size_t kill(size_t process);
size_t exit(size_t val);
void shutdown(size_t shutdown_status); 
size_t findfirstprocess(PROCESS *buf); 
size_t findnextprocess(PROCESS *buf); 
size_t dispatchhandler(void *argsix,void *argfive,void *argfour,void *argthree,void *argtwo,size_t argone);
size_t getcwd(char *dir);
size_t chdir(char *dirname);
size_t getpid(void);
size_t setlasterror(size_t err);
size_t getlasterror(void);
size_t getprocessfilename(char *buf);
size_t getwriteconsolehandle(void);
size_t getreadconsolehandle(void);
size_t getprocessargs(char *buf);
size_t getppid(void);
size_t getreturncode(void);
size_t getprocessflags(void);
size_t ksleep(size_t wait);
size_t signal(void *handler);
size_t sendsignal(size_t process,size_t signal);
size_t set_critical_error_handler(void *addr);
size_t call_critical_error_handler(char *name,size_t drive,size_t flags,size_t error);

size_t last_error_no_process=0;
uint32_t entry;
int type;
PROCESS *processes=NULL;
PROCESS *currentprocess=NULL;
char *temp_filename[MAX_PATH];
char *temp_args[MAX_PATH];
MUTEX process_mutex;

 /* load program */
size_t exec(char *filename,char *argsx,size_t flags) {
 size_t handle;
 char *fullname[MAX_PATH];
 size_t count;
 void *kernelstackbase;
 PROCESS *next;
 size_t stackp;
 Elf32_Ehdr elf_header;
 size_t processcount;
 PROCESS *last;
 Elf32_Phdr *phbuf;
 size_t *haha;
 FILERECORD *filenext;
 PSP *psp=0;
 PROCESS *oldprocess;
 size_t kernelstackp;

/* disable task switching without disabling interrupts -
   interrupts need to be enabled for device/io */

disablemultitasking();
enable_interrupts();

getfullpath(filename,fullname);

strcpy(temp_filename,fullname);			/* save filename and args */
strcpy(temp_args,argsx);

/*
 * load program to memory */

handle=open(fullname,_O_RDONLY);		/* open file */
if(handle == -1) return(-1);		/* can't open */

if(read(handle,&elf_header,sizeof(Elf32_Ehdr)) == -1) {
 enablemultitasking();
 return(-1); /* read error */
}

entry=elf_header.e_entry;		/* get entry point */
type=elf_header.e_type;

/* check magic number, elf type and number of program headers */

if(elf_header.e_ident[0] != 0x7F && elf_header.e_ident[1] != 0x45 && elf_header.e_ident[2] != 0x4C && elf_header.e_ident[3] != 0x46) {	/*  elf */
 close(handle);
 enablemultitasking();
 setlasterror(BAD_EXEC);
 return(-1);
}

if(elf_header.e_type != ET_EXEC) {		/* not executable */
 close(handle);
 enablemultitasking();

 setlasterror(BAD_EXEC);
 return(-1);
}

if(elf_header.e_phnum == 0) {			/* no program headers */
 loadpagetable(getppid());
 enablemultitasking();
 
 setlasterror(BAD_EXEC);
 return(-1);
}

/* find new process id */

 processcount=0;
 next=processes;

 while(next != NULL) {
  if(next->pid >= processcount) processcount=next->pid+1;
  next=next->next;				/* find end */

 }

page_init(processcount);				/* intialize page directory */	
loadpagetable(processcount);				/* load page table */

/* add process to process list */

if(processes == NULL) {  
 processes=alloc_int(ALLOC_KERNEL,processcount,sizeof(PROCESS),-1);

 if(processes == NULL) {
  loadpagetable(getppid());
  freepages(processcount);
  enablemultitasking();	
  return(-1);
 }

next=processes;

last=next;
}
else
{
 next=processes;

 while(next->next != NULL) {
  last=next;

  next=next->next;				/* find end */
 }

 next->next=alloc_int(ALLOC_KERNEL,processcount,sizeof(PROCESS),-1);

 next=next->next;

 if(next == NULL) {
  loadpagetable(getppid());
  freepages(processcount);
  enablemultitasking();	

  close(handle);
  setlasterror(NO_MEM);
  return(-1);
 }
}

 /* create struct */

strcpy(next->filename,temp_filename);
strcpy(next->args,temp_args);
	
getcwd(next->currentdir);		/* current directory */
next->maxticks=DEFAULT_QUANTUM_COUNT;
next->ticks=0;
next->parentprocess=getpid();
next->next=NULL;
next->pid=processcount;
next->errorhandler=NULL;
next->signalhandler=NULL;
next->rc=0;
next->flags=0;
next->writeconsolehandle=stdout;
next->readconsolehandle=stdin;
next->lasterror=0;
next->ticks=0;
next->maxticks=DEFAULT_QUANTUM_COUNT;
next->next=NULL;

oldprocess=currentprocess;
currentprocess=next;		/* set current process */

/* allocate buffer for program headers */

 phbuf=alloc_int(ALLOC_KERNEL,processcount,(elf_header.e_phentsize*elf_header.e_phnum),-1);
 if(phbuf == NULL) {
  loadpagetable(getpid());
  freepages(processcount);
  enablemultitasking();

  currentprocess=oldprocess;
  return(-1);
 }

 if(seek(handle,elf_header.e_phoff,SEEK_SET) == -1) {
  currentprocess=oldprocess;

  loadpagetable(getpid());
  freepages(processcount);
  kernelfree(phbuf);
  enablemultitasking();	
  return(-1);
 }

/* read program headers */

 if(read(handle,phbuf,(elf_header.e_phentsize*elf_header.e_phnum)) == -1) {
  currentprocess=oldprocess;

  loadpagetable(getpid());
  freepages(processcount);
  kernelfree(phbuf);
  enablemultitasking();	

  return(-1);
}

/*
 * go through program headers and load them if they are PT_LOAD */

 for(count=0;count<elf_header.e_phnum;count++) {

  if(phbuf->p_type == PT_LOAD) {			/* if segment is loadable */

   seek(handle,phbuf->p_offset,SEEK_SET);			/* seek to position */

   alloc_int(ALLOC_NORMAL,processcount,phbuf->p_memsz,phbuf->p_vaddr);

   if(read(handle,phbuf->p_vaddr,phbuf->p_filesz) == -1) {	/* read error */
     if(getlasterror() != END_OF_FILE) {
      currentprocess=oldprocess; 

      loadpagetable(getpid());
      freepages(processcount);
      kernelfree(phbuf);
      enablemultitasking();	
   
      return(-1);
     }
   }

  }

  phbuf++;		/* point to next */

 }

close(handle);

kernelfree(phbuf);

/* allocate user mode stack */

stackp=alloc_int(ALLOC_NORMAL,processcount,PROCESS_STACK_SIZE,KERNEL_HIGH-PROCESS_STACK_SIZE);
if(stackp == NULL) {
 currentprocess=oldprocess;

 loadpagetable(getpid());
 freepages(processcount);
 enablemultitasking();	
 return(-1);
}

/* allocate kernel mode stack */

kernelstackp=kernelalloc(PROCESS_STACK_SIZE);
if(kernelstackp == NULL) {
 currentprocess=oldprocess;

 loadpagetable(getpid());
 freepages(processcount);
 enablemultitasking();	
 return(-1);
}

disable_interrupts();

next->stackbase=stackp;
next->stackpointer=stackp+PROCESS_STACK_SIZE;
next->kernelstackbase=kernelstackp;
next->kernelstackpointer=kernelstackp+PROCESS_STACK_SIZE;

/* create psp */ 

ksprintf(psp->commandline,"%s %s",next->filename,next->args);
psp->cmdlinesize=strlen(psp->commandline);

enable_interrupts();

if((flags & PROCESS_FLAG_BACKGROUND)) {			/* run process in background */
 currentprocess=oldprocess;

 loadpagetable(getpid());
 enablemultitasking();
 return;
}
else
{
 initializestack(currentprocess->stackpointer,PROCESS_STACK_SIZE);

 switch_kernel_stack(currentprocess->kernelstackpointer);	/* switch kernel stack */

 enablemultitasking();

 switch_to_usermode_and_call_process(entry);		/* switch to user mode, enable interrupts, and call process */
}

}
/*

 * terminate process
 *
 */

size_t kill(size_t process) {
PROCESS *last;
PROCESS *next;
PROCESS *nextprocess;
char *b;
size_t oldprocess;

lock_mutex(&process_mutex);			/* lock mutex */

oldprocess=getpid();

next=processes;
last=next;
	
while(next != NULL) {

 if(next->pid == process) break;
 last=next;
 next=next->next;
}

if(next == NULL) {
 setlasterror(BAD_PROCESS);		/* invalid process */

 unlock_mutex(&process_mutex);			/* unlock mutex */
 enable_interrupts();
 return(-1);
}

if(processes == NULL) {
 enable_interrupts();
 shutdown(0);
}

if((next->flags & PROCESS_WAITING) == 0) {		/* process is not being waited on */
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
  kprintf("No processes, system will shut down\n");
  shutdown(_SHUTDOWN);
 }
 else
 {
 kernelfree(next);
 freepages(oldprocess);
 }

}

unlock_mutex(&process_mutex);			/* unlock mutex */

//if(next->pid == oldprocess) yield();			/* switch to next process if killing current process */
while(1) ;;

return;
}

size_t exit(size_t val) {
  kill(getpid());			/* kill current process */
  return(val);	
}

/* shutdown */

void shutdown(size_t shutdown_status) { 
 size_t newtick;

 kprintf("Shutting down:\n");
 kprintf("Sending SIGTERM signal to all processes...\n");

 sendsignal(-1,SIGTERM);		/*  send terminate signal */

 kprintf("Waiting %d seconds for processes to terminate\n",SHUTDOWN_WAIT);
 ksleep(SHUTDOWN_WAIT);
 
 newtick=gettickcount()+SHUTDOWN_WAIT;	/* wait for processes to terminate */
	
 while(gettickcount() < newtick) {
  if(processes == NULL) break;
 }

 if(processes != NULL) kprintf("processes did not terminate in time (too bad)\n");

 if(shutdown_status == _SHUTDOWN) {
  kprintf("It is now safe to turn off your computer\n");
  while(1) halt();		/* loop */
 }
 else
 {
  restart();
 }
 
}
  

size_t findfirstprocess(PROCESS *buf) { 
 memcpy(buf,processes,(size_t) sizeof(PROCESS));

 currentprocess->findptr=processes;
 return(NO_ERROR);
}

size_t findnextprocess(PROCESS *buf) { 
 PROCESS *findptr=currentprocess->findptr;

 findptr=findptr->next;
 currentprocess->findptr=findptr;

 if(findptr == NULL) return(-1);

 memcpy(buf,findptr,(size_t)  sizeof(PROCESS));
 findptr=findptr->next;
 return(NO_ERROR);
}
  

size_t wait(size_t pid) {
PROCESS *next;
size_t oldflags;
size_t newflags;

lock_mutex(&process_mutex);			/* lock mutex */

next=processes;
		
while(next != NULL) {
 if(next->pid == pid) {			/* found process */

  unlock_mutex(&process_mutex);			/* unlock mutex */

  next->flags |= PROCESS_WAITING;

  oldflags=next->flags;
  newflags=next->flags;
 
  while(newflags == oldflags) ;;		/* wait for process to change state */

  setlasterror(NO_ERROR);
  return(0);
 }

 next=next->next;
}

setlasterror(BAD_PROCESS);		/* invalid process */
return(-1);
}

/* dispatcher */

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
   kprintf((char *) argfour);
   return;

  case 0x4c:			/* terminate process */
   return(exit(lowbyte));

  case 0x50:			/* get pid */
   return(getpid());

  case 0x3e:			/* close */
   return(close((size_t) argtwo));  

  case 0x3c:			/* create */
   return(create((char *) argfour));
   
  case 0x41:                     /* delete */
   return(delete((char *) argfour));
  
 case 0x4e:			/* findfirst */
   return(findfirst(argfour,argtwo));	
	
  case 0x4f:			/* findnext */
    return(findnext(argfour,argtwo));
   
  case 0x45:			/* dup */
  return(dup(argtwo));

  case 0x46:			/* dup2 */
  return(dup2(argtwo,argthree));

  case 0x47:			/* getcwd */
   return(getcwd((char *) argfour));
  
  case 0x3f:			/* read */
   return(read((size_t)  argtwo,(char *) argfour,(size_t)  argthree));
  
  case 0x40:			/* write */
   return(write((size_t)  argtwo,(char *) argfour,(size_t)  argthree));
   
  case 0x3b:			/* chdir */
   return(chdir((char *) argfour)); 
 
  case 0x39:			/* mkdir */
   return(mkdir((char *) argfour));

  case 0x3a:			/* rmdir */
   return(rmdir((char *) argfour));

  case 0x56:			/* rename */
   return(rename((char *) argfive,(char *) argsix));

  case 0x65:			/* get struct */
   memcpy(argtwo,currentprocess,sizeof(PROCESS));
   break; 
   
  case 0x30:			/* get version */
   return(CCPVER);
  
  case 0x48:			/* allocate memory */
   return(alloc((size_t)  argtwo));

  case 0x49:			/* free */
   free(argtwo);
   return;

  case 0x4d:	
   return(getlasterror());

  case 0x90:				/* disk block read */
   return(blockio(_READ,argthree,argfour));

  case 0x91:				/* disk block write */
   return(blockio(_WRITE,argthree,argfour));

  case 0x8c:				/* set signal handler */
   return(sendsignal(argtwo,argfour));

  case 0x8d:				/* send signal */
   return(signal(argfour));

  case 0x8a:				/* wait for process to change state */
   return(wait((size_t)  argtwo));

}

 switch(argone) {		/* function */
  
  case 0x3d01:			/* open */
   return(open((char *) argfour,_O_RDONLY));

  case 0x3d02:
   return(open((char *) argfour,_O_RDWR));

  case 0x4200:		/* seek */
   return(seek((size_t)  argtwo,argfour,SEEK_SET));

  case 0x4201:		/* seek */
   return(seek((size_t)  argtwo,argfour,SEEK_CUR));

  case 0x4202:		/* seek */
   return(seek((size_t)  argtwo,argfour,SEEK_END));

  case 0x4300:			/* get file attributes */
   findfirst(argfour,&findbuf);
   return(findbuf.attribs);

  case 0x4301:			/* set file attributes */
   return(chmod(argfour,(size_t)  argtwo));
  
  case 0x5700:			/* get file time and date */
   findfirst(argfour,&findbuf);
   return;
  
  case 0x5701:				/* set file time date */
   return(setfiletimedate((size_t) argtwo,(size_t) argfour));

  case 0x4401:			/* ioctl */
   return(ioctl((size_t) argtwo,(unsigned long) argthree,(void *) argsix));

  case 0x4b00:			/* exec */
   return(exec(argfour,argtwo,FALSE));
	 
  case 0x4b02:
   return(exec(argfour,argtwo,TRUE));

  case 0x4b03:				/* load driver */
   return(load_kernel_module(argfour));

  case 0x7000:
   return(dma_alloc(argtwo));
     	
  case 0x7002:			/* restart */
    shutdown(_RESET);
    break;

  case 0x7003:			/* shutdown */
    shutdown(_SHUTDOWN);
    break;

  case 0x7004:			/* tell */
    return(tell(argtwo));
	   
  case 0x7009:			/* find process */
    return(findfirstprocess((void *) argfour));

  case 0x700A:			/* find process */
    return(findnextprocess((void *) argfour));

  case 0x700B:			/* add block device */
    return(add_block_device(argfour));

  case 0x700C:			/* add char device */
   return(add_char_device(argfour));

   case 0x700D:			/* remove block device */
    return(remove_block_device(argfour));

   case 0x700E:			/* remove char device */
     return(remove_char_device(argfour));

   case 0x7019:			/* terminate process */
     return(kill((size_t)  argtwo));

   case 0x7022:				/* set tick count */
     currentprocess->maxticks=argtwo;
     return;

   case 0x7028:				/* get next free drive number */
     return(allocatedrive());

   case 0x7029:				/* get character device */
    return(findcharacterdevice(argtwo,argfour));

   case 0x702a:				/* get block device from drive number */
     return(getblockdevice(argtwo,argfour));

   case 0x702b:				/* get block device from name */
     return(getdevicebyname(argtwo,argfour));

   case 0x702d:				/* register filesystem */
    return(register_filesystem(argfour));

   case 0x702e:				/* lock mutex */
    return(lock_mutex(argfour));

   case 0x702f:				/* unlock mutex */
    return(unlock_mutex(argfour));

  default:
   setlasterror(INVALID_FUNCTION);
   return(-1);
  }
}

/*
 * get current directory
 *
 */

size_t getcwd(char *dir) {
 char *buf[255];
 char c;
 char *b;
 char *bootdriveloc=KERNEL_HIGH+(size_t)  BOOT_INFO_DRIVE;
 BLOCKDEVICE blockdevice;

/*
 * usually the directory is got from the current process struct
 * but if there are no processes running,the current directry
 * must be built up on the fly */

 if(currentprocess == NULL) {			/* no processes so get from boot drive */
  c=*bootdriveloc;						/* get boot drive from 0xAC */
  if(c >= 0x80) c=c-0x80;
 
  if(getblockdevice(c,&blockdevice) == -1) return(-1);

  b=dir;						/* set current directory for system */
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
 * set current directory
 *
 */

size_t chdir(char *dirname) {
char *fullpath[MAX_PATH];
char *savecwd[MAX_PATH];
FILERECORD buf;

getfullpath(dirname,fullpath);

/* findfirst (and other other filesystem calls) use currentprocess->currentdir to get the
   full path of a file, but this function checks that a new current directory is valid,
   so it must copy the new current directory to currentprocess->currentdir to make sure that 
   it will be used to validate it.

   If it fails, the old path is restored
*/

strcpy(savecwd,currentprocess->currentdir);		/* save current directory */
memcpy(currentprocess->currentdir,fullpath,3);		/* get new path */

if(findfirst(fullpath,&buf) == -1) {			/* path doesn't exist */
 strcpy(currentprocess->currentdir,savecwd);		/* restore old current directory */
 return(-1);		
}

strcpy(currentprocess->currentdir,fullpath);	/* set directory */

setlasterror(NO_ERROR);  
return(NO_ERROR);				/* no error */
}

size_t getpid(void) {
 if(currentprocess == NULL) return(0);
 return(currentprocess->pid);
}

size_t setlasterror(size_t err) {
 if(currentprocess == NULL) {
  last_error_no_process=err;
  return;
 }

 currentprocess->lasterror=err;
}

size_t getlasterror(void) {
 if(currentprocess == NULL) return(last_error_no_process);

 return(currentprocess->lasterror);
}

size_t getprocessfilename(char *buf) {
if(currentprocess == NULL) {
 strcpy(buf,"<unknown>");
 return;
}

strcpy(buf,currentprocess->filename);
}

size_t getwriteconsolehandle(void) {
 if(currentprocess == NULL) return(stdout);

 return(currentprocess->writeconsolehandle);
}

size_t getreadconsolehandle(void) {
 if(currentprocess == NULL) return(stdin);

 return(currentprocess->readconsolehandle);
}

size_t getprocessargs(char *buf) {
 if(currentprocess == NULL) return(0);
strcpy(buf,currentprocess->args);
}

size_t getppid(void) {
 if(currentprocess == NULL) return(0);
 return(currentprocess->parentprocess);
}
size_t getreturncode(void) {
 if(currentprocess == NULL) return(0);
 return(currentprocess->rc);
}

size_t getprocessflags(void) {
 if(currentprocess == NULL) return(0);
 return(currentprocess->flags);
}

void setprocessflags(size_t flags) {
 if(currentprocess == NULL) return(0);

 currentprocess->flags=flags;
 return;
}

size_t ksleep(size_t wait) {
size_t newtick;

newtick=gettickcount()+wait;

while(gettickcount() < newtick) ;;
}

size_t signal(void *handler) {
 currentprocess->signalhandler=handler;
 
 setlasterror(NO_ERROR);
 return(NO_ERROR);
}

size_t sendsignal(size_t process,size_t signal) {
PROCESS *next;
next=processes;
size_t thisprocess;
	
while(next != NULL) {
 if(next->pid == process) break;
 next=next->next;
}

if(next == NULL) {
 setlasterror(BAD_PROCESS);		/* invalid process */
 return(-1);
}

disable_interrupts();

thisprocess=getpid();	/* get current process */

loadpagetable(process);			/* switch to process address space */
currentprocess->signalhandler(signal);	/*call signal handler */

loadpagetable(thisprocess);		/* switch back to original address space */
enable_interrupts();

setlasterror(NO_ERROR);
return(0);
}

size_t set_critical_error_handler(void *addr) {
 currentprocess->errorhandler=addr;
 return;
}

size_t call_critical_error_handler(char *name,size_t drive,size_t flags,size_t error) {
 if(currentprocess == NULL) return(-1);
 if(currentprocess->errorhandler == NULL)  return(-1);

 return(currentprocess->errorhandler(name,drive,flags,error));
}

size_t processmanager_init(void) {
 processes=NULL;
 currentprocess=NULL;

 initialize_mutex(&process_mutex);
}


