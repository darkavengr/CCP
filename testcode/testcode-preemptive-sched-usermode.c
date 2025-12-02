#include <stdint.h>
#include <stddef.h>
#include "kernelhigh.h"
#include "pagesize.h"
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "memorymanager.h"
#include "process.h"
#include "signal.h"
#include "bootinfo.h"
#include "debug.h"
#include "version.h"
#include "string.h"
#include "pagesize.h"

#define ENTRY_POINT 0x100

extern PROCESS *processes;
extern PROCESS *currentprocess;
extern size_t highest_pid_used;

PROCESS *processes_end;
size_t *stackinit;

uint8_t thread1[] = { 0xB4,0x9,0xBA,0xB,0x1,0x0,0x0,0xCD,0x21,0xEB,0xF5,0x31,0x0 };
uint8_t thread2[] = { 0xB4,0x9,0xBA,0xB,0x1,0x0,0x0,0xCD,0x21,0xEB,0xF5,0x32,0x0 };

void CreateUserTask(void *address) {
void *kernelstackbase;
PROCESS *next;
PROCESS *oldprocess;
size_t stackp;
PROCESS *lastprocess;
PSP *psp=NULL;
char *fullpath[MAX_PATH];
size_t *stackptr;

oldprocess=currentprocess;					/* save current process pointer */

/* add process to process list and find process ID */

if(processes == NULL) {  					/* first process */
	processes=kernelalloc(sizeof(PROCESS));			/* add entry to beginning of list */

	if(processes == NULL) {					/* return if can't allocate */
		switch_address_space(getppid());
		freepages(highest_pid_used);	

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
		switch_address_space(getppid());
		freepages(highest_pid_used);

		setlasterror(NO_MEM);
		return(-1);
	}

	lastprocess=processes_end;			/* save current end */
	next=processes_end->next;		/* get a copy of end, so that the list is not affected if any errors occur */
}

ksnprintf(next->filename,"Task %d",MAX_PATH,highest_pid_used);

next->maxticks=DEFAULT_QUANTUM_COUNT;	/* maximum number of ticks for this process */
next->ticks=0;				/* start at 0 for number of ticks */
next->parentprocess=highest_pid_used;		/* parent process ID */
next->pid=highest_pid_used;		/* process ID */
next->errorhandler=NULL;		/* error handler, NULL for now */
next->signalhandler=NULL;		/* signal handle, NULL for now */
next->childprocessreturncode=0;		/* return code from child process */
next->flags=0;				/* process flags */
next->lasterror=0;			/* last error */
next->next=NULL;

/* Create kernel stack for process */

next->kernelstacktop=kernelalloc(PROCESS_STACK_SIZE);	/* allocate stack */
if(next->kernelstacktop == NULL) {	/* return if unable to allocate */
	currentprocess=oldprocess;	/* restore current process pointer */

	switch_address_space(highest_pid_used);
	freepages(highest_pid_used);

	kernelfree(lastprocess->next);	/* remove process from list */

	lastprocess->next=NULL;		/* remove process */
	processes_end=lastprocess;
	return(-1);
}

next->kernelstackbase=next->kernelstacktop;
next->kernelstacktop += PROCESS_STACK_SIZE;			/* top of kernel stack */

currentprocess=next;		/* kludge */

initialize_current_process_kernel_stack(next->kernelstacktop,ENTRY_POINT,next->kernelstacktop-PROCESS_STACK_SIZE); /* initialize kernel stack */

page_init(highest_pid_used);				/* intialize page directory */	
switch_address_space(highest_pid_used);			/* load page table */

/* allocate user mode stack below enviroment variables */

stackp=alloc_int(ALLOC_NORMAL,highest_pid_used,PROCESS_STACK_SIZE,(END_OF_LOWER_HALF-PROCESS_STACK_SIZE-ENVIROMENT_SIZE));

if(stackp == NULL) {
	currentprocess=oldprocess;	/* restore current process pointer */
	kernelfree(next->kernelstacktop);	/* free kernel stack */

	if(lastprocess->next != NULL) kernelfree(lastprocess->next);	/* remove process from list */

	lastprocess->next=NULL;		/* remove process */
	processes_end=lastprocess;

	switch_address_space(highest_pid_used);
	freepages(highest_pid_used);
	return(-1);
}

next->stackbase=stackp;					/* add user mode stack base and pointer to new process entry */
next->stackpointer=(stackp+PROCESS_STACK_SIZE)-PAGE_SIZE;

processes_end=next;						/* save last process address */

alloc_int(ALLOC_NORMAL,highest_pid_used,PAGE_SIZE,0);		/* allocate page at address 0 */

memcpy(ENTRY_POINT,address,PAGE_SIZE);				/* copy code */

highest_pid_used++;

return(0);
}

void kernel(void) {
PROCESS *next;

disablemultitasking(); 
disable_interrupts();

CreateUserTask(&thread1);
CreateUserTask(&thread2);

next=processes;

while(next != NULL) {
	DEBUG_PRINT_DEC(next->pid);

	next=next->next;
}

/* initialize process 0 */

currentprocess=processes;

switch_address_space(0);

initialize_current_process_user_mode_stack(processes->stackpointer,PROCESS_STACK_SIZE);	/* intialize and switch to user mode stack */

enablemultitasking(); 

switch_to_usermode_and_call_process(ENTRY_POINT);		/* switch to user mode, enable interrupts, and call process */
}

