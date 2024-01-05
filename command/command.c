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

/*  CCP Command interpreter */

/*
	  CD		 *T
	  COPY		 *T   
	  DEL		 *T
	  DIR		 *T
	  ECHO		 *T
	  EXIT		 *T
	  GOTO		 *
	  IF		 *T
	  MKDIR	 	 *T
	  REM		 *T
	  RENAME	 *T
	  RMDIR	 	 *T
	  SET   	 *T
	  TYPE		 *T
	  PS		 *T
	  KILL		 *T
	  FOR		 *
*/
#include <stdint.h>
#include "../header/errors.h"
#include "command.h"
#include "../processmanager/mutex.h"
#include "../devicemanager/device.h"
#include "../filemanager/vfs.h"
#include "../processmanager/process.h"
#include "../processmanager/signal.h"

FILERECORD direntry;
extern char *errs[255];
extern char *directories[26][MAX_PATH];

unsigned long runcommand(char *fname,char *args,unsigned long backg);

unsigned long doline(char *buf);
unsigned long copyfile(char *source,char *destination);
unsigned long getvar(char *name,char *buf);
unsigned long runcommand(char *fname,char *args,unsigned long backg);
unsigned long setvar(char *name,char *val);
unsigned long readline(unsigned long handle,char *buf,int size);
void write_error(void);
uint32_t tohex(uint32_t hex,char *buf);
size_t writelfn(size_t drive,size_t block,size_t entryno,char *n,char *newname);
uint8_t createlfnchecksum(char *filename);
size_t commandconsolein;
size_t commandconsoleout;


unsigned long doline(char *buf);
int set_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int if_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int cd_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int copy_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int for_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int del_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int mkdir_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int rmdir_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int rename_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int echo_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int exit_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int type_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int dir_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int ps_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int kill_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int goto_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
int critical_error_handler(char *name,size_t drive,size_t flags,size_t error);
int rem_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);
void signal_handler(size_t signalno);

extern char *errty[];
extern char *nolabel;
extern char *syntaxerror;
extern char *noparams;
extern char *notbatchmode;
extern char *missingleftbracket;
extern char *missingrightbracket;
extern char *allfilesdeleted;
extern char *badcommand;
extern char *overwrite;
extern char *filescopied;
extern char *directoryof;
extern char *filesdirectories;
extern char *pidheader;
extern char *abortretryfail;
extern char *commandbanner;
extern char *areyousure;
extern char *terminatebatchjob;
size_t dirattribs;

int critical_error_handler(char *name,size_t drive,size_t flags,size_t error);

struct {
	char *statement;
	size_t (*call_statement)(int,void *);		/* function pointer */
} statements[] = { {  "DIR",&dir_statement },\
		   {  "SET",&set_statement },\
		   {  "IF",&if_statement },\
		   {  "CD",&cd_statement },\
		   {  "COPY",&copy_statement },\
		   {  "FOR",&for_statement },\
		   {  "MKDIR",&mkdir_statement },\
		   {  "RMDIR",&rmdir_statement },\
		   {  "RENAME",&rename_statement },\
		   {  "ECHO",&echo_statement },\
		   {  "EXIT",&exit_statement },\
		   {  "TYPE",&type_statement },\
		   {  "PS",&ps_statement },\
		   {  "KILL",&kill_statement },\
		   {  "GOTO",&goto_statement },\
		   {  "DEL",&del_statement },\	
		   {  "REM",&rem_statement },\	
		   {  NULL,NULL } };

size_t errorlevel;
uint32_t tohex(uint32_t hex,char *buf);
size_t handle;
size_t commandlineoptions=0;

char *directories[26][MAX_PATH];

/* signal handler */

