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

/* PS/2 keyboard driver */

#include <stdint.h>
#include "kernelhigh.h"
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "signal.h"
#include "bootinfo.h"
#include "debug.h"
#include "keyb.h"
#include "string.h"
#include "tty.h"
#include "ttycharacters.h"

#define MODULE_INIT keyboard_init

/* characters from scan codes */

char *scancodes_unshifted[] = { "","`","1","2","3","4","5","6","7","8","9","0","-","=",TTY_CHAR_BACK,"\t","q","w","e","r","t","y",
			        "u","i","o","p","[","]","\n"," ","a","s","d","f","g","h","j","k","l",";","@","#"," ","\\","z",
			        "x","c","v","b","n","m",",",".","/","",TTY_CHAR_PTSCR,""," ","",TTY_CHAR_F1,TTY_CHAR_F2,TTY_CHAR_F3,
				TTY_CHAR_F4,TTY_CHAR_F5,TTY_CHAR_F6,TTY_CHAR_F7,TTY_CHAR_F8,TTY_CHAR_F9,TTY_CHAR_F10,"","",TTY_CHAR_HOME,
				TTY_CHAR_UP,TTY_CHAR_PGUP,"-",TTY_CHAR_LEFT,"7",TTY_CHAR_RIGHT,"+",TTY_CHAR_END,TTY_CHAR_DOWN,
				TTY_CHAR_PGDOWN,TTY_CHAR_INSERT,TTY_CHAR_BACK };

char *scancodes_shifted[] = { "","`","!","@","\£","$","%%","^","&","*","(",")","_","+",TTY_CHAR_BACK,"\t","Q","W","E","R","T","Y","U",
			      "I","O","P","{","}","\n"," ","A","S","D","F","G","H","J","K","L",":","%","`","£","|","Z","X","C","V",
			      "B","N","M","<",">","?",TTY_CHAR_PTSCR," "," "," ",TTY_CHAR_F1,TTY_CHAR_F2,TTY_CHAR_F3,TTY_CHAR_F4,
			      TTY_CHAR_F5,TTY_CHAR_F6,TTY_CHAR_F7,TTY_CHAR_F8,TTY_CHAR_F9,TTY_CHAR_F10," "," ",TTY_CHAR_HOME,TTY_CHAR_UP,
			      TTY_CHAR_PGUP,"-",TTY_CHAR_LEFT,"7",TTY_CHAR_RIGHT,"+",TTY_CHAR_END,TTY_CHAR_DOWN,TTY_CHAR_PGDOWN,
			      TTY_CHAR_INSERT,TTY_CHAR_BACK };
	
char *keybbuf[KEYB_BUFFERSIZE];
char *keybuf=keybbuf;
size_t readcount=0;
char *keylast=keybbuf;
uint8_t keyboardflags=0;		/* for caps lock, shift, control and alt keys */
uint8_t keyboardledflags=0;

/*
 * Initialize keyboard
 *
 * In: nothing
 *
 * Returns nothing
 *
 */
size_t keyboard_init(void) {
uint8_t *keyflagsptr=KERNEL_HIGH+BIOS_DATA_AREA_ADDRESS;
TTY_HANDLER handler;

keyboardflags=0;

if(((uint8_t) *keyflagsptr & KEYB_BDA_CAPS_LOCK)) {
	keyboardflags |= CAPS_LOCK_PRESSED;
	keyboardledflags |= CAPS_LOCK_LED;
}	


readcount=0;

/* create TTY handler */
handler.ttyread=&keyboard_read;
handler.ttywrite=NULL;

if(tty_register_read_handler(&handler) == -1) {	/* add character device */
	kprintf_direct("keyb: Can't register TTY handler for keyboard: %s\n",kstrerr(getlasterror()));
	return(-1);
}

setirqhandler(1,'KEYB',&readkey);		/* set IRQ handler */

return(0);
}

/*
 * Read from keyboard
 *
 * In: buf	Buffer
       size	Number of character to read
 *
 *  Returns nothing
 *
 */
void keyboard_read(char *buf,size_t size) {
keybuf=keybbuf; 						/* point to start of buffer */

readcount=0;

while(readcount < size) ;;

memcpy(buf,keybbuf,size);

readcount=0;
	
return;
}

/*
 * Read key
 *
 * In: nothing
 *
 *  Returns nothing
 *
 */
