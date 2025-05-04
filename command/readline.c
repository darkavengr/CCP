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
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "string.h"

/*
 * Read line from console
 *
 * In:  unsigned long handle	Handle
	char *buf		Buffer
        int size		Buffer size
 *
 * Returns buffer size
 */

unsigned long readline(unsigned long handle,char *buf,int size) {
char *bufptr;
int count=0;

bufptr=buf;				/* point to buffer */

while(1) { 
	if(read(stdin,bufptr,1) == -1) {
		kprintf_direct("command: Error reading stdin\n");
		return(-1);
	}

	if(((unsigned char) *bufptr) == 0x8) {			/* if backspace, erase character */
		*bufptr--=0;

		if((count - 1) > 0) count--;
	}
	else if(((unsigned char) *bufptr) == '\n') {			/* if newline, return */
		*++bufptr=0;
		break;
	}
	else
	{
		bufptr++;
		count++;
	}

	if(count >= size) return(size);
}

return(size);
}