void signalhandler(size_t signal) {
char c;

	switch(signal) {
		case SIGHUP:
			kprintf("command: Caught signal SIGHUP. Terminating\n");
			exit(0);
			break;	

		case SIGQUIT:
			kprintf("command: Caught signal SIGQUIT. Terminating\n");
			exit(0);
			break;

		case SIGILL:
			kprintf("command: Caught signal SIGILL. Terminating\n");
			exit(0);
			break;

		case SIGABRT:
			kprintf("command: Caught signal SIGABRT. Terminating\n");
			exit(0);
			break;

		case SIGFPE:
			kprintf("command: Caught signal SIGFPE. Terminating\n");
			exit(0);
			break;

		case SIGKILL:
			kprintf("command: Caught signal SIGKILL. Terminating\n");
			exit(0);
			break;

	case SIGSEGV:
			kprintf("command: Caught signal SIGHUP. Terminating\n");
			exit(0);
			break;

	case SIGPIPE:
			kprintf("command: Caught signal SIGPIPE.\n");
			break;

	case SIGALRM:
			kprintf("command: Caught signal SIGALRM.\n");
			break;

	case SIGINT:
		/* fall through to SIGTERM */
	case SIGTERM:
			if(get_batch_mode() == TRUE) {		/* running in batch mode */
				/* ask user to terminate batch job */

				while(1) {  
					kprintf(terminatebatchjob);

					read(stdin,&c,1);
	
		 			if(c == 'Y' || c == 'y') break;
		 			if(c == 'N' || c == 'n') return;
				}
		
				/* set the batch mode to terminating. do_script checks for this flag and terminates if it is present */

				set_batch_mode(TERMINATING);		/* set batch mode */
				return;
	    			}
			

			break;

	case SIGCONT:
			kprintf("command: Caught signal SIGCONT.\n");
			break;

	case SIGSTOP:
			kprintf("command: Caught signal SIGSTOP.\n");
			break;
	}	
}

unsigned long main(void)
{
unsigned long count;
char *buffer[MAX_PATH];
char c;
unsigned char *b;
unsigned char *d;
char *commandlinearguments[10][MAX_PATH];
int argcount;
commandlineoptions=0;

struct psp {
	uint8_t slack[128];
	uint8_t cmdlinesize;			/* command line size */
	uint8_t commandline[127];			/* command line */
} *psp=0;

/* get and parse command line arguments */

kprintf(commandbanner,COMMAND_VERSION_MAJOR,COMMAND_VERSION_MINOR);

argcount=tokenize_line(psp->commandline,commandlinearguments," \t");		/* tokenize command line arguments */

if(argcount >= 2) {
	for(count=1;count<argcount;count++) {
		if(strcmpi(commandlinearguments[count],"/C") == 0) {		/* run command and exit */
			doline(commandlinearguments[count+1]);
			exit(0);
		}

		if(strcmpi(commandlinearguments[count],"/K") == 0) {		/* run command */
			doline(commandlinearguments[count+1]);
	
		}
	
		if(strcmpi(commandlinearguments[count],"/P") == 0) {		/* Make command interpreter pemenant */
			commandlineoptions |= COMMAND_PERMENANT;
		}	
		
		if(strcmpi(commandlinearguments[count],"/?") == 0) {		/* Show help */
			kprintf("Command interpreter\n");
			kprintf("\n");
			kprintf("COMMAND [/C command] [/K command] /P\n");
			kprintf("\n");
			kprintf("/C run command and exit\n");
			kprintf("\n");
			kprintf("/K run command and continue running command interpreter\n");
			kprintf("\n");
			kprintf("/P make command interpreter permanent (no exit)\n");
		
			count++;
		}
	}
}

set_critical_error_handler(&critical_error_handler);		/* set critical error handler */
set_signal_handler(signalhandler);			/* set signal handler */

set_batch_mode(FALSE);			/* set batch mode */

commandconsolein=stdin;
commandconsoleout=stdout;

c='A';

for(count=0;count<26;count++) {		/* fill directory struct */
	b=directories[count];

 *b++=c;
 *b++=':';
 *b++='\\';
 *b++=0;

	c++;
}

	/* Loop  forever accepting commands */

	while(1) {	/* forever */
		getcwd(buffer); 

		c=*buffer;
		kprintf("%c>",c);

		memset(buffer,0,MAX_PATH);

		readline(commandconsolein,buffer,MAX_PATH);			/* get line */

		if(*buffer) doline(buffer);
	}
}


/* do command */

