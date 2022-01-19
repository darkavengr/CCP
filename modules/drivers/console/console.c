/*  CCP Version 0.0.1
    (C) Matthew Boote 2020

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
#include "../../../header/errors.h"
#include "../../../devicemanager/device.h"
#include "../../../filemanager/vfs.h"
#include "console.h"
#include "../../../header/bootinfo.h"

#define KERNEL_HIGH (1 << (sizeof(unsigned int)*8)-1)

#define MODULE_INIT console_init

size_t outputconsole(unsigned int ignore,char *s,size_t size);
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
unsigned int ppos;

size_t outputconsole(unsigned int ignore,char *s,size_t size) {
char *consolepos;
uint32_t pos;
char *scrollbuf[MAX_X*MAX_Y];
unsigned int wch;

consolepos=KERNEL_HIGH+0xB8000+(((row*MAX_X)+col)*2);

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

/* move cursor */

void movecursor(uint16_t row,uint16_t col) {
uint16_t ppos;

ppos=(row*MAX_X)+col;

outb(0x3d4,0xf);							/* cursor high */
outb(0x3d5,(ppos  & 0xff));
outb(0x3d4,0xe);							/* cursor row */
outb(0x3d5,(ppos >> 8) & 0xff);
return;
}

/* get cursor */

uint32_t getcursorpos(void) {
uint8_t row;
uint8_t col;

outb(0x3d4,0xf);							/* cursor high */
row=inb(0x3d5);
outb(0x3d4,0xe);							/* cursor row */
col=inb(0x3d5);

row=row  & 0xff;
col=col >> 8;

return((row << 16)+col);
}

void console_init(char *init) {
CHARACTERDEVICE device;
uint8_t *cursor_loc=KERNEL_HIGH+(size_t)  BOOT_INFO_CURSOR;

strcpy(&device.dname,"CONOUT");
device.chario=&outputconsole;
device.ioctl=NULL;
device.flags=0;
device.data=NULL;
device.next=NULL;

add_char_device(&device);			/* con */

row=*cursor_loc++;
col=*cursor_loc++;

return(NULL);
}

void setconsolecolour(uint8_t c) {
 colour=c;
 
}

uint8_t getconsolecolour(void) {
return(colour); 
}
