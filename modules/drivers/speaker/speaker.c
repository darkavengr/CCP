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

#include "../../../devicemanager/device.h"
#include "../../../filemanager/vfs.h"

#define MODULE_INIT speaker_io

void speaker_init(char *initstring);
size_t speaker_io(unsigned int op,unsigned int *buf,unsigned size_t len);

void speaker_init(char *initstring) {
CHARACTERDEVICE bd;

strcpy(bd.dname,"SPEAKER");		/* add char device */
bd.chario=&speaker_io;
bd.ioctl=NULL;
bd.flags=0;
bd.data=NULL;
bd.next=NULL;

add_char_device(&bd); 

return;
}

size_t speaker_io(unsigned int op,size_t *buf,unsigned size_t len) {
uint32_t frequency;
uint32_t notelen;
uint8_t sound;
size_t lencount;
size_t count;

if(op == _WRITE) {
 for(count=0;count<len;count++) {
	 frequency=1193180 / (uint32_t) *buf++;
	 notelen=*buf++;

/* configure pit to interrupt speaker output; this will create the sound */

	 outb(0x43,0xB6);
	 outb(0x40,frequency & 0xFF);
	 outb(0x40,frequency >> 8);

	/* wait for sound to end */

	 lencount=gettickcount()+notelen;
	 
	 while( gettickcount() < lencount) {
	  sound=inb(0x61);

  	  if(sound != (sound | 3)) outb(0x61, sound | 3);
	 }

	 outb(0x61,inb(0x61) & 0xfc);
  }

 return;
}
 
return(-1);
}


