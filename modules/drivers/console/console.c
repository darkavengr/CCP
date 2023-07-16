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
/* colours are XY where X is background colour and Y is foreground colour

0 	Black 			8 	Dark Grey (Light Black)
1 	Blue 			9 	Light Blue
2 	Green 			10 	Light Green
3 	Cyan 			11 	Light Cyan
4 	Red 			12 	Light Red
5 	Purple 			13 	Light Purple
6 	Brown/Orange 		14 	Yellow (Light Orange)
7 	Light Grey (White) 	15 	White (Light White) */

uint8_t row=0;
uint8_t col=0;
uint8_t colour=7;
size_t ppos;

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
  
consolepos=KERNEL_HIGH+0xB8000+(((row*MAX_X)+col)*2);	/* address to write to */

if((size_t) consolepos % 2 == 1) consolepos++;
  
  while(size-- > 0) {
   if(col++ >= MAX_X) {
    col=0;						/* wrap round */
    row++;
   }

   if(*s == 0x0A || *s == 0x0D) {			/* newline */
    row++;						/* wrap round */
    col=0;
    consolepos=KERNEL_HIGH+0xB8000+(((row*80)+col)*2);
   }
  else
  {
   *consolepos++=*s++; 
   *consolepos++=colour;

   if(row >= 24) {					/* scroll */
    memcpy(scrollbuf,KERNEL_HIGH+0xB80a0,(MAX_X*2)*25);		/* to buffer */
    memcpy(KERNEL_HIGH+0xB8000,scrollbuf,(MAX_X*2)*25);		/* to screen */

    col=1;						/* wrap round */
    row=23;
    movecursor(row,col);

    consolepos=KERNEL_HIGH+0xB8000+(((row*MAX_X)+col)*2);
   }

  }
 }

   ppos=(row*MAX_X)+col;
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
 * In:  row	Row to move to
        col	Column to move to
 *
 *  Returns: nothing
 *
 */

void movecursor(uint16_t r,uint16_t c) {
uint16_t ppos;

ppos=(r*MAX_X)+c;

outb(0x3d4,0xf);							/* cursor high */
outb(0x3d5,(ppos  & 0xff));
outb(0x3d4,0xe);							/* cursor row */
outb(0x3d5,(ppos >> 8) & 0xff);

row=r;
col=c;
return;
}

/*
 * Get screen cursor X position 
 *
 * In:  nothing
 *
 *  Returns: X position
 *
 */

uint32_t get_cursor_row(void) {
return(row);

}

/*
 * Get screen cursor Y position 
 *
 * In:  nothing
 *
 *  Returns: Y position
 *
 */
uint32_t get_cursor_col(void) {
return(col);
}

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
BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;

strcpy(&device.dname,"CONOUT");
device.charioread=NULL;
device.chariowrite=&outputconsole;
device.ioctl=NULL;
device.flags=0;
device.data=NULL;
device.next=NULL;

add_char_device(&device);			/* con */

row=boot_info->cursor_row++;
col=boot_info->cursor_col++;

init_console_device(_WRITE,1,&outputconsole);
init_console_device(_WRITE,2,&outputconsole);

return(NULL);
}

void setconsolecolour(uint8_t c) {
 colour=c;
 
}

uint8_t getconsolecolour(void) {
return(colour); 
}