unsigned long doline(char *command) {
unsigned long count;
char *buffer[MAX_PATH];
char *temp[MAX_PATH];
unsigned long xdir;
unsigned long backg;
unsigned char *b;
unsigned char *d;
size_t findresult;
char x;
int savepos;
int tc;
char *parsebuf[COMMAND_TOKEN_COUNT][MAX_PATH];
int statementcount;
char c;
char firstchar;
char lastchar;
size_t drivenumber;

if(strcmp(command,"\n") == 0) return;	/* blank line */

b=command+strlen(command)-1;
if(*b == '\n') *b=0;		/* remove newline */

memset(parsebuf,0,COMMAND_TOKEN_COUNT*MAX_PATH);

tc=tokenize_line(command,parsebuf," \t");	

touppercase(parsebuf[0]);		/* convert to uppercase */
	
for(count=1;count<tc;count++) {	/* replace variables with value */

	 firstchar=*parsebuf[count];	

	 if(firstchar == '%') {		/* variable */
	 	b=parsebuf[count];
	 	b += strlen(parsebuf[count])-1;

	 	if(*b == '%') *b=0;

	 	b=parsebuf[count];
	 	b++;
	  
		if(getvar(b,temp) != -1) {
	   		strcpy(parsebuf[count],temp); 			/* replace with value */
	  	}
	  	else
	  	{
	  	 *parsebuf[count]=0;
	  	}
	 }
	 else if(firstchar == '%' && (lastchar >= '0' || lastchar <= '9')) {	/* command-line parameter */
	 	if(getvar(parsebuf[count],temp) != -1) {
	 		strcpy(parsebuf[count],temp); 			/* replace with value */
	 	}
	 	else
	 	{
	 	 *parsebuf[count]=0;
	 	}    
	 }
}


/* redirection */
	 
for(count=0;count<tc;count++) {

	 if(strcmp(parsebuf[count],"<") == 0) {		/* input redir */
	 	handle=open(parsebuf[count+1],_O_RDONLY);
	 
	 	if(handle == -1) {			/* can't open */
	 		writeerror();
	 		return;
	   	}

	  	dup2(stdin,handle); /* redirect output */

	   	xdir=count;

	    	while(xdir < tc) {						/* overwrite redir */
	    		if(strcmp(parsebuf[xdir],">") == 0 || strcmp(parsebuf[count],">>") == 0 ) break;	/* second redirect */
			strcpy(parsebuf[xdir],parsebuf[xdir-1]);
	    	}

	    	tc=tc-(tc-xdir);		
	    	break;   
	  }

	  if(strcmp(parsebuf[count],">") == 0 || strcmp(parsebuf[count],">>") == 0 ) {		/* output redir */
	  	handle=open(parsebuf[count+1],_O_RDWR);	
	    
	  	if(handle == -1) {			/* can't open */
	    		writeerror();
	     		return;
	    	}
	 
	   	if(strcmp(parsebuf[count],">>") == 0) seek(handle,getfilesize(handle),SEEK_SET);	/* seek to end */
	  
		dup2(stdout,handle); /* redirect output */
	   	xdir=count;

	   	while(xdir < tc) {						/* overwrite redir */
			if(strcmp(parsebuf[xdir],"<") == 0) break;	/* second redirect */

	    		strcpy(parsebuf[xdir],parsebuf[xdir-1]);
	  	}

	   	tc=tc-(tc-xdir);		
	   	break;   
	 }

} 


b=parsebuf[0];
b++;

if(*b == ':' && strlen(parsebuf[0]) == 2) {
	drivenumber=(char) *parsebuf[0]-'A';		/* get drive number */

	chdir(directories[drivenumber]);

	writeerror();
	return;	
}


/* execute if it's an internal command */

statementcount=0;

do {
	touppercase(parsebuf[0]);

	if(strcmp(statements[statementcount].statement,parsebuf[0]) == 0) {  /* found statement */
		statements[statementcount].call_statement(tc,parsebuf);	
		statementcount=0;
		return;
	}
	
	statementcount++;

} while(statements[statementcount].statement != NULL);

/* if external command */

if(strcmp(parsebuf[tc],"&") == 0) {	/* run in background */
	tc--;

	backg=1;
}
else
{
	backg=0;
}

/* don't put anything here, drop through to below */

if(tc > 1) {						/* copy args if any */
	memset(temp,0,MAX_PATH);

	for(count=1;count<tc;count++) {				/* get args */
		strcat(temp,parsebuf[count]);
		if(count < tc) strcat(temp," ");
	}

	strtrunc(temp,1);

}

/* run program or script in current directory */
if(runcommand(parsebuf[0],temp,backg) != -1) return;	/* run ok */

/* prepend each directory in PATH to filename in turn and execute */

if(getvar("PATH",d) != -1) {			/* get path */
	b=temp;

	/* copy each directory in path in turn, append filename and execute */

	while(*d != 0) {
	 	*b++=*d++;    
	
		if(*b == ';') {			/* seperator */ 
			b--;
   			*b++='\\';
   
  			memcpy(b,parsebuf[0],strlen(parsebuf[0]));		/* append filename */
	   
			if(runcommand(parsebuf[1],temp,backg) != -1) return;	/* run ok */

   			b=temp;
  		}
 	}

}

/* bad command */

kprintf(badcommand);

setvar("ERRORLEVEL","255");
return(-1);
}

