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
#include "command.h"
#include "../header/errors.h"
#include "../processmanager/mutex.h"
#include "../devicemanager/device.h"
#include "../filemanager/vfs.h"

/*
 * Read line from console
 *
 * In:  unsigned long handle	Handle
	char *buf		Buffer
        int size		Buffer size
 *
 * Returns nothing
 */

unsigned long readline(unsigned long handle,char *buf,int size) {
char *bufptr;
int count=0;

bufptr=buf;				/* point to buffer */

while(1) { 
	if(read(stdin,bufptr,1) == -1) {
		kprintf("command: Error reading stdin\n");
		return(-1);
	}


	if(count++ >= size) return;

	if(*bufptr == 0x8) {			/* if backspace, erase character */
		*bufptr=0;
		*--bufptr=0;
		count--;
	}

	if(*bufptr == '\n') {			/* if newline, return */
		*bufptr=0;
		break;
	}

	bufptr++;
}

return(NO_ERROR);
}

