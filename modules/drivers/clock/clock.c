/*  CCP Version 0.0.1
	   (C) Matthew Boote 2020-2023
7
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
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "clock.h"
#include "debug.h"

#define MODULE_INIT clock_init

uint16_t cmos_rtc_offsets[] = { 4,2,0,7,8,9,0x32,0xFFFF };		/* offsets to read time and date */

/*
 * Initialize RTC
 *
 * In:  init	Initialization string
 *
 * Returns: nothing
 *
 */

size_t clock_init(char *init) {
CHARACTERDEVICE device;

strncpy(&device.name,"CLOCK$",MAX_PATH);
device.charioread=&clockio_read;
device.chariowrite=&clockio_write;
device.ioctl=NULL;
device.flags=0;
device.data=NULL;
device.next=NULL;

if(add_character_device(&device) == -1) { 	/* add character device */
	kprintf_direct("clock: Can't register character device %s: %s\n",device.name,kstrerr(getlasterror()));
	return(-1);
}

setlasterror(NO_ERROR);
return(NO_ERROR);
}

/*
 * Clock I/O function
 *
 * In:  op	Operation (0=read,1=write)
	       	buf	Buffer
	ignored	This parameter is ignored
 *
 *  Returns: 0 on success or -1 on failure
 *
 */

size_t clock_io_internal(size_t op,void *buf) {
uint8_t *bufptr=buf;
size_t count=0;

DEBUG_PRINT_HEX(buf);

do {
	outb(CMOS_COMMAND_PORT,cmos_rtc_offsets[count]);	/* set read or write address in CMOS RAM */

	if(op == DEVICE_READ) {
		*bufptr=inb(CMOS_DATA_PORT);			/* read CMOS RAM data */

		kprintf_direct("%X: %X\n",cmos_rtc_offsets[count],*bufptr);
		bufptr++;

	//	asm("xchg %bx,%bx");
	}
	else
	{
		outb(CMOS_DATA_PORT,*bufptr++);			/* write CMOS RAM data */
	}

	count++;
} while(cmos_rtc_offsets[count] != 0xFFFF);


//asm("xchg %bx,%bx");

setlasterror(NO_ERROR);
return(CMOS_TIMEDATE_SIZE);
}

/*
 * Read RTC
 *
 * In:  buf	Buffer
	ignored	This parameter is ignored
 *
 *  Returns: 0 on success or -1 on failure
 *
 */
size_t clockio_read(void *buf,size_t ignored) {
	return(clock_io_internal(DEVICE_READ,buf));
}

/*
 * Write RTC
 *
 * In:  buf	Buffer
	ignored	This parameter is ignored
 *
 *  Returns: 0 on success or -1 on failure
 *
 */
size_t clockio_write(void *buf,size_t ignored) {
	return(clock_io_internal(DEVICE_WRITE,buf));
}

