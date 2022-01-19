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

char *errs[] = { "No error",\
"Invalid function",\
"File not found",\
"Path not found",\
"No handles",\
"Access denied",\
"Bad handle",\
"Reserved 1",\
"No mem",\
"Reserved 2",\
"Reserved 3",\
"Bad disk",\
"Access error",\
"Bad file",\
"Reserved 4",\
"Invalid drive",\
"Directory has files",\
"Rename across drive",\
"End of directory",\
"Write protect error",\
"Bad device",\
"Brive not ready",\
"Bad CRC",\
"File exists",\
"Directory full",\
"Disk full",\
"Input past end",\
"Device I/O error",\
"Bad file",\
"Bad executable",\
"Device already exists",\
"Invalid process",\
"Bad device",\
"Device in use",\
"Invalid driver",\
"Driver already loaded",\
"No processes",\
"End of file",\
"No drives",\
"Seek past end",\
"Can't close device",\
"Invalid block number",\
"File already open",\
"General failure",\
"Not a directory",\
"Not implemented" };

void writeerror(void) {
 unsigned long err=getlasterror();

 if(err == 0) return;			/* no noerror */

 kprintf("%s\n",errs[err]);
 kprintf("\n");
 return(0);
}
