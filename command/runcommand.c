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
#include <stddef.h>
#include "command.h"
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "string.h"

char *parsebuf[MAX_PATH][MAX_PATH];
char *batchfilebuf;
char *batchfileptr;
size_t batchmode;

/*
 * Run external command from PATH
 *
 * In:  filename	Filename to run
	args		Arguments
	backg		Background/foreground flag
 *
 * Returns 0 on success, -1 on error
 *
 */

size_t run_command_path(char *command,char *args,size_t flags) {
char *sourceptr;
char *destbuf;
char *destptr;
char *fullpath[MAX_PATH];
char *pathbuf[MAX_PATH];
char *pathptr;

/* run command in current directory */

getfullpath(command,fullpath);		/* get full path to executable */
if(runcommand(fullpath,args,flags) == 0) return(0);	/* command was successful */

/* run command in path directories */

if(GetVariableValue("PATH",destbuf) == 0) {
	destptr=destbuf;

	/* copy directory from path */
	do {
		if((char) *pathptr == ';') {	/* at end of directory name */
			*destptr++ += '\\';
			strncpy(destptr,command,MAX_PATH);	/* add filename */

			if(runcommand(destbuf,args,flags) == 0) return(0);	/* command was successful */
		}

		*destptr++=*pathptr++;
	} while((char) *pathptr != 0);
}

return(-1);
}

/*
 * Run external command
 *
 * In:  filename	Filename to run
	args		Arguments
	backg		Background/foreground flag
 *
 * Returns 0 on success, -1 on error
 *
 */

size_t runcommand(char *filename,char *args,size_t backg) {
char *filenameptr;
char *buf[MAX_PATH];
char *ext;

touppercase(filename,filename);				/* to upper case */

/* if it has extension */

filenameptr=filename;
while((char) *filenameptr != 0) {
	if((char) *filenameptr == '.') {		/* found extension */
		ext=filenameptr;			/* save pointer to filename extension */
		ext++;

		if(((char) *filenameptr++ == 'R') && ((char) *filenameptr++ == 'U') && ((char) *filenameptr++ == 'N')) {
			if(exec(filename,args,backg) == -1) {
	     			itoa(getlasterror(),buf);

				SetVariableValue("ERRORLEVEL",buf);

	    			return(-1);
	   		}

	   		return(0);
		}

	 	filenameptr=ext;

		if(((char) *filenameptr++ == 'B') && ((char) *filenameptr++ == 'A') && ((char) *filenameptr++ == 'T')) {
	    		if(run_batch_file(filename,args,backg) == -1) {
	     			itoa(getlasterror(),buf);

	     			SetVariableValue("ERRORLEVEL",buf);
	     			return(-1);
			}
		}
	       
		return(0);
	}

	filenameptr++;
}

/* run .run then .bat */

filenameptr=filename;
filenameptr += strlen(filename);		/* point to next */
ext=filenameptr;

*filenameptr='.';
*++filenameptr='R';
*++filenameptr='U';
*++filenameptr='N';

if(exec(filename,args,backg) == -1) {		/* if it's not a binary executable, add .BAT extension and attempt to run it */
	filenameptr=ext;

	*filenameptr='.';
	*++filenameptr='B';
	*++filenameptr='A';
	*++filenameptr='T';

	if(run_batch_file(filename,args,backg) == -1) return(-1);
}

return(0);
}

/*
 * Run batch file
 *
 * In: filename	Filename of batch file to run
	      args	Arguments
	      flags	Status flags, 0=run in foreground,1=run in background
 *
 *  Returns 0 on success, -1 on error
 *
 */
size_t run_batch_file(char *filename,char *args,size_t flags) {
size_t handle;
char *buf[MAX_PATH];
char *batchfileargs[MAX_PATH];
char *bufptr;
size_t parsecount;
size_t count;
char c;
size_t filesize;

if(flags & RUN_BACKGROUND) {				/* run batch file in background */
	if(GetVariableValue("COMSPEC",buf) == -1) {			/* get command interpreter */
		kprintf_direct("command: Missing COMSPEC variable\n");
		return(-1);
	}
	
	ksnprintf(batchfileargs,"%s %s",MAX_PATH,filename,args);	/* create arguments to command interpreter */

	return(exec(buf,batchfileargs,flags));
}

handle=open(filename,O_RDONLY);
if(handle == -1) return(-1);

parsecount=tokenize_line(args,parsebuf," \t");

/* get name of batch file */

getfullpath(filename,buf);
SetVariableValue("%0",buf);

for(count=1;count < parsecount;count++) {
	bufptr=buf;

	*bufptr++='%';			/* %0 %1 etc */
	itoa(count,bufptr);

	SetVariableValue(buf,parsebuf[count]);
}

filesize=getfilesize(handle);

batchfilebuf=alloc(filesize);		/* allocate buffer for batchfile */
if(batchfilebuf == NULL) {
	set_batch_mode(FALSE);			/* set batch mode */
	return(-1);
}

batchfileptr=batchfilebuf;

if(read(handle,batchfilebuf,filesize) == -1) return(-1);	/* read batchfile to buffer */

memset(buf,0,MAX_PATH);
bufptr=buf;

set_batch_mode(TRUE);			/* set batch mode */

while(*batchfileptr != 0) {
	c=*batchfileptr;
	*bufptr++=*batchfileptr++;

	if(c == 0xA) {			/* if at end of line */
		ExecuteCommand(buf);

		if(get_batch_mode() == TERMINATING) {	/* batch job was killed */
			close(handle);
			set_batch_mode(FALSE);			/* set batch mode */

			return(NO_ERROR);
	 	}

	 	bufptr=buf;
	 	memset(buf,0,MAX_PATH);
	}

}

set_batch_mode(FALSE);			/* set batch mode */
return(NO_ERROR);
}

/*
 * Get batch file buffer pointer
 *
 * In: nothing
 *
 *  Returns pointer to batchfile buffer
 *
 */
char *get_buf_pointer(void) {
return(batchfileptr);
}

/*
 * Get batch mode status
 *
 * In: nothing
 *
 *  Returns true or FALSE
 *
 */
size_t get_batch_mode(void) {
return(batchmode);
}

/*
 * Set batch mode status
 *
 * In: batch mode status (TRUE or FALSE)
 *
 *  Returns nothing
 *
 */
void set_batch_mode(size_t bm) {
batchmode=bm;
}


/*
 * Set current batch pointer
 *
 * In: current batch pointer
 *
 *  Returns nothing
 *
 */
void set_current_batchfile_pointer(char *b) {
batchfileptr=b;
}

