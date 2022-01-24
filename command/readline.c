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
#include "command.h"
#include "../header/errors.h"
#include "../processmanager/mutex.h"
#include "../devicemanager/device.h"
#include "../filemanager/vfs.h"

unsigned long readline(unsigned long handle,char *buf,int size) {
char *bufptr;
int count=0;

bufptr=buf;				/* point to buffer */

while(1) { 
 read(stdin,bufptr,1);

 if(*bufptr == '\x8') {			/* backspace */
  bufptr--;
  *bufptr=0;
 }

 if(*bufptr == '\n') {
  *bufptr=0;
  break;
 }

 bufptr++;
}

return(NO_ERROR);
}

char *readbuf[MAX_PATH];

unsigned long readlinefromfile(unsigned int handle,char *buf) {
unsigned int count=0;
char *readlinebufptr;
unsigned int readsize=0;

if(readsize == 0) readsize=read(handle,readbuf,MAX_PATH);

readlinebufptr=readbuf;

while(readsize--) {

 if(*readlinebufptr == '\n') {		/* at end */
   memcpy(buf,readbuf,count);		/* copy to buffer */

   memcpy(readbuf,buf+readsize,readsize);	/* move remaing data to start of buffer */
   memset(buf+readsize,0,MAX_PATH-readsize);	/* and fill the rest of the buffer with zeros */

   readsize -= count;
   return;
 }

 readlinebufptr++;
}

return(NO_ERROR);
}
