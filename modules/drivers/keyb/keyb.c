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

/* CCP keyboard driver */

#include <stdint.h>
#include "../../../header/errors.h"
#include "../../../devicemanager/device.h"
#include "keyb.h"

#define KERNEL_HIGH (1 << (sizeof(unsigned int)*8)-1)

#define MODULE_INIT keyb_init

void readconsole(unsigned int ignore,char *buf,size_t size);
size_t readkey(void);
void keyb_init(void);

uint32_t capson=FALSE;
uint32_t shiftpressed=FALSE;
uint32_t ctrlpressed=FALSE;
uint32_t altpressed=FALSE;

/* characters from scan codes */
char *scancodes_unshifted[] = { "`",0x22,"1","2","3","4","5","6","7","8","9","0","-","="," ","\t","q","w","e","r","t","y","u", \
		         "i","o","p","[","]","\n"," ","a","s","d","f","g","h","j","k","l",";","@","#"," ","\\","z","x","c", \
                 	"v","b","n","m",",",".","?",",","/"," "," "," "," "," "," "," "," "," "," "," ", \
		         " "," "," "," "," "," "," ","7","8","9","-","4","5","6","+","1","2","3","0"," " };

 char *scancodes_shifted[] = { "`",0x22,"!","@","\£","$","%%","^","&","*","(",")","_","+","\b","\t","Q","W","E","R","T","Y","U", \
		         "I","O","P","{","}","\n"," ","A","S","D","F","G","H","J","K","L",":","%","`","£","|","Z", \
		         "X","C","V","B","N","M","<",">","?"," "," "," "," "," "," "," "," "," "," "," "," ", \
		         " "," "," "," "," "," "," ","7","8","9","-","4","5","6","+","1","2","3","0","." };

unsigned int consoleio(unsigned int op,char *s,size_t size);
void readconsole(unsigned int ignore,char *buf,size_t size);
void keyb_init(void);

char *keybbuf[KEYB_BUFFERSIZE];
char *keybuf=keybbuf;
size_t readcount=0;
char *keylast=keybbuf;

void readconsole(unsigned int ignore,char *buf,size_t size) {
 keybuf=keybbuf; 						/* point to start of buffer */
 readcount=0;

 while(readcount < size) ;;

 memcpy(buf,keybbuf,size);				
 readcount=0;
 
 return;
}

/* called by irq1 handler see init.asm 	*/

size_t readkey(void) {
uint8_t x;
uint8_t *k=KERNEL_HIGH+0x497;
char c;
unsigned int row,col;

if((inb(0x64) & 1) == 0) return;					/* not ready */

if((*k & 64) == 64) capson=TRUE;					/* caps lock on */

x=inb(0x60);

/* if it's a break code, then do various keys */

if((x & 128) == 128) {
 if(x == LEFT_SHIFT_UP || x == RIGHT_SHIFT_UP) {
  shiftpressed=FALSE;
  return;
 }

 if(x == LEFT_ALT_UP || x == RIGHT_ALT_UP) {
   ctrlpressed=FALSE;
   return;
 }

 if(x == LEFT_CTRL_UP || x == RIGHT_CTRL_UP) {
   ctrlpressed=FALSE;
   return;
 }

 return;						/* ignore return codes */
}

/* for the rest of the keys, only make codes matter */

if(readcount++ == KEYB_BUFFERSIZE) return;				/* buffer full - must reset count see above */

getcursorpos(row,col);

switch(x) {								/* control characters */

 case KEY_TAB:
  kprintf("        ");
  return;

 case KEY_HOME_7:							/* move cursor to start of line */  
  col=0;
  movecursor(row,col);
  return;

 case KEY_END1:								/* move cursor to end of line */
  col=strlen(keybbuf);
  movecursor(row,col);
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
   col--;
   kprintf(" ");
   col--;
   movecursor(row,col);
  
  *keybuf--=0;
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
  x=inb(0x60);
  ctrlpressed=TRUE;
  readcount--;				/* see above comment */
  return;
}

if(ctrlpressed == TRUE) { 							/* control characters */
 switch(x) {
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
      exit(1);
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

  if(ctrlpressed == TRUE && altpressed == TRUE && x == KEY_DEL) {
   kprintf("CTRL-ALT-DEL\n\n");
   shutdown(1);
  }

/* if caps lock in on only shift letters */

  if(capson == TRUE) {
   c=*scancodes_shifted[x];		/* get character */

   if((c > 'A'-1 && c < 'Z'+1) && shiftpressed == TRUE) {
    c=c+32;				/* to lowercase */
    *keybuf++=c;
    kprintf(scancodes_unshifted[x]);
    return;
   }

   if((c > 'A'-1 && c < 'Z'+1) && shiftpressed == FALSE) {
    *keybuf++=c;
    kprintf(scancodes_shifted[x]);
    return;
   }

  }
 
  if(shiftpressed == FALSE) {
   kprintf(scancodes_unshifted[x]);

   *keybuf++=*scancodes_unshifted[x];
  }
  else
  {

   kprintf(scancodes_shifted[x]);
   *keybuf++=*scancodes_shifted[x];
  }

return;
}

void keyb_init(void) {
CHARACTERDEVICE device;

 capson=FALSE;
 shiftpressed=FALSE;
 ctrlpressed=FALSE;
 altpressed=FALSE;

 strcpy(&device.dname,"CON");
 device.chario=&readconsole;
 device.ioctl=NULL;
 device.flags=0;
 device.data=NULL;
 device.next=NULL;

 add_char_device(&device);			/* con */

 setirqhandler(1,&readkey);		/* set irq handler */
 return(NULL);
}

