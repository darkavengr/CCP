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

#include <stdint.h>
#include <stddef.h>
#include "../../filemanager/vfs.h"
#include "../../processmanager/process.h"

size_t tickcount=0;
unsigned int multitaskingenabled;

extern PROCESS *currentprocess;
extern PROCESS *processes;

void switch_task(size_t *regs);
void disablemultitasking(void);
void enablemultitasking(void);
size_t gettickcount(void);
void init_multitasking(void);

#define X86_NUMBER_OF_REGISTERS		11		// 8 registers saved by pusha and 3 by the interrupt

/* switch task 

 This is called by a function which is expected to save the registers onto
 the stack, call switch_task and restore the new registers from the stack,
 the calling function will then restore the registers and return, transferring control to the
 new process

 You are not expected to understand this */

void switch_task(size_t *regs) {

 tickcount++;

 if(multitaskingenabled == FALSE) {				/* return if multiasking is disabled */
  return;
 }

 if(processes == NULL || currentprocess == NULL) {			/* no processes */
  return;
 }

 if(++currentprocess->ticks < currentprocess->maxticks) {		/* return if process timeslot not complete */
  return;
 }

// asm(".intel_syntax");
// asm("inc byte ptr [0x800b8000]");
// asm(".att_syntax");

 currentprocess->ticks=currentprocess->maxticks;
 memcpy(&currentprocess->regs,regs,X86_NUMBER_OF_REGISTERS*sizeof(size_t));			/* save registers */

 currentprocess=currentprocess->next;				/* point to next process */
 if(currentprocess == NULL) currentprocess=processes;		/* if at end, loop back to start */

 loadpagetable(currentprocess->pid);				/* load page tables */

 switch_kernel_stack(currentprocess->kernelstackpointer);	/* switch kernel stack */

// if(currentprocess->pid == 1) {
//  asm("xchg %bx,%bx");
// }
 
 memcpy(regs,&currentprocess->regs,X86_NUMBER_OF_REGISTERS*sizeof(size_t));		/* load registers */
 return;
}

void disablemultitasking(void) {
 multitaskingenabled=FALSE;
 return;
}

void enablemultitasking(void) {
 multitaskingenabled=TRUE;
 return;
}

size_t gettickcount(void) {
 return(tickcount);
}


void init_multitasking(void) {
tickcount=0;

setirqhandler(0,&switch_task);
return;
}

void kwait(unsigned int period) {
 unsigned int de=gettickcount()+period;

 while(gettickcount() < de) ;;
}

