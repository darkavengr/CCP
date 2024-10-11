/*			 CCP Version 0.0.1

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
#include "kernelhigh.h"
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "console.h"
#include "bootinfo.h"

#define MODULE_INIT console_init

uint8_t color=7;		/* text color */

/* color is in the format XY where X is background color and Y is foreground color

0 	Black 			8 	Dark Grey (Light Black)
1 	Blue 			9 	Light Blue
2 	Green 			A	Light Green
3 	Cyan 			B	Light Cyan
4 	Red 			C	Light Red
5 	Purple 			D 	Light Purple
6 	Brown/Orange 		E 	Yellow (Light Orange)
7 	Light Grey (White) 	F 	White (Light White) */

/*
 * Initialize screen
 *
 * In:  char *init	Initialization string
 *
 * Returns: nothing
 *
 */

void console_init(char *init) {
CHARACTERDEVICE device;

color=7;

strncpy(&device.name,"CONOUT",MAX_PATH);

device.charioread=NULL;
device.chariowrite=&outputconsole;
device.ioctl=&console_ioctl;
device.flags=0;
device.data=NULL;
device.next=NULL;

add_character_device(&device);			/* add console device */

init_console_device(_WRITE,1,&outputconsole);
init_console_device(_WRITE,2,&outputconsole);
return(NULL);
}

/*
 * Console I/O function
 *
 * In:  op	Operation (0=read,1=write)
			    buf	Buffer
	len	Number of bytes to read/write
 *
 *  Returns: nothing
 *
 */
size_t outputconsole(char *s,size_t size) {
char *consolepos;
char *scrollbuf[MAX_X*MAX_Y];
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;		/* point to boot information */

consolepos=KERNEL_HIGH+0xB8000+(((bootinfo->cursor_row*MAX_X)+bootinfo->cursor_col)*2);	/* address to write to */

if((size_t) consolepos % 2 == 1) consolepos++;
	 
	while(size-- > 0) {
		if(bootinfo->cursor_col++ >= MAX_X) {
			bootinfo->cursor_col=0;						/* wrap round */
			bootinfo->cursor_row++;
		}
	
		if(*s == 0x0A || *s == 0x0D) {			/* newline */
			bootinfo->cursor_row++;						/* wrap round */
			bootinfo->cursor_col=0;
			consolepos=KERNEL_HIGH+0xB8000+(((bootinfo->cursor_row*80)+bootinfo->cursor_col)*2);

			consolepos++;
			s++;
		}
		else
		{
			*consolepos++=*s++; 
			*consolepos++=color;
		}

		if(bootinfo->cursor_row >= 24) {					/* scroll */
			memcpy(scrollbuf,KERNEL_HIGH+0xB80a0,(MAX_X*2)*25);		/* to buffer */
			memcpy(KERNEL_HIGH+0xB8000,scrollbuf,(MAX_X*2)*25);		/* to screen */

			bootinfo->cursor_col=1;						/* wrap round */
			bootinfo->cursor_row=23;
			movecursor(bootinfo->cursor_row,bootinfo->cursor_col);

			consolepos=KERNEL_HIGH+0xB8000+(((bootinfo->cursor_row*MAX_X)+bootinfo->cursor_col)*2);
		}

		
	}

movecursor(bootinfo->cursor_row,bootinfo->cursor_col);

return(size);
}

/*
 * Move cursor
 *
 * In:  row	Row move to
	col	Column to move to
 *
 *  Returns: nothing
 *
 */

void movecursor(uint16_t row,uint16_t col) {
uint16_t newpos;
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;

newpos=(row*MAX_X)+col;

outb(0x3d4,0xf);							/* cursor low byte */
outb(0x3d5,(newpos  & 0xff));
outb(0x3d4,0xe);							/* cursor high byte */
outb(0x3d5,(newpos >> 8) & 0xff);

bootinfo->cursor_row=row;
bootinfo->cursor_col=col;
return(0);
}


/*
 * Console ioctl handler
 *
 * In:  handle	Handle created by open() to reference device
	       request Request number
	       buffer  Buffer
 *
 *  Returns: -1 on error, 0 on success
 *
 */
size_t console_ioctl(size_t handle,unsigned long request,void *buffer) {
FILERECORD consoledevice;
uint8_t *bufptr;
size_t drive;
BLOCKDEVICE bd;
uint8_t pos;
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;

bufptr=buffer;

switch(request) {
	case IOCTL_CONSOLE_SET_CURSOR_ROW:		/* set cursor row position */
		pos=*bufptr;

		movecursor(pos,bootinfo->cursor_row);
		return(0);

	case IOCTL_CONSOLE_SET_CURSOR_COL:		/* set cursor row position */
		pos=*bufptr;

		movecursor(bootinfo->cursor_row,pos);
		return(0);

	case IOCTL_CONSOLE_GET_CURSOR_ROW:		/* get cursor row position */
		*bufptr=bootinfo->cursor_row;
		return(0);

	case IOCTL_CONSOLE_GET_CURSOR_COL:		/* get cursor column position */
		*bufptr=bootinfo->cursor_col;
		return(0);

	case IOCTL_CONSOLE_SET_COLOR:			/* set text color */
		color=*bufptr;
		return(0);

	case IOCTL_CONSOLE_GET_COLOR:			/* get text color */
		*bufptr=color;
		return(0);

	default:
		setlasterror(INVALID_PARAMETER);
		return(-1);
}

return(-1);
}
