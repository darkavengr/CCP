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
#include <stddef.h>
#include "errors.h"
#include "command.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "memorymanager.h"
#include "process.h"
#include "signal.h"

extern char *errors[255];
extern char *directories[26][MAX_PATH];
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
size_t commandconsolein;
size_t commandconsoleout;

struct {
	char *command;
	size_t (*call_command)(int,void *);		/* function pointer */
} commands[] = { {  "DIR",&dir_command },\
		   {  "SET",&set_command },\
		   {  "IF",&if_command },\
		   {  "CD",&cd_command },\
		   {  "COPY",&copy_command },\
		   {  "FOR",&for_command },\
		   {  "MKDIR",&mkdir_command },\
		   {  "RMDIR",&rmdir_command },\
		   {  "RENAME",&rename_command },\
		   {  "ECHO",&echo_command },\
		   {  "EXIT",&exit_command },\
		   {  "TYPE",&type_command },\
		   {  "PS",&ps_command },\
		   {  "KILL",&kill_command },\
		   {  "GOTO",&goto_command },\
		   {  "DEL",&del_command },\	
		   {  "REM",&rem_command },\	
		   {  NULL,NULL } };

size_t errorlevel;
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

	case SIGTERM:
		exit(0);

	case SIGINT:
		if(get_batch_mode() == TRUE) {		/* running in batch mode */
			/* ask user to terminate batch job */

			while(1) {  
				kprintf("%s",terminatebatchjob);

				read(stdin,&c,1);
	
	 			if((c == 'Y') || (c == 'y')) break;
	 			if((c == 'N') || (c == 'n')) return(0);
			}
		
			/* set the batch mode to terminating. do_script checks for this flag and terminates if it is present */

			set_batch_mode(TERMINATING);		/* set batch mode */
			return(0);
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
} *psp=NULL;

/* get and parse command line arguments */

kprintf(commandbanner,COMMAND_VERSION_MAJOR,COMMAND_VERSION_MINOR);

argcount=tokenize_line(psp->commandline,commandlinearguments," \t");		/* tokenize command line arguments */

