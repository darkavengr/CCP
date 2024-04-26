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

#define MODULE_INIT pic_init

void *irq_handlers[15];

extern irq0;
extern irq1;
extern irq2;
extern irq3;
extern irq4;
extern irq5;
extern irq6;
extern irq7;
extern irq8;
extern irq9;
extern irq10;
extern irq11;
extern irq12;
extern irq13;
extern irq14;
extern irq15;

/*
 * Initialize PIC
 *
 * In: nothing
 *
 *  Returns nothing
 *
 */
void pic_init(void) {
disable_interrupts();

set_interrupt(0x8e,8,&irq0,0xf0);		/* set interrupts */
set_interrupt(0x8e,8,&irq1,0xf1);
set_interrupt(0x8e,8,&irq2,0xf2);
set_interrupt(0x8e,8,&irq3,0xf3);
set_interrupt(0x8e,8,&irq4,0xf4);
set_interrupt(0x8e,8,&irq5,0xf5);
set_interrupt(0x8e,8,&irq6,0xf6);
set_interrupt(0x8e,8,&irq7,0xf7);
set_interrupt(0x8e,8,&irq8,0xf8);
set_interrupt(0x8e,8,&irq9,0xf9);
set_interrupt(0x8e,8,&irq10,0xfa);
set_interrupt(0x8e,8,&irq11,0xfb);
set_interrupt(0x8e,8,&irq12,0xfc);
set_interrupt(0x8e,8,&irq13,0xfd);
set_interrupt(0x8e,8,&irq14,0xfe);
set_interrupt(0x8e,8,&irq15,0xff);

/* remap irqs
 the default irq interrupts for the master pic conflict with interrupt 0-8 exceptions
 so they must be moved to 0xf0 - 0xff */

outb(PIC_MASTER_COMMAND,ICW1_INIT | ICW1_SINGLE);	/* remap irq */
outb(PIC_SLAVE_COMMAND,ICW1_INIT | ICW1_SINGLE);

outb(PIC_MASTER_DATA,0xf0);				/* remap master pic irqs to interrupts f0-f7 */
outb(PIC_SLAVE_DATA,0xf8);			/* remap slave pic irqs to interrupts f8-ff */

outb(PIC_MASTER_DATA,4);
outb(PIC_SLAVE_DATA,2);
outb(PIC_MASTER_DATA,ICW4_8086);
outb(PIC_SLAVE_DATA,ICW4_8086);

outb(PIC_MASTER_DATA,0);	/* enable all irqs */
outb(PIC_SLAVE_DATA,0);	

enable_interrupts();

return;
}

/*
 * Set IRQ handler
 *
 * In: size_t irqnumber	IRQ number
       void *handler		IRQ handler
 *
 *  Returns nothing
 *
 */
void setirqhandler(size_t irqnumber,void *handler);
void pic_init(void);

void setirqhandler(size_t irqnumber,void *handler) {
irq_handlers[irqnumber]=handler;
return;
}

//void disable_timer() {
//uint8_t pic_data;

//pic_data=inb(
