/*  CCP Version 0.0.1
    (C) Matthew Boote 2020-2023-2022

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
#include "pic.h"
#include "string.h"
#include "idtflags.h"
#include "kernelselectors.h"

#define MODULE_INIT pic_init

extern irq0();
extern irq1();
extern irq2();
extern irq3();
extern irq4();
extern irq5();
extern irq6();
extern irq7();
extern irq8();
extern irq9();
extern irq10();
extern irq11();
extern irq12();
extern irq13();
extern irq14();
extern irq15();

/*
 * Initialize PIC
 *
 * In: nothing
 *
 *  Returns: nothing
 *
 */
void pic_init(void) {
disable_interrupts();

set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq0,0xf0);		/* set interrupts */
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq1,0xf1);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq2,0xf2);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq3,0xf3);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq4,0xf4);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq5,0xf5);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq6,0xf6);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq7,0xf7);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq8,0xf8);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq9,0xf9);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq10,0xfa);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq11,0xfb);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq12,0xfc);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq13,0xfd);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq14,0xfe);
set_interrupt(IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE,KERNEL_CODE_SELECTOR,&irq15,0xff);

/* remap IRQs
 the default IRQ interrupts for the master pic conflict with interrupt 0-8 exceptions
 so they must be moved to 0xF0 - 0xFF */

outb(PIC_MASTER_COMMAND,ICW1_INIT | ICW1_SINGLE);	/* remap IRQ */
outb(PIC_SLAVE_COMMAND,ICW1_INIT | ICW1_SINGLE);

outb(PIC_MASTER_DATA,0xf0);				/* remap master PIC IRQs to interrupts F0-F7 */
outb(PIC_SLAVE_DATA,0xf8);			/* remap slave PIC IRQs to interrupts F8-FF */

outb(PIC_MASTER_DATA,4);
outb(PIC_SLAVE_DATA,2);
outb(PIC_MASTER_DATA,ICW4_8086);
outb(PIC_SLAVE_DATA,ICW4_8086);

outb(PIC_MASTER_DATA,0);	/* enable all IRQs */
outb(PIC_SLAVE_DATA,0);	

enable_interrupts();

return;
}

/*
 * Disable IRQ
 *
 * In: irqnumber	IRQ number
 *
 *  Returns: nothing
 *
 */
void disableirq(size_t irqnumber) {
if(irqnumber < 8) {				/* if master */
	outb(PIC_MASTER_DATA, (inb(PIC_MASTER_DATA) & ~(1 << irqnumber)) );	/* mask IRQ bit */
}
else						/* if slave */
{
	outb(PIC_SLAVE_DATA, (inb(PIC_SLAVE_DATA) & ~(1 << (irqnumber-8))) );	/* mask IRQ bit */
}

}

/*
 * Enable IRQ
 *
 * In: irqnumber	IRQ number
 *
 *  Returns: nothing
 *
 */
void enableirq(size_t irqnumber) {
if(irqnumber < 8) {				/* if master */
	outb(PIC_MASTER_DATA, (inb(PIC_MASTER_DATA) | (1 << irqnumber)) );	/* mask IRQ bit */
}
else						/* if slave */
{
	outb(PIC_SLAVE_DATA, (inb(PIC_SLAVE_DATA) | (1 << (irqnumber-8))) );	/* set IRQ bit */
}

}