if(argcount >= 2) {
	for(count=1;count<argcount;count++) {
		b=commandlinearguments[count];

		if((char) *b == '/') {
			b++;

			if(((char) *b == 'C') || ((char) *b == 'c')) {
				doline(commandlinearguments[count+1]);
				exit(0);
			}
			else if(((char) *b == 'K') || ((char) *b == 'k')) {
				doline(commandlinearguments[count+1]);
			}
			else if(((char) *b == 'P') || ((char) *b == 'p')) {
				commandlineoptions |= COMMAND_PERMENANT;
				count++;	
			}			
			else if((char) *b == '?') {
				kprintf("Command interpreter\n");
				kprintf("\n");
				kprintf("COMMAND [OPTIONS] {command}\n");
				kprintf("\n");
				kprintf("/C run command and exit\n");
				kprintf("\n");
				kprintf("/K run command and continue running command interpreter\n");
				kprintf("\n");
				kprintf("/P make command interpreter permanant (no exit)\n");
		
				exit(0);
			}
			else
			{
				kprintf("command: Invalid option: /%c\n",(char) *commandlinearguments[count]);
				exit(1);
			}
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
	kprintf("%c>",c);		/* display prompt */

	memset(buffer,0,MAX_PATH);

	readline(commandconsolein,buffer,MAX_PATH);			/* get line */

	if(*buffer) doline(buffer);
}

return(0);
}


/* do command */

unsigned long doline(char *command) {
unsigned long count;
char *buffer[MAX_PATH];
char *temp[MAX_PATH];
unsigned long backg;
unsigned char *b;
unsigned char *d;
int savepos;
int tc;
char *parsebuf[COMMAND_TOKEN_COUNT][MAX_PATH];
int commandcount;
char c;
char firstchar;
char lastchar;
int drivenumber;
int pipecount;
int inputpipehandle;
int outputpipehandle;

if(strncmp(command,"\n",MAX_PATH) == 0) return(0);	/* blank line */

b=command+strlen(command)-1;
if(*b == '\n') *b=0;		/* remove newline */

memset(parsebuf,0,COMMAND_TOKEN_COUNT*MAX_PATH);

tc=tokenize_line(command,parsebuf," \t");	

touppercase(parsebuf[0],parsebuf[0]);		/* convert to uppercase */
	
for(count=1;count<tc;count++) {	/* replace variables with value */

	 firstchar=*parsebuf[count];	

	 if(firstchar == '%') {		/* variable */
	 	b=parsebuf[count];
	 	b += strlen(parsebuf[count])-1;

	 	if(*b == '%') *b=0;

	 	b=parsebuf[count];
	 	b++;
	  
		if(getvar(b,temp) != -1) {
	   		strncpy(parsebuf[count],temp); 			/* replace with value */
	  	}
	  	else
	  	{
	  	 *parsebuf[count]=0;
	  	}
	 }
	 else if(firstchar == '%' && (lastchar >= '0' || lastchar <= '9')) {	/* command-line parameter */
	 	if(getvar(parsebuf[count],temp) != -1) {
	 		strncpy(parsebuf[count],temp); 			/* replace with value */
	 	}
	 	else
	 	{
	 	 *parsebuf[count]=0;
	 	}    
	 }
}


/* redirection */
savepos=0;					/* no redirection for now */
	 
for(count=0;count<tc;count++) {

	 if(strncmp(parsebuf[count],"<",MAX_PATH) == 0) {		/* input redirection */
		if(savepos == 0) savepos=count;		/* save position of first redirect */

	 	handle=open(parsebuf[count-1],O_RDONLY);
	 	if(handle == -1) {			/* can't open */
	 		kprintf("%s\n",kstrerr(getlasterror()));
	 		return(0);
	   	}

	  	dup2(handle,stdin);	 		/* redirect input */

		continue;
	}
	else if(strncmp(parsebuf[count],">",MAX_PATH) == 0) {	/* output redirection */
		if(savepos == 0) savepos=count;		/* save position of first redirect */

	  	handle=open(parsebuf[count+1],O_WRONLY | O_CREAT | O_TRUNC);		
	  	if(handle == -1) {			/* can't open */
	    		kprintf("%s\n",kstrerr(getlasterror()));
	     		return(-1);
	    	}
	 
		dup2(handle,stdout); 		/* redirect output */

		continue;
	}
	else if(strncmp(parsebuf[count],">>",MAX_PATH) == 0) {	/* output redirection with append */
		if(savepos == 0) savepos=count;		/* save position of first redirect */

	  	handle=open(parsebuf[count+1],O_WRONLY);		
	  	if(handle == -1) {			/* can't open */
	    		kprintf("%s\n",kstrerr(getlasterror()));
	     		return(0);
	    	}
	 
		if(seek(handle,0,SEEK_END) == -1) {	/* seek to end */
	    		kprintf("%s\n",kstrerr(getlasterror()));
	     		return(0);
	    	}
		
		dup2(handle,stdout); 		/* redirect output */

		continue;
	}

} 

if(savepos != 0) {				/* if there was a redirection */
	/* remove remainder of command */

	for(count=savepos;count<tc;count++) {
		memset(parsebuf[count],0,MAX_PATH);
	}

	tc=(savepos-1);				/* new end */
}

/* pipe data */

pipecount=0;

for(count=0;count<tc;count++) {

	if(strncmp(parsebuf[count],"|",MAX_PATH) == 0) {	/* pipe command */

		/* command1 | command2 | command3 */
		if((count == 0) || (count == tc)) {	/* no command before or after */
			kprintf("%s",syntaxerror);
			return(0);
		}


		/* get command arguments */

		memset(temp,0,MAX_PATH);
		
		for(commandcount=count+1;commandcount<tc;commandcount++) {
			strncat(temp,parsebuf[commandcount],MAX_PATH);
	
			if(strncmp(parsebuf[commandcount],"|",MAX_PATH) == 0) break;
		}

		outputpipehandle=pipe();		/* create a pipe */
				
		dup2(outputpipehandle,stdout);	/* connect stdout to pipe */
		dup2(outputpipehandle,stderr);	/* connect stderr to pipe */

		if(runcommand(parsebuf[count],temp,FALSE) == -1) {		/* run command */
			kprintf("%s\n",kstrerr(getlasterror()));
			return(-1);
		}

		if(pipecount > 0) {		/* if it's not the first command */
				inputpipehandle=outputpipehandle;		/* copy output handle to input handle */				

				dup2(inputpipehandle,stdin);		/* connect stdin to pipe */
		}

		pipecount++;
	}
}

/* if changing drive */

b=parsebuf[0];
b++;

if((char) *b == ':' && strlen(parsebuf[0]) == 2) {
	drivenumber=(char) *parsebuf[0]-'A';		/* get drive number */

	if(chdir(directories[drivenumber]) == -1) {	/* change to directory for drive */
		kprintf("%s\n",kstrerr(getlasterror()));
		return(-1);	
	}

	return(0);
}

/* check if it's an internal command */

commandcount=0;

do {
	if(strncmp(commands[commandcount].command,parsebuf[0],MAX_PATH) == 0) {  /* found command */
		commands[commandcount].call_command(tc,parsebuf);	
		commandcount=0;
		return(0);
	}
	
	commandcount++;

} while(commands[commandcount].command != NULL);

/* if external command */

if(strncmp(parsebuf[tc],"&",MAX_PATH) == 0) {	/* run in background */
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
		strncat(temp,parsebuf[count],MAX_PATH);
		if(count < tc) strncat(temp," ",MAX_PATH);
	}

	strtrunc(temp,1);

}

/* run program or script in current directory */

if(runcommand(parsebuf[0],temp,backg) != -1) return(0);	/* run ok */

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
	   
			if(runcommand(parsebuf[1],temp,backg) != -1) return(0);	/* run ok */

   			b=temp;
  		}
 	}

}

