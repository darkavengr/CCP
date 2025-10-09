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

#include <stdint.h>
#include <stddef.h>
#include "pit.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "string.h"

#define MODULE_INIT pit_init

/*
 * Initialize PIT (Programmable Interval Timer)
 *
 * In:  char *init	Initialization string
 *
 * Returns: nothing
 *
 */
size_t pit_init(char *init) {
CHARACTERDEVICE device;

outb(PIT_COMMAND_REGISTER,0x34);				/* set PIT timer interval */
outb(PIT_CHANNEL_0_REGISTER,PIT_VAL & 0xFF);
outb(PIT_CHANNEL_0_REGISTER,((PIT_VAL >> 8) & 0xFF));

strncpy(device.name,"TIMER",MAX_PATH);
device.charioread=&pit_read;
device.chariowrite=&pit_write;
device.ioctl=NULL;
device.flags=0;
device.data=NULL;
device.next=NULL;

if(add_character_device(&device) == -1) {	/* add character device */
	kprintf_direct("pit: Can't register character device %s: %s\n",device.name,kstrerr(getlasterror()));
	return(-1);
}

return(0);
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

if(op == DEVICE_READ) {
	outb(PIT_COMMAND_REGISTER,0x34);

	val=(inb(PIT_CHANNEL_0_REGISTER) << 8)+inb(PIT_CHANNEL_0_REGISTER);		/* read PIT */ 
	*buf=val;

	return(0);
}
else if(op == DEVICE_WRITE) {
	val=*buf;

	outb(PIT_COMMAND_REGISTER,0x34);
	outb(PIT_CHANNEL_0_REGISTER,val & 0xFF);	/* send low byte */
	outb(PIT_CHANNEL_0_REGISTER,((val >> 8) & 0xFF));	/* send high byte */

	return(0);
}

setlasterror(INVALID_PARAMETER);
return(-1);
}

size_t pit_read(size_t size,size_t *buf) {
 return(pit_io(DEVICE_READ,buf,size));
}

size_t pit_write(size_t size,size_t *buf) {
 return(pit_io(DEVICE_WRITE,buf,size));
}

