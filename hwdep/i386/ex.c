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
    along with CCP.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
 *
 * Exception handler
 *
 */

#include <stdint.h>
#include <stddef.h>
#include "../../header/kernelhigh.h"
#include "hwdefs.h"
#include "../../filemanager/vfs.h"
#include "../../processmanager/process.h"

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

/*
 * x86 exception handler
 *
 * In: uint32_t *regs	Registers
       uint32_t e	Error number
       uint32_t dummy	Dummy value
 *
 * Returns nothing
 * 
 */

extern void exception(uint32_t *regs,uint32_t e,uint32_t dummy);
uint32_t tohex(uint32_t hex,char *buf);
uint32_t flags;
char *exp[] = { "Division by zero exception","Debug exception","Non maskable interrupt","Breakpoint exception", \
		"Into detected overflow","Out of bounds exception","Invalid opcode exception","No coprocessor exception", \
		 "Double Fault","Coprocessor segment overrun","Bad TSS","Segment not present","Stack fault", \
		 "General protection fault","Page fault","Unknown interrupt exception","Coprocessor fault", \
		 "Alignment check exception","Machine check exception" };

char *regnames[] = { "EIP=", "ESP=", "EAX=", "EBX=", "ECX=", "EDX=", "ESI=", "EDI=", "EBP=","" };

char *flagsname[]= { "","Overflow", " Direction"," Interrupt"," Trap"," Sign"," Zero",""," Adjust","","",""," Carry","$" };

extern void exception(uint32_t *regs,uint32_t e,uint32_t dummy) {
uint32_t count;
uint32_t flagsmask;
char *b;
char *processname[MAX_PATH];
uint32_t shiftcount;

if(regs[0] >= KERNEL_HIGH) {					/* kernel or module error */
	if(get_in_module_flag() == TRUE) {			/* module error */
		get_current_module_name(processname);

		kprintf_direct("%s at address %X in kernel module %s\n",exp[e],regs[0],processname);  /* exception */
	}
	else
	{
		kprintf_direct("Kernel panic [%s] at address %X\n",exp[e],regs[0]);
	}
}
else
{
	getprocessfilename(processname);
 
	kprintf_direct("%s at address %X in application %s\n",exp[e],regs[0],processname);  /* exception */
}

count=0;

do {							/* dump registers */
	if(regnames[count] == "") break;

	kprintf_direct("%s=%X\n",regnames[count],regs[count]);

} while(regs[count++] != "");


flagsmask=4096;						/* mask to get flags */
count=0;
shiftcount=12;

/* dump flags */

do {
	if(flagsname[count] == "$") break;

	if(flagsname[count] != "") {
		kprintf_direct("%s=%d ",flagsname[count],(((uint32_t) regs[10] & flagsmask) >> shiftcount));
	}

	flagsmask=flagsmask >> 1;
	shiftcount=shiftcount-1;
	count++;
} while(flagsname[count] != "$");

if(regs[0] >= KERNEL_HIGH) {
	if(get_in_module_flag() == TRUE) {			/* module error */
		return(-1);
	}

	asm("xchg %bx,%bx");
	kprintf_direct("\nThe system will now shut down\n");
	shutdown(0);
}
else
{
	kprintf_direct("\n");

	asm("xchg %bx,%bx");
	enable_interrupts();

	kill(getpid());
	kprintf_direct("Process terminated\n");
}

}

