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

/* CCP keyboard driver */

#include <stdint.h>
#include "../../../header/kernelhigh.h"
#include "../../../header/errors.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "../../../filemanager/vfs.h"
#include "../../../processmanager/signal.h"
#include "../../../header/bootinfo.h"
#include "../../../header/debug.h"
#include "keyb.h"

#define MODULE_INIT keyb_init

void readconsole(char *buf,size_t size);
void keyb_init(void);

size_t readkey(void);
void keyb_init(void);

/* characters from scan codes */
char *scancodes_unshifted[] = { "`",0x22,"1","2","3","4","5","6","7","8","9","0","-","="," ","\t","q","w","e","r","t","y","u", \
			    "i","o","p","[","]","\n"," ","a","s","d","f","g","h","j","k","l",";","@","#"," ","\\","z","x","c", \
		           	"v","b","n","m",",",".","/",",","/"," "," "," "," "," "," "," "," "," "," "," ", \
			    " "," "," "," "," "," "," ","7","8","9","-","4","5","6","+","1","2","3","0"," " };

char *scancodes_shifted[] = { "`",0x22,"!","@","\£","$","%%","^","&","*","(",")","_","+","\b","\t","Q","W","E","R","T","Y","U", \
			    "I","O","P","{","}","\n"," ","A","S","D","F","G","H","J","K","L",":","%","`","£","|","Z", \
			    "X","C","V","B","N","M","<",">","?"," "," "," "," "," "," "," "," "," "," "," "," ", \
			    " "," "," "," "," "," "," ","7","8","9","-","4","5","6","+","1","2","3","0","." };

char *keybbuf[KEYB_BUFFERSIZE];
char *keybuf=keybbuf;
size_t readcount=0;
char *keylast=keybbuf;
uint8_t keyboardflags;		/* for caps lock, shift, control and alt keys */

/*
 * Initialize keyboard
 *
 * In: nothing
 *
 * Returns nothing
 *
 */
void keyb_init(void) {
CHARACTERDEVICE device;

keyboardflags=0;
readcount=0;

/* create console device */
strncpy(&device.name,"CON",MAX_PATH);
device.charioread=&readconsole;
device.chariowrite=NULL;
device.ioctl=NULL;
device.flags=0;
device.data=NULL;
device.next=NULL;

add_character_device(&device);

setirqhandler(1,&readkey);		/* set irq handler */
	
init_console_device(_READ,0,&readconsole);
return;
}

/*
 * Read from console
 *
 * In: ignore		Ignored value
       buf		Buffer
       size_t size	Number of character to read
 *
 *  Returns nothing
 *
 */
void readconsole(char *buf,size_t size) {
keybuf=keybbuf; 						/* point to start of buffer */
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;

readcount=0;

do {	
	if(readcount > size) readcount=0;

}  while(readcount < size);

memcpy(buf,keybbuf,size);				
readcount=0;
	
return;
}

/*
 * Keyboard irq handler
 *
 * In: nothing
 *
 *  Returns nothing
 *
 * called by irq1 handler see init.asm  */

