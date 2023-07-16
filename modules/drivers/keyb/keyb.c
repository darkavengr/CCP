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
#include "../../../processmanager/signal.h"
#include "keyb.h"

#define MODULE_INIT keyb_init

void readconsole(char *buf,size_t size);
void keyb_init(void);

size_t readkey(void);
void keyb_init(void);

uint32_t capson=FALSE;
uint32_t shiftpressed=FALSE;
uint32_t ctrlpressed=FALSE;
uint32_t altpressed=FALSE;

/* characters from scan codes */
char *scancodes_unshifted[] = { "`",0x22,"1","2","3","4","5","6","7","8","9","0","-","="," ","\t","q","w","e","r","t","y","u", \
		         "i","o","p","[","]","\n"," ","a","s","d","f","g","h","j","k","l",";","@","#"," ","\\","z","x","c", \
                 	"v","b","n","m",",",".","/",",","/"," "," "," "," "," "," "," "," "," "," "," ", \
		         " "," "," "," "," "," "," ","7","8","9","-","4","5","6","+","1","2","3","0"," " };

 char *scancodes_shifted[] = { "`",0x22,"!","@","\�","$","%%","^","&","*","(",")","_","+","\b","\t","Q","W","E","R","T","Y","U", \
		         "I","O","P","{","}","\n"," ","A","S","D","F","G","H","J","K","L",":","%","`","�","|","Z", \
		         "X","C","V","B","N","M","<",">","?"," "," "," "," "," "," "," "," "," "," "," "," ", \
		         " "," "," "," "," "," "," ","7","8","9","-","4","5","6","+","1","2","3","0","." };

char *keybbuf[KEYB_BUFFERSIZE];
char *keybuf=keybbuf;
size_t readcount=0;
char *keylast=keybbuf;

/*
 * Read from console
 *
 * In: size_t ignore	Ignored value
       char *buf		Buffer
       size_t size		Number of character to read
 *
 *  Returns nothing
 *
 */