int set_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
char *varptr=getenv();
char *buffer[MAX_PATH];
char *b;

if(tc == 1) {			/* display variables */
	while((size_t) *varptr != NULL) {
		b=buffer;

	/* copy variable and value */

		while(*varptr != 0) {
		 *b++=*varptr++;
		}

		kprintf("%s\n",buffer);
	}

return;
}

/* set variable */
setvar(parsebuf[1],parsebuf[3]);
return;
}

int if_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t start;
size_t inverse;
size_t condition;
char *buffer[MAX_PATH];

size_t count;

if(tc == 1) {			/* not enough args */
	kprintf(noparams);
	return;
}

start=1;

if(strcmp(parsebuf[1],"NOT")) {
	inverse=FALSE;
	start++;
}

if(strcmp(parsebuf[start],"EXIST") == 0) {
	if(inverse == FALSE) {
		if(getfileattributes(parsebuf[start+1]) != -1) {
			condition=TRUE;
		}
		else
		{
	 		condition=FALSE;
		}
	 
		start++;
	}
}
else
{
/* find == */

	 for(count=start;count<tc;count++) {
	 	if(strcmp(parsebuf[count],"==") == 0) break;
	 }

	 if(count >= tc) {	/* no == */
	 	kprintf(syntaxerror); 
	 	return;
	 }

	 if(strcmp(parsebuf[count-1],parsebuf[count+1]) == 0) condition=TRUE;
}

start=count+2;		/* save start of command */

/* copy tokens to buffer */
if(condition == TRUE) {
	for(count=start;count<tc;count++) {				/* get args */
		strcat(buffer,parsebuf[count]);
	    
	   	if(count <= tc-1) strcat(buffer," ");
	}

	doline(buffer);
	return;
}

}

/*
 * cd: set current directory
 */

int cd_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
char *directoryname[MAX_PATH];
size_t drivenumber;

if(tc == 1) {			/* not enough args */
	getcwd(directoryname);

	kprintf("%s\n",directoryname);
	return;
}

getfullpath(parsebuf[1],directoryname);	/* get full path of directory */

if(chdir(parsebuf[1]) == -1) {		/* set directory */
	writeerror();
	return;
}

drivenumber=(char) *directoryname-'A';	/* get drive number */

strcpy(directories[drivenumber],directoryname);	/* save directory name */
return;
}


int copy_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t count;
size_t runopts;
size_t inverse;
char *b;
FILERECORD sourcedirentry;
FILERECORD destdirentry;
int findresult;
char c;
char *buffer[MAX_PATH];

if(tc == 1) {			/* not enough args */
	kprintf(noparams);
	return;
}

if(strlen(parsebuf[1]) == 0 || strlen(parsebuf[2]) == 0) { /* missing args */
	 kprintf(noparams);
	 return;
}

findresult=findfirst(parsebuf[1],&sourcedirentry);

if(findresult == -1) {	/* no source file */
	 writeerror(FILE_NOT_FOUND);
	 return;
}

count=0;

do {
	findresult=findfirst(parsebuf[2],&destdirentry);

	if((findresult != -1) && getlasterror() != (FILE_NOT_FOUND)) {
		while(1) {
			kprintf(overwrite,parsebuf[2]);
			read(stdin,&c,1);

			if(c == 'Y' || c == 'y') break;
			if(c == 'N' || c == 'n') return;
		}
	}
	 
	kprintf("%s\n",sourcedirentry.filename);
	count++;

	if(direntry.flags == FILE_DIRECTORY) {	/* copying to directory */
		ksprintf(buffer,"%s\\%s",parsebuf[2],&direntry.filename);
	}

	if(copyfile(sourcedirentry.filename,parsebuf[2]) == -1) {
		writeerror();
		return;
	}

} while(findnext(parsebuf[1],&sourcedirentry) != -1);

kprintf(filescopied,count);

return;
}


int for_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t start;
size_t count;
size_t startvars;
size_t endvars;
char *b;
char *buffer[MAX_PATH];
char *tempbuffer[MAX_PATH];
char c;

//  0   1 2   3    4   5  6
//for %a in (*.*) do echo %a

if(strcmpi(parsebuf[2],"IN") != 0) {
	kprintf(syntaxerror);
	return;
}

