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
#include "../../../header/kernelhigh.h"
#include "../../../header/errors.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "../../../filemanager/vfs.h"
#include "console.h"
#include "../../../header/bootinfo.h"

#define MODULE_INIT console_init

size_t outputconsole(char *s,size_t size);
void movecursor(uint16_t row,uint16_t col);
uint32_t getcursorpos(void);
void console_init(char *init);
void setconsolecolour(uint8_t c);

size_t ppos;

/* bootinfo->cursor_colours are XY where X is background bootinfo->cursor_colour and Y is foreground bootinfo->cursor_colour

0 	Black 			8 	Dark Grey (Light Black)
1 	Blue 			9 	Light Blue
2 	Green 			10 	Light Green
3 	Cyan 			11 	Light Cyan
4 	Red 			12 	Light Red
5 	Purple 			13 	Light Purple
6 	Bbootinfo->cursor_rown/Orange 		14 	Yellow (Light Orange)
7 	Light Grey (White) 	15 	White (Light White) */

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

strcpy(&device.dname,"CONOUT");
device.charioread=NULL;
device.chariowrite=&outputconsole;
device.ioctl=NULL;
device.flags=0;
device.data=NULL;
device.next=NULL;

add_char_device(&device);			/* con */

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
uint32_t pos;
char *scrollbuf[MAX_X*MAX_Y];
size_t wch;
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
		}
		else
		{
		 *consolepos++=*s++; 
		 *consolepos++=7;

			if(bootinfo->cursor_row >= 24) {					/* scroll */
				memcpy(scrollbuf,KERNEL_HIGH+0xB80a0,(MAX_X*2)*25);		/* to buffer */
				memcpy(KERNEL_HIGH+0xB8000,scrollbuf,(MAX_X*2)*25);		/* to screen */

				bootinfo->cursor_col=1;						/* wrap round */
				bootinfo->cursor_row=23;
				movecursor(bootinfo->cursor_row,bootinfo->cursor_col);

				consolepos=KERNEL_HIGH+0xB8000+(((bootinfo->cursor_row*MAX_X)+bootinfo->cursor_col)*2);
			}

		 }
	}

ppos=(bootinfo->cursor_row*MAX_X)+bootinfo->cursor_col;
asm(".intel_syntax noprefix");
asm("mov dx,0x3d4");				/* set cursor position */
asm("mov al,0x0f");
asm("out dx,al");

asm("mov dx,0x3d5");
asm("mov ax,[ppos]");
asm("and ax,0xff");
asm("out dx,al");

asm("mov dx,0x3d4");				/* set cursor position */
asm("mov al,0x0e");
asm("out dx,al");
			
asm("mov  dx,0x3d5");
asm("mov ax,[ppos]");
asm("mov cl,8");
asm("shr ax,cl");
asm("and ax,0xFF");
asm("out dx,al");
asm(".att_syntax");					/* ugggh */

return(size);
}

/*
 * Move cursor
 *
 * In:  r	Row move to
	c	Column to move to
 *
 *  Returns: nothing
 *
 */

void movecursor(uint16_t r,uint16_t c) {
uint16_t ppos;
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;

ppos=(r*MAX_X)+c;

outb(0x3d4,0xf);							/* cursor high */
outb(0x3d5,(ppos  & 0xff));
outb(0x3d4,0xe);							/* cursor bootinfo->cursor_row */
outb(0x3d5,(ppos >> 8) & 0xff);

bootinfo->cursor_row=r;
bootinfo->cursor_col=c;
return;
}


