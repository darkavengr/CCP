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
#include "version.h"
#include "string.h"

extern PROCESS *processes;
extern PROCESS *currentprocess;
PROCESS *processes_end;
extern size_t highest_pid_used;
size_t *stackinit;

extern irq_exit;

void kernel_thread1(void) {

while(1) {
	kprintf_direct("1");
}

}

void kernel_thread2(void) {

while(1) {
	kprintf_direct("2");
}

}

void CreateKernelTask(void *address) {
PROCESS *next;

/* add process to process list and find process id */

if(processes == NULL) {  
	processes=kernelalloc(sizeof(PROCESS));

	if(processes == NULL) return(-1);

	processes_end=processes;
	next=processes;
}
else
{
	processes_end->next=kernelalloc(sizeof(PROCESS));
	if(processes_end->next == NULL)	return(-1);

	next=processes_end->next;
}

next->maxticks=DEFAULT_QUANTUM_COUNT;
next->pid=highest_pid_used;
next->kernelstacktop=kernelalloc(PROCESS_STACK_SIZE);
next->kernelstacktop += PROCESS_STACK_SIZE;
next->next=NULL;

stackinit=next->kernelstacktop-(12*sizeof(size_t));
next->kernelstackpointer=stackinit;

*stackinit=&irq_exit;
*++stackinit=0xFFFFFFFF;
*++stackinit=0xEEEEEEEE;
*++stackinit=0xDDDDDDDD;
*++stackinit=0xCCCCCCCC;
*++stackinit=next->kernelstacktop;
*++stackinit=next->kernelstacktop-PROCESS_STACK_SIZE;
*++stackinit=0xBBBBBBBB;
*++stackinit=0xAAAAAAAA;
*++stackinit=address;
*++stackinit=0x8;
*++stackinit=0x200;

highest_pid_used++;

processes_end=next;
}

void kernel(void) {
size_t (*init_task)(void);
PROCESS *next;

disablemultitasking(); 
disable_interrupts();

CreateKernelTask(&kernel_thread1);
CreateKernelTask(&kernel_thread2);

/* add process to process list and find process id */

currentprocess=NULL;
init_task=&kernel_thread1;

enablemultitasking(); 
enable_interrupts();

init_task();
}

