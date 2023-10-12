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

/*
 * High-level kernel initalization
 *
 * In: nothing
 *
 * Returns nothing
 */

/*
extern PROCESS *processes;
extern PROCESS *currentprocess;
extern irq_exit;

void kernel_thread1(void) {
enablemultitasking(); 
enable_interrupts();

while(1) {

	asm(".intel_syntax noprefix");
	asm("inc byte ptr [0x800b8000]");
	asm(".att_syntax prefix");
}

}

void kernel_thread2(void) {
enablemultitasking(); 
enable_interrupts();

while(1) {
	asm(".intel_syntax noprefix");
	asm("inc byte ptr [0x800b8002]");
	asm(".att_syntax prefix");
}

}

*/
void kernel(void) {

/*
size_t *stackinit;
PROCESS *next;
size_t (*init_task)(void);

disablemultitasking(); 
disable_interrupts();

processes=kernelalloc(sizeof(PROCESS));

processes->maxticks=DEFAULT_QUANTUM_COUNT;
processes->maxticks=DEFAULT_QUANTUM_COUNT;
processes->next=NULL;
next->parentprocess=0;
next->pid=0;

processes->kernelstacktop=kernelalloc(PROCESS_STACK_SIZE);
processes->kernelstacktop += PROCESS_STACK_SIZE;

DEBUG_PRINT_HEX(processes->kernelstacktop);

stackinit=processes->kernelstacktop-(12*sizeof(size_t));
processes->kernelstackpointer=stackinit;

*stackinit=&irq_exit;
*++stackinit=0x11111111;
*++stackinit=0x22222222;
*++stackinit=0x33333333;
*++stackinit=0x44444444;
*++stackinit=processes->kernelstacktop;
*++stackinit=processes->kernelstacktop-PROCESS_STACK_SIZE;
*++stackinit=0x55555555;
*++stackinit=0x66666666;
*++stackinit=&kernel_thread2;
*++stackinit=0x8;
*++stackinit=0x200;

processes->next=kernelalloc(sizeof(PROCESS));
next=processes->next;

next->maxticks=DEFAULT_QUANTUM_COUNT;
next->next=NULL;
next->pid=1;

next->kernelstacktop=kernelalloc(PROCESS_STACK_SIZE);
next->kernelstacktop += PROCESS_STACK_SIZE;

DEBUG_PRINT_HEX(next->kernelstacktop);

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
*++stackinit=&kernel_thread2;
*++stackinit=0x8;
*++stackinit=0x200;

currentprocess=processes;

init_task=&kernel_thread1;
init_task();

*/

if(exec("\\COMMAND.RUN","/P /K \\AUTOEXEC.BAT",FALSE) ==  -1) { /* can't run command interpreter */
	kprintf_direct("Missing or corrupt command interpreter, system halted (%d)",getlasterror());
	halt();
	while(1) ;;
}

}

