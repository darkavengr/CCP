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

char *parsebuf[9][MAX_PATH];

unsigned long runcommand(char *filename,char *args,unsigned long backg) {
char *z[10];
unsigned int errorlevel;
char *b;
char *buf[MAX_PATH];
char *ext;

touppercase(filename);				/* to upper case */

/* if it has extension */

b=filename;
while(*b != 0) {
 if(*b == '.') {		/* found extension */
  ext=b;
  ext++;

	  if(*b++ == 'R' && *b++ == 'U' && *b++ == 'N') {
	   if(exec(filename,args,backg) == -1) {
	    errorlevel=getlasterror();
	    itoa(errorlevel,buf);
	    setvar("ERRORLEVEL",buf);
	    return(-1);
	    }

	   return(0);
          }

b=ext;

	  if(*b++ == 'B' && *b++ == 'A' && *b++ == 'T') {
	    if(doscript(filename,args) == -1) {
	     errorlevel=getlasterror();
	     itoa(errorlevel,buf);
	     setvar("ERRORLEVEL",buf);
	     return(-1);
	    }
        
	    return(0);
	   }


  break;
 }

b++;
}
/* run .run then .scp */

b=filename+strlen(filename);		/* point to next */
*b='.';
*++b='R';
*++b='U';
*++b='N';

if(exec(filename,args,backg) == -1) return(-1);

b=filename+strlen(filename);		/* point to next */
*b='.';
*++b='B';
*++b='A';
*++b='T';

doscript(filename,args);
}

int doscript(char *filename,char *args) {
unsigned int handle;
char *buf[MAX_PATH];
char *bptr;
char *z[10];
unsigned int parsecount;
unsigned int count;

handle=open(filename,_O_RDONLY);
if(handle == -1) return(-1);

parsecount=tokenize_line(args,parsebuf," \t");

setvar("%0",filename);

for(count=1;count<parsecount;count++) {
 strcpy(buf,"%");		/* %0 %1 etc */
 bptr=buf;
 bptr++;
 itoa(count,bptr);

 setvar(z,filename);
}

while(readlinefromfile(handle,buf,MAX_PATH) != -1) {
 doline(buf);
}

 return(NO_ERROR);
}
