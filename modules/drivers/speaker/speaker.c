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

#include "processmanager/mutex.h"
#include "device.h"
#include "vfs.h"

#define MODULE_INIT speaker_io

size_t speaker_init(char *initstring);
size_t speaker_io_write(size_t *buf,size_t len);

/*
 * Intialize speaker
 *
 * In: nothing
 *
 * Returns nothing
 *
 */
size_t speaker_init(char *initstring) {
CHARACTERDEVICE bd;

strncpy(bd.dname,"SPEAKER",MAX_PATH);		/* add char device */
bd.chariowrite=&speaker_io_write;
bd.charioread=NULL;
bd.ioctl=NULL;
bd.flags=0;
bd.data=NULL;
bd.next=NULL;

if(add_char_device(&bd) == -1) {
	kprintf_direct("speaker: Can't register character device %s: %s\n",bd.name,kstrerr(getlasterror()));
	return(-1);
}

return(0);
}

/*
 * Speaker I/O handler
 *
 * In: size_t op 	Operation (0=read, 1=write)
       size_t *buf	Buffer
       size_t len	Number of bytes to read or write
 *
 *  Returns 0 on success, -1 on error
 *
 */
size_t speaker_io_write(size_t *buf,size_t len) {
uint32_t frequency;
uint32_t notelen;
uint8_t sound;
size_t lencount;
size_t count;

for(count=0;count<len;count++) {
	 frequency=1193180 / (uint32_t) *buf++;
	 notelen=*buf++;

/* configure pit to interrupt speaker output; this will create the sound */

	 outb(0x43,0xB6);
	 outb(0x40,frequency & 0xFF);
	 outb(0x40,frequency >> 8);

	/* wait for sound to end */

	 lencount=get_tick_count()+notelen;
	 
	 while( get_tick_count() < lencount) {
	 	sound=inb(0x61);

  	 	if(sound != (sound | 3)) outb(0x61, sound | 3);
	 }

	 outb(0x61,inb(0x61) & 0xfc);
  }

return(0);
}


