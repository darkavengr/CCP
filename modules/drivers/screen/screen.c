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
#include "screen.h"
#include "bootinfo.h"
#include "string.h"
#include "tty.h"
#include "ttycharacters.h"

#define MODULE_INIT screen_init

uint8_t color=DEFAULT_TEXT_COLOR;	/* text color */
char *scrollbuf[SCREEN_MAX_X*SCREEN_MAX_Y];
size_t line_end_pos;

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
 * In:  init	Initialization string
 *
 * Returns: nothing
 *
 */

size_t screen_init(char *init) {
TTY_HANDLER handler;

color=(COLOR_BLACK << 4) | COLOR_WHITE;			/* set default text color to white on black */

/* create TTY handler */
handler.ttywrite=&screen_write;
handler.ttyread=NULL;

if(tty_register_write_handler(&handler) == -1) {	/* register TTY handler */
	kprintf_direct("screen: Can't register TTY handler for screen: %s\n",kstrerr(getlasterror()));
	return(-1);
}

line_end_pos=0;
return(0);
}

/*
 * Text screen I/O function
 *
 * In:  op	Operation (0=read,1=write)
	buf	Buffer
	len	Number of bytes to read/write
 *
 *  Returns: nothing
 *
 */
size_t screen_write(char *data,size_t size) {
char *screenpos;
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;		/* point to boot information */
size_t count=size;
char *dataptr=data;
size_t screen_clear_count;

screenpos=KERNEL_HIGH+0xB8000+(((bootinfo->cursor_row*SCREEN_MAX_X)+bootinfo->cursor_col)*2);	/* address to write to */

if((size_t) screenpos % 2 == 1) screenpos++;
	 
	while(count-- > 0) {
		if(bootinfo->cursor_col++ >= SCREEN_MAX_X) {
			bootinfo->cursor_col=0;						/* wrap round */
			bootinfo->cursor_row++;
		}
	
		if(((char) *dataptr == 0x0A) || ((char) *dataptr == 0x0D)) {			/* newline */
			bootinfo->cursor_row++;						/* wrap round */
			bootinfo->cursor_col=0;

			line_end_pos=0;		/* set end of line position */

			screenpos=KERNEL_HIGH+0xB8000+(((bootinfo->cursor_row*80)+bootinfo->cursor_col)*2);

			screenpos++;
			dataptr++;
		}
		else if((char) *dataptr == TTY_CHAR_BACK) {			/* backspace */
			asm("xchg %bx,%bx");

			if((bootinfo->cursor_col-1) > 0) bootinfo->cursor_col--;
			
			*screenpos--=0;		/* erase backspace character */
			*screenpos--=0;		/* erase character */

			movecursor(bootinfo->cursor_row,bootinfo->cursor_col-2);
		}
		else if((char) *dataptr == TTY_CHAR_HOME) {			/* home */
			movecursor(bootinfo->cursor_row,0);
		}
		else if((char) *dataptr == TTY_CHAR_END) {			/* end */
			movecursor(bootinfo->cursor_row,line_end_pos);
		}
		else
		{
			*screenpos++=*dataptr++; 
			*screenpos++=color;

			line_end_pos++;
		}

		movecursor(bootinfo->cursor_row,bootinfo->cursor_col);

		if(bootinfo->cursor_row >= (SCREEN_MAX_Y-1)) {					/* if at bottom of screen, scroll up */
			memcpy(scrollbuf,KERNEL_HIGH+0xB80a0,(SCREEN_MAX_X*2)*SCREEN_MAX_Y);		/* to buffer */
			memcpy(KERNEL_HIGH+0xB8000,scrollbuf,(SCREEN_MAX_X*2)*SCREEN_MAX_Y);		/* to screen */
		
			screenpos=KERNEL_HIGH+0xB80a0;

			line_end_pos=0;

			bootinfo->cursor_col=1;						/* wrap round */
			bootinfo->cursor_row=SCREEN_MAX_Y-2;
			movecursor(bootinfo->cursor_row,bootinfo->cursor_col);

			screenpos=KERNEL_HIGH+0xB8000+(((bootinfo->cursor_row*SCREEN_MAX_X)+bootinfo->cursor_col)*2);
		}

		
	}

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

newpos=(row*SCREEN_MAX_X)+col;

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
size_t screen_ioctl(size_t handle,unsigned long request,void *buffer) {
uint8_t *bufptr;
size_t drive;
BLOCKDEVICE bd;
uint8_t pos;
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;

bufptr=buffer;

switch(request) {
	case IOCTL_SCREEN_SET_CURSOR_ROW:		/* set cursor row position */
		pos=*bufptr;

		movecursor(pos,bootinfo->cursor_row);
		return(0);

	case IOCTL_SCREEN_SET_CURSOR_COL:		/* set cursor row position */
		pos=*bufptr;

		movecursor(bootinfo->cursor_row,pos);
		return(0);

	case IOCTL_SCREEN_GET_CURSOR_ROW:		/* get cursor row position */
		*bufptr=bootinfo->cursor_row;
		return(0);

	case IOCTL_SCREEN_GET_CURSOR_COL:		/* get cursor column position */
		*bufptr=bootinfo->cursor_col;
		return(0);

	case IOCTL_SCREEN_SET_COLOR:			/* set text color */
		color=*bufptr;
		return(0);

	case IOCTL_SCREEN_GET_COLOR:			/* get text color */
		*bufptr=color;
		return(0);

	default:
		setlasterror(INVALID_PARAMETER);
		return(-1);
}

return(-1);
}
