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
#include "command.h"
#include "../processmanager/mutex.h"
#include "../devicemanager/device.h"
#include "../filemanager/vfs.h"
#include "../header/errors.h"

/*
 * Copy file
 *
 * In:  char *source		File to copy from
	char *destination	File to copy to
 *
 * Returns 0 on success, -1 on error
 */

unsigned long copyfile(char *source,char *destination) {
 unsigned long sourcehandle;
 unsigned long desthandle;
 char *buf[MAX_PATH];
 char *readbuf;
 unsigned long result;
 unsigned long count;
 unsigned long countx;

 kprintf("source=%s\n",source);
 kprintf("destination=%s\n",destination);

 sourcehandle=open(source,_O_RDONLY);
 if(sourcehandle == -1) {				/* can't open */
  writeerror();
  return(-1);
 }

 desthandle=create(destination);
 if(desthandle == -1)  {
  if(getlasterror() == FILE_EXISTS) desthandle=open(destination,_O_RDWR);			/* file exists */
  if(desthandle == -1)   return(-1);

 }

 kprintf("copy file\n");

 readbuf=alloc(MAX_PATH);
 if(readbuf == -1) {
  writeerror();
  return(-1);
 }

 while((count != -1) || (countx != -1)) {
  count=read(sourcehandle,readbuf,MAX_PATH);
  kprintf("read=%d\n",count);


  if(count == -1) {
   writeerror();
   return(-1);
  }

  countx=write(desthandle,readbuf,count);

  if(countx == -1) {
   writeerror();
   return(-1);
  }

  kprintf("write=%d\n",countx);
 }

  return(0);
}  