for(start=3;start<tc;start++) {		/* find start */
	touppercase(parsebuf[start]);

	if(strcmpi(parsebuf[start],"do") == 0) {
		ksprintf(buffer,"%s ",parsebuf[start+1]);

		for(count=start+2;count<tc;count++) {				/* get args */
			if(strcmpi(parsebuf[count],")") == 0) break;			/* at end */

			strcat(buffer,parsebuf[count]);
			strcat(buffer," ");
	   	}

	  	break;
	 }   
}

if(start == tc) {
	kprintf(syntaxerror);
	return;
}
	
startvars=3;
endvars=start-1;

/* remove ( and ) */

if(strcmp(parsebuf[startvars],"(") == 0) {
	 startvars++;			/* skip ( */
}
else
{
	c=*parsebuf[startvars];

	if(c != '(') {
		kprintf(missingleftbracket);
		return;
	}
	else
	{
		/* copy start variable without ( */

		b=parsebuf[startvars];
		b++;

		strcpy(tempbuffer,b);
		strcpy(parsebuf[startvars],tempbuffer);
	}
}

if(strcmp(parsebuf[start-1],")") == 0) {
	endvars--;			/* skip ( */
}
else
{
	b=parsebuf[start-1];
	b += strlen(parsebuf[start-1]);
	b--;

	c=*b;

	if(c != ')') {
		kprintf(missingrightbracket);
		return;
	 }
	 else
	 {
	 	/* end variable without ) */

	  *b=0;
	 }
}

for(count=startvars;count<endvars+1;count++) {
	setvar(parsebuf[1],parsebuf[count]);

	doline(buffer);
}

return;
}
	

/*
 * del: delete file
 *
*/

int del_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t inverse;
int runopts;
char c;

if(tc == 1) {			/* not enough args */
	kprintf(noparams);
	return;
}

if(strcmp(parsebuf[1],"*") == 0 || strcmp(parsebuf[1],"*.*") == 0) {
	kprintf(allfilesdeleted);

	while(1) {  
		kprintf(areyousure);
		read(stdin,&c,1);
	
		if(c == 'Y' || c == 'y') break;
		if(c == 'N' || c == 'n') return;
	 }
}

if(delete(parsebuf[1]) == -1) writeerror();

return;
}


int mkdir_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
if(tc < 2) {			/* not enough args */
	kprintf(noparams);
	return;
}

if(mkdir(parsebuf[1]) == -1) {		/* set directory */
	writeerror();
	return;
}

return;
}


int rmdir_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
if(tc < 2) {			/* not enough args */
	kprintf(noparams);
	return;
}

if(rmdir(parsebuf[1]) == -1) {		/* set directory */
	writeerror();
	return;
}

return;
}



int rename_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
if(tc < 2) {			/* not enough args */
	kprintf(noparams);
	return;
}

if(rename(parsebuf[1],parsebuf[2]) == -1) {		/* set directory */
	writeerror();
	return;
}

return;
}

/*
 * echo: display text
 */

int echo_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t count;

if(tc == 1) {			/* not enough args */
	kprintf("\n");
	return;
}

for(count=1;count<tc;count++) {
	 kprintf("%s ",parsebuf[count]);
}

return;
}

/*
 * exit: terminate command
 */

int exit_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
	if((commandlineoptions & COMMAND_PERMENANT) == 0) exit(0);
}

/*
 * type: display file
 *
 * type [file]
 *
 */


int type_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
char *readbuf;
size_t handle;
int findresult;

if(tc == 1) {			/* not enough args */
	kprintf("\n");
	return;
}

readbuf=alloc(MAX_READ_SIZE);		/* allocate buffer */

if(readbuf == NULL) {			/* can't allocate */
	writeerror();
	return;
}

handle=open(parsebuf[1],_O_RDONLY);

if(handle == -1) {				/* can't open */
	writeerror();
	return;
}
	
/* read until end of file */
	
findresult=0;

while(findresult != -1) {	 
	 findresult=read(handle,readbuf,MAX_READ_SIZE);			/* read from file */
	 if(findresult == -1) {				/* can't read */
	 	if(getlasterror() != END_OF_FILE) {
	 		writeerror();
	 		break;
	 	}
	 }

	 kprintf(readbuf);

}
	

free(readbuf);

close(handle);
return;
}

/*
 * dir: list directory 
 *
 * dir [filespec]
 *
 */

