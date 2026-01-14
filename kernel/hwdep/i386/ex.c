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
#include "kernelhigh.h"
#include "hwdefs.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "memorymanager.h"
#include "process.h"
#include "string.h"
#include "ex.h"
#include "module.h"

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

void exception(uint32_t *regs,uint32_t faultnumber,uint32_t dummy);
uint32_t flags;

char *exceptions[] = { "Division by zero exception","Debug exception","Non maskable interrupt","Breakpoint exception", \
		"Into detected overflow","Out of bounds exception","Invalid opcode exception","No coprocessor exception", \
		 "Double Fault","Coprocessor segment overrun","Bad TSS","Segment not present","Stack fault", \
		 "General protection fault","Page fault","Unknown interrupt exception","Coprocessor fault", \
		 "Alignment check exception","Machine check exception" };

char *regnames[] = { "EIP", "ESP", "EAX", "EBX", "ECX", "EDX", "ESI", "EDI", "EBP","" };

char *flagsname[]= { NULL,"Overflow", "Direction","Interrupt","Trap","Sign","Zero",NULL,"Adjust",NULL,NULL,NULL,"Carry","$" };

size_t count;
uint32_t flagsmask;
char *b;
char *processname[MAX_PATH];
uint32_t shiftcount;
size_t rowcount;
size_t faultaddress;
KERNELMODULE *module;

void exception(uint32_t *regs,uint32_t faultnumber,uint32_t faultinfo) {
asm volatile ( "mov %%cr2, %0" : "=r"(faultaddress));	/* get fault address */

//if(e == PAGE_FAULT) {
	/* If there was a page fault, do possible demand paging */

	//if(faultaddress <= get_usermode_stack_base()) {		/* user-mode stack overflow */
	//	alloc_int(ALLOC_NORMAL,getpid(),get_usermode_stack_base()-faultaddress,faultaddress-(get_usermode_stack_base()-faultaddress)); /* extend stack downwards */
//		return;	
	//}
//}

/* If here, it's a fatal exception */

if(regs[0] >= KERNEL_HIGH) {		/* kernel or kernel module */
	module=FindModuleFromAddress(regs[0]);		/* get module associated with address */

	if(module != NULL) {		/* exception in module */
		kprintf_direct("%s at address %08X in module %s:\n",exceptions[faultnumber],regs[0],module->filename); 
	}
	else
	{
		kprintf_direct("\nKernel panic [%s] at address %08X:\n",exceptions[faultnumber],regs[0]);
	}
}
else			/* application */
{
	getprocessfilename(processname);
 
	kprintf_direct("%s at address %08X in %s:\n",exceptions[faultnumber],regs[0],processname);  /* exception */
}

if((faultnumber == DOUBLE_FAULT) || (faultnumber == INVALID_TSS) || (faultnumber == SEGMENT_NOT_PRESENT) || \
   (faultnumber == STACK_FAULT) || (faultnumber == GENERAL_PROTECTION_FAULT) || (faultnumber == ALIGN_CHECK)) {
	kprintf_direct("When accessing GDT selector %X\n\n",faultinfo);
}
else if(faultnumber == PAGE_FAULT) {
	if(faultinfo & PAGEFAULT_WRITE) {
		kprintf_direct("Error writing page at address %X",faultaddress);
	}
	else
	{
		kprintf_direct("Error reading page at address %X",faultaddress);
	}

	if(faultinfo & PAGEFAULT_USERMODE) {		/* user mode */
		kprintf_direct(" in user-mode");
	}
	else
	{
		kprintf_direct(" in kernel mode");
	}

	if(faultinfo & PAGEFAULT_INSTRUCTION_FETCH) kprintf_direct(" on instruction fetch");
	
	else if(faultinfo & PAGEFAULT_RESERVEDBITS) {
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
	else
	{
		kprintf_direct("\n");
	}
}

count=0;
rowcount=0;

do {							/* dump registers */
	if(regnames[count] == "") break;

	kprintf_direct("%s=%08X ",regnames[count],regs[count]);

	if(++rowcount == 4) {			/* at end of row */
		kprintf_direct("\n");
		rowcount=0;
	}

} while(regs[count++] != "");


/* Dump CPU status flags */

kprintf_direct("\n\n");

flagsmask=4096;						/* mask to get flags */
count=0;
shiftcount=12;
rowcount=0;

/* dump flags */

do {
	if(flagsname[count] == "$") break;

	if(flagsname[count] != NULL) {
		kprintf_direct("%s=%d",flagsname[count],(((uint32_t) regs[10] & flagsmask) >> shiftcount));
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

} while(flagsname[count] != "$");

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

