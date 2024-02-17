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
#include "../header/errors.h"
#include "../processmanager/mutex.h"
#include "../devicemanager/device.h"
#include "../filemanager/vfs.h"

unsigned long runcommand(char *filename,char *args,unsigned long backg);
char *get_buf_pointer(void);
size_t get_batch_mode(void);
size_t set_batch_mode(size_t bm);
void set_current_batchfile_pointer(char *b);
char *parsebuf[9][MAX_PATH];
char *batchfilebuf;
char *batchfileptr;
size_t batchmode;

/*
 * Run external command
 *
 * In: char *filename		Filename to run
	      char *args		Arguments
	      unsigned long backg	Background/foreground flag
 *
 * Returns 0 on success, -1 on error
 *
 */

unsigned long runcommand(char *filename,char *args,unsigned long backg) {
size_t errorlevel;
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
	    		if(dobatchfile(filename,args,backg) == -1) {
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
/* run .run then .bat */

b=filename;
b += strlen(filename);		/* point to next */

ext=b;

*b='.';
*++b='R';
*++b='U';
*++b='N';

if(exec(filename,args,backg) == -1) {
	b=ext;

 	*b='.';
 	*++b='B';
 	*++b='A';
 	*++b='T';

	if(dobatchfile(filename,args,backg) == -1) return(-1);
}

}

/*
 * Run batchfile
 *
 * In: filename	Filename of batch file to run
	      args	Arguments
	      flags	Status flags, 0=run in foreground,1=run in background
 *
 *  Returns 0 on success, -1 on error
 *
 */
int dobatchfile(char *filename,char *args,size_t flags) {
size_t handle;
char *buf[MAX_PATH];
char *batchfileargs[MAX_PATH];
char *bufptr;
size_t parsecount;
size_t count;
char c;
size_t filesize;

if(flags == RUN_BACKGROUND) {				/* run batch file in background */
	if(getvar("COMSPEC",buf) == -1) {			/* get command interpreter */
		kprintf("command: Missing COMSPEC variable\n");
		return(-1);
	}
	
	/* create arguments to command interpreter */

	strcpy(batchfileargs,filename);
	strcat(batchfileargs," ");
	strcat(batchfileargs,args);

	return(exec(buf,batchfileargs,flags));
}

handle=open(filename,_O_RDONLY);
if(handle == -1) return(-1);

parsecount=tokenize_line(args,parsebuf," \t");

/* get name of batchfile */

getfullpath(filename,buf);
setvar("%0",buf);

for(count=1;count<parsecount;count++) {
	bufptr=buf;

	*bufptr++='%';			/* %0 %1 etc */
	itoa(count,bufptr);

	setvar(buf,parsebuf[count]);
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

	if(c == 0x0a) {			/* if at end of line */
		doline(buf);

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
 * In: batch mode status (true or FALSE)
 *
 *  Returns nothing
 *
 */
size_t set_batch_mode(size_t bm) {
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