/* Invalid command */

if((getlasterror() == FILE_NOT_FOUND) || (getlasterror() == END_OF_DIRECTORY)) {
	setvar("ERRORLEVEL",FILE_NOT_FOUND);

	kprintf("%s",badcommand);
}
else
{
	kprintf("error=%X\n",getlasterror());

	setvar("ERRORLEVEL",getlasterror());
	kprintf("%s\n",kstrerr(getlasterror()));
}

return(-1);
}

int set_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
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

	return(0);
}

setvar(parsebuf[1],parsebuf[3]);  /* set variable */
return(0);
}

int if_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t start;
size_t inverse;
size_t condition;
char *buffer[MAX_PATH];

size_t count;

if(tc == 1) {			/* insufficient parameters */
	kprintf("%s",noparams);
	return(-1);
}

start=1;

if(strncmp(parsebuf[1],"NOT",MAX_PATH)) {
	inverse=FALSE;
	start++;
}

if(strncmp(parsebuf[start],"EXIST",MAX_PATH) == 0) {
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
	 	if(strncmp(parsebuf[count],"==",MAX_PATH) == 0) break;
	 }

	 if(count >= tc) {	/* no == */
	 	kprintf("%s",syntaxerror); 
	 	return(-1);
	 }

	 if(strncmp(parsebuf[count-1],parsebuf[count+1],MAX_PATH) == 0) condition=TRUE;
}

start=count+2;		/* save start of command */

/* copy tokens to buffer */
if(condition == TRUE) {
	for(count=start;count<tc;count++) {				/* get args */
		strncat(buffer,parsebuf[count],MAX_PATH);
	    
	   	if(count <= tc-1) strncat(buffer," ",MAX_PATH);
	}

	doline(buffer);
	return(0);
}

}

/*
 * cd: set current directory
 */

int cd_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
char *directoryname[MAX_PATH];
size_t drivenumber;

if(tc == 1) {			/* insufficient parameters */
	getcwd(directoryname);

	kprintf("%s\n",directoryname);
	return(0);
}

getfullpath(parsebuf[1],directoryname);	/* get full path of directory */

if(chdir(parsebuf[1]) == -1) {		/* set directory */
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}

drivenumber=(char) *directoryname-'A';	/* get drive number */

strncpy(directories[drivenumber],directoryname);	/* save directory name */
return(0);
}


int copy_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t count;
size_t runopts;
size_t inverse;
char *b;
FILERECORD sourcedirentry;
FILERECORD destdirentry;
int findresult;
char c;
char *buffer[MAX_PATH];

if(tc == 1) {			/* insufficient parameters */
	kprintf("%s",noparams);
	return(-1);
}

if(strlen(parsebuf[1]) == 0 || strlen(parsebuf[2]) == 0) { /* missing args */
	 kprintf("%s",noparams);
	 return(-1);
}

findresult=findfirst(parsebuf[1],&sourcedirentry);

if(findresult == -1) {	/* no source file */
	kprintf("%s\n",kstrerr(getlasterror()));
	 return(-1);
}

count=0;

do {
	findresult=findfirst(parsebuf[2],&destdirentry);

	if((findresult != -1) && getlasterror() != (FILE_NOT_FOUND)) {
		while(1) {
			kprintf(overwrite,parsebuf[2]);
			read(stdin,&c,1);

			if((c == 'Y') || (c == 'y')) break;
			if((c == 'N') || (c == 'n')) return(0);
		}
	}
	 
	kprintf("%s\n",sourcedirentry.filename);
	count++;

	if(destdirentry.flags == FILE_DIRECTORY) {	/* copying to directory */
		ksnprintf(buffer,"%s\\%s",MAX_PATH,parsebuf[2],&sourcedirentry.filename);
	}

	if(copyfile(sourcedirentry.filename,parsebuf[2]) == -1) {
		kprintf("%s\n",kstrerr(getlasterror()));
		return(-1);
	}

} while(findnext(parsebuf[1],&sourcedirentry) != -1);

