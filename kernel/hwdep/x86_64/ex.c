/*  CCP Version 0.0.1
    (C) Matthew Boote 2020-2023

    This file is part of CCP.

    CCP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation,either version 3 of the License,or
    (at your option) any later version.

    CCP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CCP.  If not,see <https://www.gnu.org/licenses/>.
*/

/*
 *
 * Exception handler
 *
 */

#include <stdint.h>
#include <stddef.h>
#include "kernelhigh.h"
#include "hwdefs.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "memorymanager.h"
#include "process.h"
#include "string.h"
#include "debug.h"

#define DIVZERO 	0
#define DEBUGEX		1
#define NMI		2
#define BREAKPOINT	3
#define INTO_OVERFLOW	4
#define OOB		5
#define INVALID_OPCODE	6
#define NO_COPROCESSOR	7
#define DOUBLE_FAULT	8
#define COPROCESSORSEGOVERRRUN 9
#define	INVALID_TSS		10
#define SEGMENT_NOTPRESENT 11
#define STACK_FAULT	12
#define GPF		14
#define PAGE_FAULT	15
#define COPROCESSOR_FAULT 16
#define ALIGN_CHECK_EXCEPTION 17
#define MACHINE_CHECK_EXCEPTION 18

void enable_interrupts(void);
void exception(uint64_t *regs,uint64_t e);

/*
 * x86 exception handler
 *
 * In: regs	Registers
       e	Error number
       dummy	Dummy value
 *
 * Returns nothing
 * 
 */


uint64_t flags;
char *exp[] = { "Division by zero exception","Debug exception","Non maskable interrupt","Breakpoint exception",\
		"Into detected overflow","Out of bounds exception","Invalid opcode exception","No coprocessor exception",\
		 "Double Fault","Coprocessor segment overrun","Bad TSS","Segment not present","Stack fault",\
		 "General protection fault","Page fault","Unknown interrupt exception","Coprocessor fault",\
		 "Alignment check exception","Machine check exception" };

char *regnames[] = { "RIP","RSP","RAX","RBX","RCX","RDX","RSI","RDI","RBP","R10","R11","R12","R13","R14","R15",NULL};

char *flagsname[]= { "","Overflow"," Direction"," Interrupt"," Trap"," Sign"," Zero",""," Adjust","","",""," Carry","$" };

void exception(uint64_t *regs,uint64_t e) {
size_t count;
uint64_t flagsmask;
char *b;
char processname[MAX_PATH];
size_t shiftcount;
size_t rowcount;

if(regs[0] >= KERNEL_HIGH) {
	kprintf_direct("\nKernel panic [%s] at address %016lX\n",exp[e],regs[0]);
}
else
{
	getprocessfilename(processname);
 
	kprintf_direct("%s at address %016lX in %s\n",exp[e],regs[0],processname);  /* exception */
}

count=0;
rowcount=0;

while(regnames[count] != NULL) {							/* dump registers */
	if(regnames[count] == NULL) break;

	kprintf_direct("%s=%016lX ",regnames[count],regs[count]);

	if(++rowcount == 3) {				/* at end of row */
		kprintf_direct("\n");
		rowcount=0;
	}

	count++;
}

flagsmask=4096;						/* mask to get flags */
count=0;
shiftcount=12;

/* dump flags */

do {
	if(flagsname[count] == "$") break;

	if(flagsname[count] != "") kprintf_direct("%s=%d ",flagsname[count],(((uint64_t) regs[10] & flagsmask) >> shiftcount));
	
	flagsmask=flagsmask >> 1;
	shiftcount--;
	count++;

} while(flagsname[count] != "$");

asm("xchg %bx,%bx");
if(get_current_process_pointer() == NULL) {
	kprintf_direct("\nThe system will now shut down\n");
	shutdown(0);
}
else
{
	asm("xchg %bx,%bx");
	enable_interrupts();

	kill(getpid());
	kprintf_direct("\nProcess terminated\n");
}

}

