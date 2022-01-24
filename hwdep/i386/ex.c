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

/*
 *
 * Exception handler
 *
 */

#include <stdint.h>
#include <stddef.h>
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
#define	BAD_TSS		10
#define SEGMENT_NOTPRESENT 11
#define STACK_FAULT	12
#define GPF		14
#define PAGE_FAULT	15
#define COPROCESSOR_FAULT 16
#define ALIGN_CHECK_EXCEPTION 17
#define MACHINE_CHECK_EXCEPTION 18

#define KERNEL_HIGH 1 << (sizeof(unsigned int)*8)-1

extern void exception(uint32_t *regs,uint32_t e,uint32_t dummy);
uint32_t tohex(uint32_t hex,char *buf);
uint32_t flags;
char *exp[] = { "Division by zero exception","Debug exception","Non maskable interrupt","Breakpoint exception", \
		"Into detected overflow","Out of bounds exception","Invalid opcode exception","No coprocessor exception", \
		 "Double Fault","Coprocessor segment overrun","Bad TSS","Segment not present","Stack fault", \
		 "General protection fault","Page fault","Unknown interrupt exception","Coprocessor fault", \
		 "Alignment check exception","Machine check exception" };

char *regnames[] = { "EIP=", "ESP=", "EAX=", "EBX=", "ECX=", "EDX=", "ESI=", "EDI=", "EBP=","" };

char *flagsname[]= { "","O=", " D="," I="," T="," S="," Z=",""," A=",""," P=",""," C=","$" };

extern void exception(uint32_t *regs,uint32_t e,uint32_t dummy) {
uint32_t count;
unsigned int handle;
uint32_t flagsmask;
char *b;
char *processname[MAX_PATH];

if(regs[0] >= KERNEL_HIGH) {
 kprintf("Kernel panic [%s] at address %X\n",exp[e],regs[0]);
}
else
{
 getprocessfilename(processname);
 
 kprintf("%s at address %X in %s\n",exp[e],regs[0],processname);  /* exception */
}

count=0;

do {							/* dump registers */
 if(regnames[count] == "") break;

 kprintf("%s=%X\n",regnames[count],regs[count]);

} while(regs[count++] != "");


flagsmask=2048;						/* mask to get flags */
count=0;

while(flagsname[count] != "$") {			/* dump flags */

 flagsmask=flagsmask >> 1;
 count++;

 if(flagsname[count] == "$") break;

 if(flagsname[count] == "") continue;			/* skip blank */

  kprintf(flagsname[count]);

  if(((uint32_t) regs[10] & flagsmask) == flagsmask) {			/* regbuf[10] is flags */
   kprintf("1");
  }
  else
  {
   kprintf("0");
  }
}


if(regs[0] >= KERNEL_HIGH) {
asm("xchg %bx,%bx");
// kprintf("\nThe system will now shut down\n");
 
// shutdown(0);
}
else
{
 kprintf("\n");
 asm("xchg %bx,%bx");
 kill(getpid());
 kprintf("Process terminated\n");
}

}