kprintf(filescopied,count);

return(0);
}


int for_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
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

if(strncmpi(parsebuf[2],"IN",MAX_PATH) != 0) {
	kprintf("%s",syntaxerror);
	return(-1);
}

for(start=3;start<tc;start++) {		/* find start */
	touppercase(parsebuf[start],parsebuf[start]);

	if(strncmpi(parsebuf[start],"do",MAX_PATH) == 0) {
		ksnprintf(buffer,"%s ",MAX_PATH,parsebuf[start+1]);

		for(count=start+2;count<tc;count++) {				/* get args */
			if(strncmpi(parsebuf[count],")",MAX_PATH) == 0) break;			/* at end */

			strncat(buffer,parsebuf[count],MAX_PATH);
			strncat(buffer," ",MAX_PATH);
	   	}

	  	break;
	 }   
}

if(start == tc) {
	kprintf("%s",syntaxerror);
	return(-1);
}
	
startvars=3;
endvars=start-1;

/* remove ( and ) */

if(strncmp(parsebuf[startvars],"(",MAX_PATH) == 0) {
	 startvars++;			/* skip ( */
}
else
{
	c=*parsebuf[startvars];

	if(c != '(') {
		kprintf(missingleftbracket);
		return(-1);
	}
	else
	{
		/* copy start variable without ( */

		b=parsebuf[startvars];
		b++;

		strncpy(tempbuffer,b);
		strncpy(parsebuf[startvars],tempbuffer);
	}
}

if(strncmp(parsebuf[start-1],")") == 0,MAX_PATH) {
	endvars--;			/* skip ( */
}
else
{
	b=parsebuf[start-1];
	b += strlen(parsebuf[start-1]);
	b--;

	c=*b;

	if(c != ')') {
		kprintf("%s",missingrightbracket);
		return(-1);
	}
	else
	{
	 	/* end variable without ) */

		*b=0;
	}
}

for(count=startvars;count<endvars+1;count++) {
	setvar(parsebuf[1],parsebuf[count]);

	if(doline(buffer) == -1) {
		kprintf("%s\n",kstrerr(getlasterror()));
		return(-1);
	}
}

return(0);
}
	

/*
 * del: delete file
 *
*/

int del_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t inverse;
int runopts;
char c;

if(tc == 1) {			/* insufficient parameters */
	kprintf("%s",noparams);
	return(-1);
}

if((strncmp(parsebuf[1],"*",MAX_PATH) == 0) || (strncmp(parsebuf[1],"*.*",MAX_PATH) == 0)) {
	kprintf("%s",allfilesdeleted);

	while(1) {  
		kprintf("%s",areyousure);
		read(stdin,&c,1);
	
		if((c == 'Y') || (c == 'y')) break;
		if((c == 'N') || (c == 'n')) return(0);
	 }
}

if(delete(parsebuf[1]) == -1) {
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}

return(0);
}


int mkdir_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
if(tc < 2) {			/* insufficient parameters */
	kprintf("%s",noparams);
	return(-1);
}

if(mkdir(parsebuf[1]) == -1) {		/* set directory */
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}

return(0);
}


int rmdir_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
if(tc < 2) {			/* insufficient parameters */
	kprintf("%s",noparams);
	return(-1);
}

if(rmdir(parsebuf[1]) == -1) {		/* set directory */
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}

return(0);
}



int rename_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
if(tc < 2) {			/* insufficient parameters */
	kprintf("%s",noparams);
	return(-1);
}

if(rename(parsebuf[1],parsebuf[2]) == -1) {		/* set directory */
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}

return(0);
}

/*
 * echo: display text
 */

int echo_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t count;
size_t isnewline=TRUE;
size_t starttoken=1;

if(tc == 1) {
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}

if(strncmp(parsebuf[1],"-n",MAX_PATH) == 0) {		/* no newline */
	isnewline=FALSE;
	
	starttoken++;
}

for(count=starttoken;count<tc;count++) {
	 kprintf("%s ",parsebuf[count]);
}

if(isnewline == TRUE) kprintf("\n");

return(0);
}

/*
 * exit: terminate command
 */

int exit_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
	if((commandlineoptions & COMMAND_PERMENANT) == 0) exit(atoi(parsebuf[1]));
}

/*
 * type: display file
 *
 * type [file]
 *
 */


int type_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
char *readbuf;
size_t handle;
int findresult;

if(tc == 1) {
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}

readbuf=alloc(MAX_READ_SIZE);		/* allocate buffer */

if(readbuf == NULL) {			/* can't allocate */
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}

