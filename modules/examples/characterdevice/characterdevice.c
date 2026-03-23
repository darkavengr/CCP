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
#include "characterdevice.h"
#include "errors.h"

#define MODULE_INIT example_chardevice_init

/*
 * Function name
 *
 * In: Parameters	Description
 *
 *  Returns: Return parameters
 *
 */

size_t example_chardevice_init(char *initstr) {
CHARACTERDEVICE device;

/* device information */
strncpy(&device.name,"EXAMPLE_CHAR_DEVICE",MAX_PATH);		/* device name */
device.charioread=&example_chardevice_read;			/* read handler */
device.chariowrite=example_chardevice_write;			/* write handler */
device.ioctl=example_chardevice_ioctl;				/* ioctl() handler */
device.flags=0;							/* device flags */
device.data=NULL;						/* pointer to device information */
device.next=NULL;

if(add_character_device(&device) == -1) { 	/* add character device */
	kprintf_direct("example_chardevice: Can't register character device %s: %s\n",device.name,kstrerr(getlasterror()));
	return(-1);
}

setlasterror(NO_ERROR);
return(NO_ERROR);

return;
}

/*
 * Read device
 *
 * In:  buf	Buffer
	size	Number of bytes to read
 *
 *  Returns: 0 on success or -1 on failure
 *
 */
size_t example_chardevice_read(void *buf,size_t size) {
/* read device code here */

return(0);
}

/*
 * Write device
 *
 * In:  buf	Buffer
	size	Number of bytes to write
 *
 *  Returns: 0 on success or -1 on failure
 *
 */
size_t example_chardevice_write(void *buf,size_t size) {
/* write device code here */

return(0);
}

/*
 * ioctl() handler
 *
 * In: size_t handler		Handle used to for port (was opened with open());
	      unsigned long request	Request number
	      char *buffer		Buffer
 *
 *  Returns 0 on success, -1 on error
 *
 */
size_t example_chardevice_ioctl(size_t handle,unsigned long request,char *buffer) {
/* ioctl() device code here */
}