size_t readkey(void) {
uint8_t keycode;
char c;
size_t row,col;
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;

if((inb(0x64) & 1) == 0) return;					/* not ready */

keycode=inb(0x60);

/* if it's a break code, then do various keys */

if((keycode & 128) == 128) {
/* clear keyboard flags if key is up */

	if(keycode == LEFT_SHIFT_UP || keycode == RIGHT_SHIFT_UP) {
		keyboardflags &= !SHIFT_PRESSED;
		return;
	}

	if(keycode == LEFT_ALT_UP || keycode == RIGHT_ALT_UP) {
		keyboardflags &= !ALT_PRESSED;
		return;
	}

	if(keycode == LEFT_CTRL_UP || keycode == RIGHT_CTRL_UP) {
		keyboardflags &= !CTRL_PRESSED;
		return;
	}

	return;						/* ignore return codes */
}

/* for the rest of the keys, only make codes matter */

if(readcount++ == KEYB_BUFFERSIZE) return;

DEBUG_PRINT_HEX(readcount);

switch(keycode) {								/* control characters */

	case KEY_TAB:
		write(stdout,"\t",1);
	 	return;

	case KEY_HOME_7:							/* move cursor to start of line */  
		bootinfo->cursor_row=0;
		return;

	case KEY_END1:								/* move cursor to end of line */
		col=strlen(keybbuf);

		bootinfo->cursor_row=25;
		return;

	case LEFT_SHIFT:
		keyboardflags |= SHIFT_PRESSED;

		if((readcount-1) > 0) readcount--;				/* decrement readcount because shift shouldn't count this as a character on it's own */
		return;
	
	case RIGHT_SHIFT:
		keyboardflags |= SHIFT_PRESSED;

	 	if((readcount-1) > 0) readcount--;				/* see above comment */
	 	return;

	case KEY_ALT:
		keyboardflags |= ALT_PRESSED;
	
		if((readcount-1) > 0) readcount--;				/* see above comment */
		return;

	case KEY_BACK:
		*keybuf=8;
		return;							
	
	case KEY_CAPSLOCK:
		if((keyboardflags & CAPSLOCK_PRESSED)) {	/* toggle capslock */
			keyboardflags &= !CAPSLOCK_PRESSED;
	 	}
	 	else
	 	{
			keyboardflags |= CAPSLOCK_PRESSED;
		}

		if((readcount-1) > 0) readcount--;				/* see above comment */
	 	return;

	case KEY_CTRL:					/* ctrl characters and ctrl alt del */
		keycode=inb(0x60);

		keyboardflags |= CTRL_PRESSED;

		if((readcount-1) > 0) readcount--;				/* see above comment about ignoring keys */
		return;
}

if((keyboardflags & CTRL_PRESSED)) { 							/* control characters */

switch(keycode) {
	case KEY_SINGQUOTE_AT:
		*keybuf++=CTRL_AT;
		write(stdout,"^@",1);

		return;

	case KEY_A:
		*keybuf++=CTRL_A;
		write(stdout,"^A",1);

		return;
	  
	case KEY_B:
		*keybuf++=CTRL_B;
		write(stdout,"^B",1);

	return;

	case KEY_C:
		write(stdout,"^C",1);

		sendsignal(getpid(),SIGTERM);		/* send signal to terminate signal */
		return;

	case KEY_D:
		*keybuf++=CTRL_D;
		write(stdout,"^D",1);
	 	return;

	case KEY_E:
		*keybuf++=CTRL_E;
		write(stdout,"^E",1);
		return;

	case KEY_F:
		*keybuf++=CTRL_F;
		write(stdout,"^F",1);

		return;

	case KEY_G:
		*keybuf++=CTRL_G;
		write(stdout,"^G",1);

		return;


	case KEY_I:								/* tab */
	 	*keybuf++="\t";
		write(stdout,"\t",1);
	  	return;

	case KEY_J:
		*keybuf++=CTRL_J;
		write(stdout,"^J",1);

	 	return;

	case KEY_K:
		*keybuf++=CTRL_K;
		write(stdout,"^K",1);

	 	return;

	case KEY_L:
		*keybuf++=CTRL_L;
	  	write(stdout,"^L",1);

	  	return;

	case KEY_M:
		*keybuf++=CTRL_M;
		write(stdout,"^M",1);
		return;

	case KEY_N:
		*keybuf++=CTRL_N;
		write(stdout,"^N",1);
		return;

	case KEY_O:
		*keybuf++=CTRL_O;
		write(stdout,"^O",1);
		return;

	case KEY_P:
		*keybuf++=CTRL_P;
		write(stdout,"^P",1);
		return;

	case KEY_Q:
		*keybuf++=CTRL_Q;
		write(stdout,"^Q",1);
		return;

	case KEY_R:
		*keybuf++=CTRL_R;
		write(stdout,"^R",1);
		return;

	case KEY_S:
		*keybuf++=CTRL_S;
		write(stdout,"^S",1);
		return;

	case KEY_T:
		*keybuf++=CTRL_T;
		write(stdout,"^T",1);
		return;

	case KEY_V:
		*keybuf++=CTRL_V;
		write(stdout,"^V",1);
		return;

	case KEY_W:
		*keybuf++=CTRL_W;
		write(stdout,"^W",1);
		return;

	case KEY_X:
		*keybuf++=CTRL_X;
	  	write(stdout,"^X",1);
	  	return;

	case KEY_Y:
		*keybuf++=CTRL_Y;
		write(stdout,"^Y",1);
		return;

	case KEY_Z:
		*keybuf++=CTRL_Z;
		return;
	}

}

if((keyboardflags & CTRL_PRESSED) && (keyboardflags & ALT_PRESSED) && keycode == KEY_DEL) {	/* press ctrl-alt-del */
	shutdown(1);
}

/* if caps lock in on only shift letters */

if((keyboardflags & CAPSLOCK_PRESSED)) {
	c=*scancodes_shifted[keycode];		/* get character */

 	if((c >= 'A' && c <= 'Z') && ((keyboardflags & SHIFT_PRESSED) == 0)) {
		c=c+32;				/* to lowercase */
		*keybuf++=c;
		write(stdout,scancodes_shifted[keycode],1);
		return;
	}

	if((c >= 'A' && c <= 'Z') && (keyboardflags & SHIFT_PRESSED)) {
		*keybuf++=c;

		write(stdout,scancodes_unshifted[keycode],1);
		return;
	}

}
	
/* If capslock is not on, use uppercase letters if shift pressed. Use lowercase letters otherwise */

if((keyboardflags & SHIFT_PRESSED)) {
	write(stdout,scancodes_shifted[keycode],1);

	*keybuf++=*scancodes_shifted[keycode];
}
else
{
	write(stdout,scancodes_unshifted[keycode],1);

	*keybuf++=*scancodes_unshifted[keycode];	
}

return;
}


