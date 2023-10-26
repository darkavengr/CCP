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
#include "../../header/kernelhigh.h"
#include "hwdefs.h"
#include "../../devicemanager/device.h"
#include "../../filemanager/fat.h"
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

extern void exception(uint64_t addr,uint64_t e,uint64_t dummy);
uint64_t tohex(uint64_t hex,char *buf);
char *exp[] = { "Division by zero exception ","Debug exception ","Non maskable interrupt ","Breakpoint exception ", \
		"Into detected overflow","Out of bounds exception ","Invalid opcode exception ","No coprocessor exception ", \
		 "Double Fault ","Coprocessor segment overrun ","Bad TSS ","Segment not present ","Stack fault ", \
		 "General protection fault ","Page fault ","Unknown interrupt exception ","Coprocessor fault", \
		 "Alignment check exception ","Machine check exception" };

char *regs[] = { "RIP=", "RSP=", "RAX=", "RBX=", "RCX=", "RDX=", "RSI=", "RDI=", "RBP=",\ 
		 "R8=","R9=","R10=","R11=","R12=","R13=","R14=","R15=","R16=","" };

char *flagsname[]= { "","O=", " D="," I="," T="," S="," Z=",""," A=",""," P=",""," C=","$" };


extern void exception(uint64_t *regs,uint64_t e,uint64_t dummy) {
int count;
uint64_t flagsmask;
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

  if(((uint64_t) regs[10] & flagsmask) == flagsmask) {			/* regbuf[10] is flags */
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


size_t tohex64(uint64_t hex,char *buf) {
size_t count;
uint64_t h;
uint64_t shiftamount;
uint64_t mask;

char *b;
char *hexbuf[] = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"};

mask=0xF000000000000000;
shiftamount=60;

b=buf;

for(count=1;count<16;count++) {
 h=((hex & mask) >> shiftamount);	/* shift nibbles to end */

 shiftamount=shiftamount-4;
 mask=mask >> 4;  
 strcpy(b++,hexbuf[h]);
}

 return(NULL);
}

