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

#include "pit.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"

#define MODULE_INIT pit_init

void pit_init(char *init);
unsigned int pit_io(unsigned int op,unsigned int *buf,unsigned int ignored);

void pit_init(char *init) {
CHARACTERDEVICE bd;
uint32_t pit_val=PIT_VAL;

outb(0x43,0x34);
outb(0x40,PIT_VAL & 0xFF);
outb(0x40,((PIT_VAL >> 8) & 0xFF));

strcpy(bd.dname,"TIMER");		/* add char device */
bd.chario=&pit_io;
bd.ioctl=NULL;
bd.flags=0;
bd.data=NULL;
bd.next=NULL;

add_char_device(&bd); 
return;
}

unsigned int pit_io(unsigned int op,unsigned int *buf,unsigned int ignored) {
unsigned int val;

if(op == PIT_READ) {
  outb(0x43,0x34);

  val=(inb(0x40) << 8)+inb(0x40);		/* read pit */ 
  *buf=val;

  return;
}

if(op == PIT_WRITE) {
 val=*buf;

 outb(0x43,0x34);
 outb(0x40,val & 0xFF);
 outb(0x40,((val >> 8) & 0xFF));

 return;
}
 
return(-1);
}