void readconsole(char *buf,size_t size) {
 keybuf=keybbuf; 						/* point to start of buffer */
 readcount=0;

 while(readcount < size) {

/* if backspace, delete character */
  
  if(*keybuf == 0x8) {
   
/* overwrite character on screen */

   if(get_cursor_col() > 0) {
	   movecursor(get_cursor_row(),get_cursor_col()-1);
	   outputconsole(" ",1);
	   movecursor(get_cursor_row(),get_cursor_col()-1);	
   }

   return;
  }
 }      

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
 * called by irq1 handler see init.asm 	*/

size_t readkey(void) {
uint8_t keycode;
char c;
size_t row,col;

if((inb(0x64) & 1) == 0) return;					/* not ready */

keycode=inb(0x60);

/* if it's a break code, then do various keys */

if((keycode & 128) == 128) {
 if(keycode == LEFT_SHIFT_UP || keycode == RIGHT_SHIFT_UP) {
  shiftpressed=FALSE;
  return;
 }

 if(keycode == LEFT_ALT_UP || keycode == RIGHT_ALT_UP) {
   ctrlpressed=FALSE;
   return;
 }

 if(keycode == LEFT_CTRL_UP || keycode == RIGHT_CTRL_UP) {
   ctrlpressed=FALSE;
   return;
 }

 return;						/* ignore return codes */
}

/* for the rest of the keys, only make codes matter */

if(readcount++ == KEYB_BUFFERSIZE) return;				/* buffer full - must reset count see above */

switch(keycode) {								/* control characters */

 case KEY_TAB:
  kprintf("        ");
  return;

 case KEY_HOME_7:							/* move cursor to start of line */  
  movecursor(0,get_cursor_col());
  return;

 case KEY_END1:								/* move cursor to end of line */
  col=strlen(keybbuf);
  movecursor(25,get_cursor_col());
  return;

 case LEFT_SHIFT:
  shiftpressed=TRUE;
  readcount--;					/* decrement readcount because shift shouldn't count as a character on it's own */
  return;
 
 case RIGHT_SHIFT:
  shiftpressed=TRUE;
  readcount--;				/* see above comment */
  return;

 case KEY_ALT:
  altpressed=TRUE;
  readcount--;				/* see above comment */
  return;

 case KEY_BACK:
  *keybuf=8;
  return;							
 
 case KEY_CAPSLOCK:
  if(capson == TRUE) {
   capson=FALSE;
  }
  else
  {
   capson=TRUE;
  }

  readcount--;				/* see above comment */
  return;

 case KEY_CTRL:									/* ctrl characters and ctrl alt del */
  keycode=inb(0x60);
  ctrlpressed=TRUE;
  readcount--;				/* see above comment */
  return;
}

if(ctrlpressed == TRUE) { 							/* control characters */
 switch(keycode) {
  case KEY_SINGQUOTE_AT:
   *keybuf++=CTRL_AT;
   kprintf("^@");
   return;

    case KEY_A:
     *keybuf++=CTRL_A;
     kprintf("^A");
     return;
   
     case KEY_B:
      *keybuf++=CTRL_B;
      kprintf("^B");
      return;

     case KEY_C:
      kprintf("^C");

      sendsignal(getpid(),SIGTERM);		/* send signal to terminate signal */
      return;

      case KEY_D:
       *keybuf++=CTRL_D;
       kprintf("^D");
       return;

      case KEY_E:
       *keybuf++=CTRL_E;
       kprintf("^E");
	return;

      case KEY_F:
       *keybuf++=CTRL_F;
       kprintf("^F");
       return;

       case KEY_G:
       *keybuf++=CTRL_G;
       kprintf("^G");
       return;


      case KEY_I:								/* tab */
       *keybuf++="\t";
       kprintf("\t");
        return;

      case KEY_J:
       *keybuf++=CTRL_J;
       kprintf("^J");
       return;

      case KEY_K:
       *keybuf++=CTRL_K;
       kprintf("^K");
       return;

      case KEY_L:
       *keybuf++=CTRL_L;
        kprintf("^L");
        return;

      case KEY_M:
       *keybuf++=CTRL_M;
	kprintf("^M");
	return;

      case KEY_N:
       *keybuf++=CTRL_N;
       kprintf("^N");
       return;

      case KEY_O:
       *keybuf++=CTRL_O;
       kprintf("^O");
       return;

      case KEY_P:
       *keybuf++=CTRL_P;
       kprintf("^P");
      return;


      case KEY_Q:
       *keybuf++=CTRL_Q;
       kprintf("^Q");
       return;

      case KEY_R:
       *keybuf++=CTRL_R;
       kprintf("^R");
       return;

      case KEY_S:
       *keybuf++=CTRL_S;
       kprintf("^S");
       return;

      case KEY_T:
       *keybuf++=CTRL_T;
       kprintf("^T");
       return;

      case KEY_V:
       *keybuf++=CTRL_V;
       kprintf("^V");
       return;

      case KEY_W:
       *keybuf++=CTRL_W;
       kprintf("^W");
       return;

      case KEY_X:
       *keybuf++=CTRL_X;
        kprintf("^X");
        return;

      case KEY_Y:
       *keybuf++=CTRL_Y;
       kprintf("^Y");
       return;

      case KEY_Z:
       *keybuf++=CTRL_Z;
       return;

     }

    }

  if(ctrlpressed == TRUE && altpressed == TRUE && keycode == KEY_DEL) {
   kprintf("CTRL-ALT-DEL\n\n");
   shutdown(1);
  }

/* if caps lock in on only shift letters */

  if(capson == TRUE) {
   c=*scancodes_shifted[keycode];		/* get character */

   if((c > 'A'-1 && c < 'Z'+1) && shiftpressed == TRUE) {
    c=c+32;				/* to lowercase */
    *keybuf++=c;
    kprintf(scancodes_unshifted[keycode]);
    return;
   }

   if((c > 'A'-1 && c < 'Z'+1) && shiftpressed == FALSE) {
    *keybuf++=c;
    kprintf(scancodes_shifted[keycode]);
    return;
   }

  }
 
  if(shiftpressed == FALSE) {
   kprintf(scancodes_unshifted[keycode]);

   *keybuf++=*scancodes_unshifted[keycode];
  }
  else
  {

   kprintf(scancodes_shifted[keycode]);
   *keybuf++=*scancodes_shifted[keycode];
  }

return;
}

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

 capson=FALSE;
 shiftpressed=FALSE;
 ctrlpressed=FALSE;
 altpressed=FALSE;

 strcpy(&device.dname,"CON");
 device.charioread=&readconsole;
 device.chariowrite=NULL;
 device.ioctl=NULL;
 device.flags=0;
 device.data=NULL;
 device.next=NULL;

 add_char_device(&device);			/* con */

 setirqhandler(1,&readkey);		/* set irq handler */
 
 init_console_device(_READ,0,&readconsole);
 return;
}