int dir_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t findresult;
char *b;
char *buffer[MAX_PATH];
size_t dircount;
size_t fcount;
char *z[MAX_PATH];

if(!*parsebuf[1]) strcpy(parsebuf[1],"*");	/* find all by default */

getfullpath(parsebuf[1],buffer);

/* find end of name */

b=buffer+strlen(buffer);

while(*b-- != '\\') ;;

b=b+2;
*b=0;

kprintf(directoryof,buffer);
	
memset(&direntry.filename,0,MAX_PATH);

findresult=findfirst(parsebuf[1],&direntry);

if(findresult == -1) {
	if(getlasterror() != END_OF_DIR) writeerror();
	return;
}


dircount=0;
fcount=0;

while(findresult != -1) {

/*time date size filename */

	  kprintf("%u:%u:%u %u/%u/%u",direntry.last_written_time_date.hours,direntry.last_written_time_date.minutes,direntry.last_written_time_date.seconds,\
			        direntry.last_written_time_date.day,direntry.last_written_time_date.month,direntry.last_written_time_date.year);

	  if(direntry.flags & FILE_DIRECTORY) {
	  	kprintf("     <DIR> ");

		dircount++;
	  }
	  else
	  {
	  	itoa(direntry.filesize,buffer);		/* pad out file size */		
	  	memset(z,0,10);
	  	memset(z,' ',10-strlen(buffer));
	  	kprintf(z);

	  	kprintf("%d ",direntry.filesize);
	 }

	  kprintf("%s\n",direntry.filename);
	  fcount++;

	  memset(&direntry,sizeof(FILERECORD));

	  findresult=findnext(parsebuf[1],&direntry);
	  if(findresult == -1) break;
}

ksprintf(z,"%d",getlasterror()); 
setvar("ERRORLEVEL",z);

if(getlasterror() != END_OF_DIR) {
	writeerror();
	return;
}

kprintf(filesdirectories,fcount,dircount);

return;
}

int ps_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t findresult;
PROCESS pbuf;

findresult=findfirstprocess(&pbuf);		/* get first process */
if(findresult == -1) return;		/* no processes */

kprintf(pidheader);

while(findresult != -1) {
	kprintf("%u    %u          %s             %s\n",pbuf.pid,pbuf.parentprocess,pbuf.filename,pbuf.args);

	findresult=findnextprocess(&pbuf);		/* get first process */
	if(findresult == -1) return;		/* no processes */
}

return;
}

int kill_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
int findresult;

if(tc == 1) {		/* missing argument */
	kprintf(noparams);
	return;
}
	 
findresult=kill(atoi(parsebuf[1],10));
if(findresult == -1) writeerror();
return;
}

int goto_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
char *buffer[MAX_PATH];
char *bufptr;
char *b;

if(tc == 1) {		/* missing label */
	kprintf(nolabel);
	return;
}

if(get_batch_mode() == FALSE) {			/* Not batch mode */
	kprintf(notbatchmode);
	return;
}

bufptr=get_buf_pointer();		/* get script buffer */
b=buffer;

while(*bufptr != 0) {
	*b++=*bufptr;			/* copy line data */
	
	if(*bufptr == 0x0A) {		/* at end of line */
		b=buffer;
		bufptr=buffer;

		if(*b == ':') {		/* is label */
			b++;
	
			if(strcmp(b,parsebuf[1]) == 0) {	/* found label */	
				set_current_batchfile_pointer(b);
	        		return;
	       		}
	 	}
	}

	bufptr++;
}

/* label not found */

kprintf(nolabel);  
return(-1);
}

int rem_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
}

int critical_error_handler(char *name,size_t drive,size_t flags,size_t error) {
CHARACTERDEVICE *cd;
BLOCKDEVICE *bd;
char c;
char *buf[10];
char *b;

if((flags & 0x80000000) == 0) {			/* from disk */
	 flags &= 0x80000000;
	 kprintf("\n%s %s Drive %c\n",errs[error],errty[flags],drive+'A');
}
else
{
	 kprintf("\n%s %s %s\n",errs[error],errty[flags],name);
}

while(1) {
	  kprintf(abortretryfail);
	  read(stdin,buf,1);

	  b=buf;
	  c=*buf;

	  if(c == 'A' || c == 'a') return(CRITICAL_ERROR_ABORT);
	  if(c == 'R' || c == 'r') return(CRITICAL_ERROR_RETRY);
	  if(c == 'F' || c == 'f') return(CRITICAL_ERROR_FAIL);
}

}

