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

#include "pit.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "../../../filemanager/vfs.h"

#define MODULE_INIT pit_init

void pit_init(char *init);
size_t pit_io(size_t op,size_t *buf,size_t ignored);
size_t pit_read(size_t size,size_t *buf);
size_t pit_write(size_t size,size_t *buf);

/*
 * Initialize PIT (Programmable Interval Timer)
 *
 * In:  char *init	Initialization string
 *
 * Returns: nothing
 *
 */
void pit_init(char *init) {
CHARACTERDEVICE bd;
uint32_t pit_val=PIT_VAL;

outb(0x43,0x34);
outb(0x40,PIT_VAL & 0xFF);
outb(0x40,((PIT_VAL >> 8) & 0xFF));

strncpy(bd.name,"TIMER",MAX_PATH);		/* add char device */

bd.charioread=&pit_read;
bd.chariowrite=&pit_write;
bd.ioctl=NULL;
bd.flags=0;
bd.data=NULL;
bd.next=NULL;

add_character_device(&bd); 
return;
}

/*
 * PIT I/O function
 *
 * In:  op	Operation (0=read,1=write)
        buf	Buffer
	len	Number of bytes to read/write
 *
 *  Returns: nothing
 *
 */

size_t pit_io(size_t op,size_t *buf,size_t ignored) {
size_t val;

if(op == PIT_READ) {
	outb(0x43,0x34);

	val=(inb(0x40) << 8)+inb(0x40);		/* read pit */ 
	*buf=val;

	return(0);
}

if(op == PIT_WRITE) {
	val=*buf;

	outb(0x43,0x34);
	outb(0x40,val & 0xFF);
	outb(0x40,((val >> 8) & 0xFF));

	return(0);
}
 
return(-1);
}

size_t pit_read(size_t size,size_t *buf) {
 return(_READ,buf,size);
}

size_t pit_write(size_t size,size_t *buf) {
 return(_READ,buf,size);
}