void readkey(void) {
uint8_t keycode;
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;
uint8_t keychar;

if((inb(KEYBOARD_STATUS_PORT) & KEYB_STATUS_OUTPUT_BUFFER_FULL) == 0) return;		/* not ready */

keycode=inb(KEYBOARD_DATA_PORT);			/* read scan code */

/* if it's a break code, then do various keys */

if(keycode & 128) {
/* clear keyboard flags if key is up */

	if((keycode == LEFT_SHIFT_UP) || (keycode == RIGHT_SHIFT_UP)) {
		keyboardflags &= !SHIFT_PRESSED;
		return;
	}

	if((keycode == LEFT_ALT_UP) || (keycode == RIGHT_ALT_UP)) {
		keyboardflags &= !ALT_PRESSED;
		return;
	}

	if((keycode == LEFT_CTRL_UP) || (keycode == RIGHT_CTRL_UP)) {
		keyboardflags &= !CTRL_PRESSED;
		return;
	}

	return;
}

/* for the rest of the keys only the make codes matter */

if(readcount++ == KEYB_BUFFERSIZE) return;		/* buffer is full */

/* handle special keys */

switch(keycode) {
	case LEFT_SHIFT:
		keyboardflags |= SHIFT_PRESSED;

		if((readcount-1) >= 0) readcount--;	/* decrement readcount because shift shouldn't count this as a character on it's own */
		return;
	
	case RIGHT_SHIFT:
		keyboardflags |= SHIFT_PRESSED;

	 	if((readcount-1) >= 0) readcount--;
	 	return;

	case KEY_ALT:
		keyboardflags |= ALT_PRESSED;
	
		*keybuf++=TTY_CHAR_ALT;
		return;					
	
	case KEY_SCROLL_LOCK:
		if((keyboardflags & SCROLL_LOCK_PRESSED)) {		/* toggle scroll lock */
			keyboardflags &= !SCROLL_LOCK_PRESSED;
			keyboardledflags &= !SCROLL_LOCK_LED;
	 	}
	 	else
	 	{
			keyboardflags |= SCROLL_LOCK_PRESSED;
			keyboardledflags |= SCROLL_LOCK_LED;
		}

		outb(KEYBOARD_DATA_PORT,SET_KEYBOARD_LEDS);		/* set keyboard LEDs */
		outb(KEYBOARD_DATA_PORT,keyboardledflags);

		if((readcount-1) >= 0) readcount--;
	 	return;

	case KEY_NUMLOCK:
		if((keyboardflags & NUM_LOCK_PRESSED)) {		/* toggle num lock */
			keyboardflags &= !NUM_LOCK_PRESSED;
			keyboardledflags &= !NUM_LOCK_LED;
	 	}
	 	else
	 	{
			keyboardflags |= NUM_LOCK_PRESSED;
			keyboardledflags |= NUM_LOCK_LED;
		}

		outb(KEYBOARD_DATA_PORT,SET_KEYBOARD_LEDS);		/* set keyboard LEDs */
		outb(KEYBOARD_DATA_PORT,keyboardledflags);

		if((readcount-1) >= 0) readcount--;
	 	return;

	case KEY_CAPSLOCK:
		if((keyboardflags & CAPS_LOCK_PRESSED)) {	/* toggle caps lock */
			keyboardflags &= !CAPS_LOCK_PRESSED;
			keyboardledflags &= !CAPS_LOCK_LED;
	 	}
	 	else
	 	{
			keyboardflags |= CAPS_LOCK_PRESSED;
			keyboardledflags |= CAPS_LOCK_LED;
		}

		outb(KEYBOARD_DATA_PORT,SET_KEYBOARD_LEDS);		/* set keyboard LEDs */
		outb(KEYBOARD_DATA_PORT,keyboardledflags);

		if((readcount-1) >= 0) readcount--;
	 	return;

	case KEY_CTRL:					/* ctrl characters and ctrl alt del */
		keycode=inb(KEYBOARD_DATA_PORT);

		keyboardflags |= CTRL_PRESSED;

		if((readcount-1) >= 0) readcount--;				/* see above comment about ignoring keys */
		return;
}

if((keyboardflags & CTRL_PRESSED) && (keyboardflags & ALT_PRESSED) && keycode == KEY_DEL) {	/* pressed ctrl-alt-del */
	shutdown(1);
}

if((keyboardflags & CTRL_PRESSED)) {					/* control characters */
	if( ((keycode >= KEY_Q) && (keycode <= KEY_LEFT_SQR_BRACKET_LEFT_CURL_BRACKET)) ||
 	    ((keycode >= KEY_A) && (keycode <= KEY_L)) ||
	    ((keycode >= KEY_Z) && (keycode <= KEY_M))) {		/* ctrl-X characters */
		*keybuf++=(uint8_t) *scancodes_shifted[keycode]-'@';		/* put ASCII character for control */
	}
}
else
{ 
	/* if caps lock is on only shift letters */

	if((keyboardflags & CAPS_LOCK_PRESSED)) {
		keychar=*scancodes_shifted[keycode];		/* get character */

 		if(((keychar >= 'A') && (keychar <= 'Z')) && ((keyboardflags & SHIFT_PRESSED) == 0)) {
			keychar += 32;				/* convert to lowercase */

			*keybuf++=keychar;
			return;
		}

		if(((keychar >= 'A') && (keychar <= 'Z')) && (keyboardflags & SHIFT_PRESSED)) {
			*keybuf++=keychar;
			return;
		}

	}
	
	/* If capslock is off, use uppercase letters if shift pressed. Use lowercase letters otherwise */

	if((keyboardflags & SHIFT_PRESSED)) {
		*keybuf++=*scancodes_shifted[keycode];
	}
	else
	{
		*keybuf++=*scancodes_unshifted[keycode];	
	}
}

return;
}

