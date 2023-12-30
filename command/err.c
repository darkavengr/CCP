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

char *errs[] = { "No error",\
"Invalid function",\
"File not found",\
"Path not found",\
"No handles",\
"Access denied",\
"Invalid handle",\
"Reserved 1",\
"Out of memory",\
"Reserved 2",\
"Reserved 3",\
"Unknown drive format",\
"Access error",\
"Invalid file",\
"Reserved 4",\
"Invalid drive specification",\
"Directory not empty",\
"Can't rename across drives",\
"End of directory",\
"Write protect error",\
"Invalid device",\
"Drive not ready",\
"Invalid CRC",\
"File already exists",\
"Directory is full",\
"Disk full",\
"Input past end of file",\
"Device I/O error",\
"Invalid file",\
"Invalid executable",\
"Device already exists",\
"Invalid process",\
"Invalid device",\
"Device is in use",\
"Invalid kernel module",\
"Kernel module already loaded",\
"No processes",\
"End of file",\
"No drives",\
"Seek past end",\
"Can't close device",\
"Invalid block number",\
"File already open",\
"General failure",\
"Not a directory",\
"Not implemented",\
"File in use",\
"Invalid executable format",\
"Unknown filesystem" };

char *errty[] = { "reading","writing" };
char *nolabel = { "Missing label\n" };
char *syntaxerror = { "Syntax error\n" };
char *noparams = { "No parameters\n" };
char *notbatchmode = { "Not in batch mode\n" };
char *missingleftbracket = { "Missing (\n" };
char *missingrightbracket = { "Missing )\n" };
char *allfilesdeleted = { "All files will be deleted!\n" };
char *badcommand = { "Bad command or filename\n" };
char *overwrite = { "Overwrite %s (y/n)?" };
char *filescopied = { "%d File(s) copied\n" };
char *directoryof = { "Directory of %s\n\n" };
char *filesdirectories = { "%d files(s) %d directories\n" };
char *pidheader = { "PID   Parent     Filename        Arguments\n" };
char *abortretryfail = { "\nAbort, Retry, Fail?" };
char *commandbanner = { "Command version %d.%d\n" };
char *areyousure = { "Are you sure (y/n)?" };
char *terminatebatchjob = { "\nTerminate batch job (y/n)?" };

/*
 * Display error
 *
 * In:  nothing
 *
 * Returns nothing
 */

void writeerror(void) {
unsigned long err=getlasterror();

if(err == 0) return;			/* no noerror */

kprintf("%s\n",errs[err]);
kprintf("\n");
return(0);
}
