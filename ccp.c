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
#include "header/errors.h"
#include "filemanager/vfs.h"
#include "processmanager/process.h"
#include "header/debug.h"
#include "modules/filesystems/fat/fat.h"

/*
 * High-level kernel initalization
 *
 * In: nothing
 *
 * Returns nothing
 */


extern PROCESS *processes;
extern PROCESS *currentprocess;
PROCESS *processes_end;
extern size_t highest_pid_used;
size_t *stackinit;

extern irq_exit;


void kernel_thread1(void) {
enablemultitasking(); 
enable_interrupts();

while(1) {
	kprintf_direct("1");
}

}

void kernel_thread2(void) {
enablemultitasking(); 
enable_interrupts();

while(1) {
	kprintf_direct("2");
}

}

void kernel_thread3(void) {
enablemultitasking(); 
enable_interrupts();

while(1) {
	kprintf_direct("3");
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
//size_t (*init_task)(void);
//PROCESS *next;

//disablemultitasking(); 
//disable_interrupts();

//CreateKernelTask(&kernel_thread1);
//CreateKernelTask(&kernel_thread2);
//CreateKernelTask(&kernel_thread3);

/* add process to process list and find process id */

//currentprocess=processes;
//init_task=&kernel_thread1;
//init_task();

if(exec("\\COMMAND.RUN","/P /K \\AUTOEXEC.BAT",FALSE) ==  -1) {
	kprintf_direct("Missing or corrupt command interpreter, system halted (%d)",getlasterror());

	asm("xchg %bx,%bx");
	halt();
	while(1) ;;
}

asm("xchg %bx,%bx");
}

