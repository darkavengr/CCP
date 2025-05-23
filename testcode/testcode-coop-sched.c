/*  CCP Version 0.0.1
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
    along with CCP.  If not, see <https:www.gnu.org/licenses/>.
*/

/*
 *
 * C C P - Computer control program
 *
 */
#include <stdint.h>
#include <stddef.h>
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "memorymanager.h"
#include "process.h"
#include "debug.h"
#include "string.h"

extern PROCESS *processes;
extern PROCESS *currentprocess;
extern size_t highest_pid_used;
extern PROCESS *processes_end;
extern end_yield;

void kernel_thread1(void) {
while(1) {
	kprintf_direct("1");

	yield();
}

}

void kernel_thread2(void) {
while(1) {
	kprintf_direct("2");

	yield();
}

}

void CreateKernelTask(void *address) {
PROCESS *next;
size_t *stackinit;

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

next->flags=0;
next->ticks=0;
next->maxticks=DEFAULT_QUANTUM_COUNT;
next->pid=highest_pid_used;
next->kernelstacktop=kernelalloc(PROCESS_STACK_SIZE);
next->kernelstacktop += PROCESS_STACK_SIZE;
next->next=NULL;

stackinit=next->kernelstacktop-(12*sizeof(size_t));
next->kernelstackpointer=stackinit;

*stackinit=&end_yield;				/* return address */
*++stackinit=0xFFFFFFFF;			/* edi */
*++stackinit=0xEEEEEEEE;			/* esi */	
*++stackinit=next->kernelstacktop-PROCESS_STACK_SIZE;	/* ebp */
*++stackinit=next->kernelstacktop;		/* esp */
*++stackinit=0xDDDDDDDD;			/* ebx */
*++stackinit=0xCCCCCCCC;			/* edx */
*++stackinit=0xBBBBBBBB;			/* ecx */
*++stackinit=0xAAAAAAAA;			/* eax */
*++stackinit=address;				/* interrupt EIP */
*++stackinit=0x8;				/* interrupt CS */
*++stackinit=0x200;				/* interrupt EFLAGS */


highest_pid_used++;

processes_end=next;
}

void kernel(void) {
size_t (*init_task)(void);

disablemultitasking(); 
disable_interrupts();

CreateKernelTask(&kernel_thread1);
CreateKernelTask(&kernel_thread2);

/* add process to process list and find process id */

currentprocess=processes;

asm volatile("mov %0, %%esp":: "a"(currentprocess->kernelstackpointer));

init_task=&kernel_thread1;

//enable_interrupts();
enablemultitasking();

init_task();
}