handle=open(parsebuf[1],O_RDONLY);
if(handle == -1) {				/* can't open */
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}
	
/* read until end of file */
	
findresult=0;

while(findresult != -1) {	 
	 findresult=read(handle,readbuf,MAX_READ_SIZE);			/* read from file */
	 if(findresult == -1) {				/* can't read */
	 	if(getlasterror() != END_OF_FILE) {
	 		kprintf("%s\n",kstrerr(getlasterror()));
	 		break;
	 	}
	 }

	 kprintf(readbuf);
}
	

free(readbuf);

close(handle);
return(0);
}

/*
 * dir: list directory 
 *
 * dir [filespec]
 *
 */

int dir_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
char *b;
char *buffer[MAX_PATH];
size_t dircount;
size_t fcount;
char *temp[MAX_PATH];
FILERECORD direntry;

if(!*parsebuf[1]) strncpy(parsebuf[1],"*");	/* find all by default */

getfullpath(parsebuf[1],buffer);

/* find end of name */

b=buffer+strlen(buffer);

while(*b-- != '\\') ;;

b=b+2;
*b=0;

//kprintf("Directory of %s\n",buffer);

kprintf(directoryof,buffer);
	
memset(&direntry.filename,0,MAX_PATH);

if(findfirst(parsebuf[1],&direntry) == -1) {
	if(getlasterror() != END_OF_DIRECTORY) kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}


dircount=0;
fcount=0;

do {

/*time date size filename */

	  kprintf("%u:%u:%u %u/%u/%u",direntry.last_written_time_date.hours,direntry.last_written_time_date.minutes,\
				      direntry.last_written_time_date.seconds,direntry.last_written_time_date.day,\
				      direntry.last_written_time_date.month,direntry.last_written_time_date.year);

	  if(direntry.flags & FILE_DIRECTORY) {
	  	kprintf("     <DIR> ");

		dircount++;
	  }
	  else
	  {
	  	kprintf(" %d ",direntry.filesize);
	  }

	  kprintf("%s\n",direntry.filename);
	  fcount++;

	  memset(&direntry,sizeof(FILERECORD));

} while(findnext(parsebuf[1],&direntry) != -1);

ksnprintf(temp,"%d",MAX_PATH,getlasterror()); 		/* set errorlevel */
setvar("ERRORLEVEL",temp);

if(getlasterror() != END_OF_DIRECTORY) {		/* return error */
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}

kprintf(filesdirectories,fcount,dircount);

return(0);
}

int ps_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
size_t findresult;
PROCESS pbuf;
PROCESS *handle;

handle=findfirstprocess(&pbuf);				/* find first process */

do {
	kprintf("%u\n",pbuf.pid);

	handle=findnextprocess(handle,&pbuf);		/* get next process */
} while(handle != NULL);

return(0);
}

int kill_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
int findresult;

if(tc == 1) {		/* missing argument */
	kprintf("%s",noparams);
	return(-1);
}
	 
if(kill(atoi(parsebuf[1],10)) == -1) {
	kprintf("%s\n",kstrerr(getlasterror()));
	return(-1);
}

return(0);
}

int goto_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
char *buffer[MAX_PATH];
char *bufptr;
char *b;

if(tc == 1) {		/* missing label */
	kprintf("%s",nolabel);
	return(-1);
}

if(get_batch_mode() == FALSE) {			/* Not batch mode */
	kprintf("%s",notbatchmode);
	return(-1);
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
	
			if(strncmp(b,parsebuf[1],MAX_PATH) == 0) {	/* found label */	
				set_current_batchfile_pointer(b);
	        		return(0);
	       		}
	 	}
	}

	bufptr++;
}

/* label not found */

kprintf(nolabel);  
return(-1);
}

int rem_command(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
}

int critical_error_handler(char *name,size_t drive,size_t flags,size_t error) {
char *buf[10];

if((flags & 0x80000000) == 0) {			/* from disk */
	 flags &= 0x80000000;
	 kprintf("\n%s %s Drive %c\n",errors[error],errty[flags],drive+'A');
}
else
{
	 kprintf("\n%s %s %s\n",errors[error],errty[flags],name);
}

while(1) {
	  kprintf(abortretryfail);
	  read(stdin,buf,1);

	  if(((char) *buf == 'A') || ((char) *buf == 'a')) return(CRITICAL_ERROR_ABORT);
	  if(((char) *buf == 'R') || ((char) *buf == 'r')) return(CRITICAL_ERROR_RETRY);
	  if(((char) *buf == 'F') || ((char) *buf == 'f')) return(CRITICAL_ERROR_FAIL);
}

return(0);
}

