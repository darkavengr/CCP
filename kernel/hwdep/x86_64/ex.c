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
#include "../i386/ex.h"
#include "module.h"

void enable_interrupts(void);


uint64_t flags;
char *exceptions[] = { "Division by zero exception","Debug exception","Non maskable interrupt","Breakpoint exception","Into detected overflow",
		"Out of bounds exception","Invalid opcode exception","No coprocessor exception","Double Fault","Coprocessor segment overrun",
		"Bad TSS","Segment not present","Stack fault","General protection fault","Page fault","Unknown interrupt exception",
		"Coprocessor fault","Alignment check exception","Machine check exception","SIMD Floating-Point Exception",
		"Virtualization Exception","Control Protection Exception","Reserved #1","Reserved #2","Reserved #3","Reserved #4","Reserved #5",
		"Hypervisor Injection Exception","VMM Communication Exception","Security Exception","Reserved #5" };

char *registernames[] = { "RIP","RSP","RAX","RBX","RCX","RDX","RSI","RDI","RBP","R10","R11","R12","R13","R14","R15",NULL};

char *flagsnames[]= { NULL,"Overflow"," Direction"," Interrupt"," Trap"," Sign"," Zero",NULL," Adjust",NULL,NULL,NULL," Carry","$" };

char *ReadingWriting[] = { "Reading","Writing" };
char *KernelUserMode[] = { "kernel","user-mode" };

size_t count;
uint64_t flagsmask;
char processname[MAX_PATH];
size_t shiftcount;
size_t rowcount;
uint64_t faultaddress;
uint64_t stackbase;
size_t pd_entry;
KERNELMODULE *module;
char *FunctionName;

/*
 * x86_64 exception handler
 *
 * In: regs		Registers
       faultnumber	Error number
       faultinfo	Fault information
 *
 * Returns nothing
 * 
 */
void exception(uint64_t *regs,uint64_t faultnumber,uint64_t faultinfo) {
asm volatile ( "mov %%cr2, %0" : "=r"(faultaddress));	/* get fault address */

//if(faultnumber == PAGE_FAULT) {
	/* If there was a page fault, do possible demand paging */

	/* user-mode stack overflow */
//	if((faultaddress < KERNEL_HIGH) && 
//	   ((get_usermode_stack_base() != NULL) && (faultaddress < get_usermode_stack_base())) &&
//	   (faultaddress > GetUserHeapEnd())) {
//		faultaddress &= ~(PAGE_SIZE-1);		/* round down to PAGE_SIZE */

//		kprintf_direct("demand paging faultaddress=%X\n",faultaddress);
//		asm("xchg %bx,%bx");

//		alloc_int(ALLOC_NORMAL,PAGE_SIZE,faultaddress); /* extend stack downwards */
//		return;	
//	}
//}

/* If here, it's a fatal exception */

if(regs[0] >= KERNEL_HIGH) {		/* kernel or kernel module */
	module=FindModuleFromAddress(regs[0]);		/* get module associated with address */

	if(module != NULL) {		/* exception in module */
		kprintf_direct("\n%s at address %016X in kernel module %s",exceptions[faultnumber],regs[0],module->filename); 
	}
	else
	{
		kprintf_direct("\nKernel panic [%s] at address %016X",exceptions[faultnumber],regs[0]);
	}

	/* display function name */

	FunctionName=FindSymbolFromAddress(regs[0]);
	if(FunctionName != NULL) {		/* found function name */
		kprintf_direct(" in function %s:\n",FunctionName);
	}
	else
	{
		kprintf_direct(":\n");
	}
}
else					/* application exception */
{
	getprocessfilename(processname);
 
	kprintf_direct("\n%s at address %016X in %s:\n",exceptions[faultnumber],regs[0],processname);
}

if((faultnumber == DOUBLE_FAULT) || (faultnumber == INVALID_TSS) || (faultnumber == SEGMENT_NOT_PRESENT) || \
   (faultnumber == STACK_FAULT) || (faultnumber == GENERAL_PROTECTION_FAULT) || (faultnumber == ALIGN_CHECK)) {
	kprintf_direct("When accessing GDT selector %X\n\n",faultinfo);
}
else if(faultnumber == PAGE_FAULT) {
	kprintf_direct("%s %s page at address %X\n",ReadingWriting[(faultinfo & PAGEFAULT_WRITE) >> 1],KernelUserMode[(faultinfo & PAGEFAULT_USERMODE) >> 2],faultaddress);

	if(faultinfo & PAGEFAULT_INSTRUCTION_FETCH) kprintf_direct(" on instruction fetch");
	
	if(faultinfo & PAGEFAULT_RESERVEDBITS) {
		kprintf_direct(": Reserved bits overwritten\n\n");
	}
	else if(faultinfo & PAGEFAULT_PROTKEY) {
		kprintf_direct(": Protection key violation\n\n");
	}
	else if(faultinfo & PAGEFAULT_SHADOW_STACK) {
		kprintf_direct(": Shadow stack access violation\n\n");
	}
	else if(faultinfo & PAGEFAULT_SGX) {
		kprintf_direct(": Reserved bits overwritten\n\n");
	}
	else if((faultinfo & PAGEFAULT_PRESENT) == 0) {
		kprintf_direct(": Page not present\n\n");
	}
}

/* display registers */

count=0;
rowcount=0;

while(registernames[count] != NULL) {							/* dump registers */
	if(registernames[count] == NULL) break;

	kprintf_direct("%s=%016X ",registernames[count],regs[count]);

	if(++rowcount == 3) {				/* at end of row */
		kprintf_direct("\n");
		rowcount=0;
	}

	count++;
}

/* Dump CPU status flags */

kprintf_direct("\n\n");

flagsmask=4096;						/* mask to get flags */
count=0;
shiftcount=12;
rowcount=0;

/* dump flags */

do {
	if(flagsnames[count] == "$") break;

	if(flagsnames[count] != NULL) {
		kprintf_direct("%s=%d",flagsnames[count],(((uint64_t) regs[10] & flagsmask) >> shiftcount));
		rowcount++;

		if(rowcount == 5) {			/* at end of row */
			kprintf_direct("\n");
			rowcount=0;
		}
		else
		{
			kprintf_direct(" ");
		}
	}

flagsmask=flagsmask >> 1;
	shiftcount--;
	count++;

} while(flagsnames[count] != "$");

asm("xchg %bx,%bx");

if((get_current_process_pointer() == NULL) || (regs[0] >= KERNEL_HIGH)) {		/* kernel or kernel module */
	kprintf_direct("\nThe system will now shut down\n");
	shutdown(0);
}

enable_interrupts();

/* terminate process */
kill(getpid());
kprintf_direct("\nProcess terminated\n");

return;
}

