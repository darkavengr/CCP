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

/*  CCP Command interpreter */

/*
   CD			*T
   COPY			*   
   DEL			*T
   DIR			*T
   ECHO			*T
   EXIT			*
   GOTO			*
   IF			*
   MKDIR		*T
   REM			*T
   RENAME		*T
   RMDIR		*
   SET   		*T
   TYPE			*T
   PS			*T
   KILL			*
*/
#include <stdint.h>
#include "../header/errors.h"
#include "command.h"
#include "../devicemanager/device.h"
#include "../filemanager/vfs.h"
#include "../processmanager/process.h"

FILERECORD direntry;
extern char *errs[255];
extern char *directories[26][MAX_PATH];
extern VARIABLES *vars;

unsigned long runcommand(char *fname,char *args,unsigned long backg);

unsigned long doline(char *buf);
unsigned long copyfile(char *source,char *destination);
unsigned long getvar(char *name,char *buf);
unsigned long runcommand(char *fname,char *args,unsigned long backg);
unsigned long setvar(char *name,char *val);
unsigned long readline(unsigned long handle,char *buf,int size);
void write_error(void);
uint32_t tohex(uint32_t hex,char *buf);
unsigned int writelfn(unsigned int drive,unsigned int block,unsigned int entryno,char *n,char *newname);
uint8_t createlfnchecksum(char *filename);
unsigned int commandconsolein;
unsigned int commandconsoleout;


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
int critical_error_handler(char *name,unsigned int drive,unsigned int flags,unsigned int error);
int rem_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]);

char *errty[] = { "reading","writing" };
unsigned int dirattribs;

int critical_error_handler(char *name,unsigned int drive,unsigned int flags,unsigned int error);
char *nolabel[] = { "Missing label\n" };
char *syntaxerror[] = { "Syntax error\n" };
char *noparams[] = { "No parameters\n" };
char *presskey[]={ "Press any key to continue..." };

struct {
 char *statement;
 unsigned int (*call_statement)(int,void *);		/* function pointer */
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
		   {  "IF",&if_statement },\
		   {  "PS",&ps_statement },\
		   {  "KILL",&kill_statement },\
		   {  "GOTO",&goto_statement },\
		   {  "IF",&if_statement },\	
		   {  "DEL",&del_statement },\	
		   {  "SET",&set_statement },\	
		   {  "REM",&rem_statement },\	
		   {  NULL,NULL } };

unsigned int errorlevel;
uint32_t tohex(uint32_t hex,char *buf);
unsigned long handle;

char *directories[26][MAX_PATH];

unsigned long main(void)
{
unsigned long count;
char *buffer[MAX_PATH];
char c;
char *b;

struct psp {
 uint8_t slack[128];
 uint8_t cmdlinesize;			/* command line size */
 uint8_t commandline[127];			/* command line */
} *psp=0;

set_critical_error_handler(critical_error_handler);

kprintf("Command version 1.0\n");

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

 while(1) {				/* forever */
  getcwd(buffer); 

  c=*buffer;
  kprintf("%c>",c);

  memset(buffer,0,MAX_PATH);

  readline(commandconsolein,buffer,MAX_PATH);			/* get line */
  if(*buffer) doline(buffer);

 }
}


/* do command */

unsigned long doline(char *buf) {
unsigned long count;
char *buffer[MAX_PATH];
unsigned long xdir;
unsigned long backg;
char *b;
char *d;
unsigned long findresult;
char c;
char *readbuf;
char x;
int savepos;
int tc;
char *parsebuf[20][MAX_PATH];
int statementcount;

if(strcmp(buf,"\n") == 0) return;	/* blank line */

memset(parsebuf,0,20*MAX_PATH);

tc=tokenize_line(buf,parsebuf," \t");

touppercase(parsebuf[0]);		/* convert to uppercase */
	
for(count=0;count<tc;count++) {	/* replace variables with value */
  b=parsebuf[count]+(strlen(parsebuf[count]))-1;

  if(*parsebuf[count] == '%' && *b == '%') {
   strtrunc(parsebuf[count],1);
   b=parsebuf[count]+1;
   getvar(b,buffer);
   strcpy(parsebuf[count],buffer); /* replace with value */
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
c=*b;					/* get drive letter */

if(c == ':' && strlen(parsebuf[0]) == 2) {
 c=*parsebuf[0];

 c=c-'A';					/* get drive number */
  
 chdir(directories[c]);
 writeerror();
 return;	
}


/* do statement */

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
 memset(buffer,0,MAX_PATH);

 for(count=1;count<tc;count++) {				/* get args */
  strcat(buffer,parsebuf[count]);
  if(count < tc) strcat(buffer," ");
 }

 strtrunc(buffer,1);

}

/* run program or script in current directory */
 if(runcommand(parsebuf[0],buffer,backg) != -1) return;	/* run ok */

/* prepend each directory in PATH to filename in turn and execute */

 if(getvar("PATH",d) != -1) {			/* get path */


	 b=buf;

	 while(*d != "\0") {
	  *b++=*d++;    
	
	  if(*b == ";") {			/* seperator */ 
	   b--;
	   *b="\\";
	   b++;
	   memcpy(buf,parsebuf[0],strlen(parsebuf[0]));		/* append filename */
	   if(runcommand(parsebuf[1],buffer,backg) != -1) return;	/* run ok */

	   b=buf;
	  }
	 }

}

/* bad command */

kprintf("Bad command or filename\n");

setvar("ERRORLEVEL","255");
return(-1);
}

int set_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
VARIABLES findvar;

if(tc == 1) {			/* display variables */

 while(findvars(&findvar) != -1) kprintf("%s=%s\n",findvar.name,findvar.val);
 return;
}

setvar(parsebuf[1],parsebuf[3]);
return;
}

int if_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
unsigned int start;
unsigned int inverse;
unsigned int condition;
char *buffer[MAX_PATH];

unsigned int count;

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

if(strcmp(parsebuf[start],"ERRORLEVEL") == 0) {
 getvar("ERRORLEVEL",buffer);

 if(inverse == FALSE && strcmp(buffer,parsebuf[start+1]) != 0) {
  condition=FALSE;
 }
 else
 {
  condition=TRUE;
 }
}

 if((strcmp(parsebuf[start],"EXIST") != 0) && (strcmp(parsebuf[start],"ERRORLEVEL") != 0)) {
  if(strcmp(parsebuf[start+1],"==") != 0) {		/* no == */
   kprintf(syntaxerror);
   errorlevel=254;
   return;
  }

  if(strcmp(parsebuf[start],parsebuf[start+2]) == 0) condition=TRUE;
 }

/* copy tokens to buffer */
if(condition == TRUE) {
    strcpy(buffer,parsebuf[start]);
    strcat(buffer," ");

    for(count=start+1;count<tc;count++) {				/* get args */
     strcat(buffer,parsebuf[count]);
     strcat(buffer," ");
   }

   doline(buffer);
   return;
 }
}

/*
 * cd: set current directory
 */

int cd_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
char *buffer[MAX_PATH];

if(tc == 1) {			/* not enough args */
 getcwd(buffer);

 kprintf("%s\n",buffer);
 return;
}

if(chdir(parsebuf[1]) == -1) {		/* set directory */
 writeerror();
 return;
}

 return;
}


int copy_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
unsigned int count;
unsigned int runopts;
unsigned int inverse;
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
   kprintf("Overwrite %s (y/n)?",parsebuf[2]);
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

kprintf("%d File(s) copied\n",count);
 return;
}


int for_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
int start;
int count;
char *buffer[MAX_PATH];

//  0   1 2   3    4   5  6
//for a in (*.*) do echo %a%

 touppercase(parsebuf[2]);

 if(strcmp(parsebuf[2],"IN") != 0) {
  kprintf(syntaxerror);
  return;
 }

 for(start=3;start<tc;start++) {		/* find start */
   touppercase(parsebuf[start]);

  if(strcmp(parsebuf[start],"do") == 0) break;
 }

 if(start == tc) {
  kprintf(syntaxerror);
  return;
 }
  
/* get command */
 strcpy(buffer,parsebuf[start]);
  
 for(count=start+1;count<tc;count++) {				/* get args */
  strcat(buffer,parsebuf[count]);
  strcat(buffer," ");
 }

 for(count=3;count<start-1;count++) {
  setvar(parsebuf[1],parsebuf[count]);
 
  doline(buffer);
 }

 return;
}
 

/*
 * del: delete file
 */

int del_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
unsigned int inverse;
int runopts;
char c;

 if(tc == 1) {			/* not enough args */
  kprintf(noparams);
  return;
 }

 if(strcmp(parsebuf[1],"*") == 0 || strcmp(parsebuf[1],"*.*") == 0) {
  kprintf("All files will be deleted!\n");

  while(1) {  
   kprintf("Are you sure (y/n)?");
   read(stdin,&c,1);
 
    if(c == 'Y' || c == 'y') break;
    if(c == 'N' || c == 'n') return;
  }
 }

  if(runopts & DELETE_PROMPT) {	/* delete */
   while(1) {  
    kprintf("Delete %s (y/n)?",parsebuf[1]);
    read(stdin,&c,1);
   
    if(c == 'Y' || c == 'y') break;
    if(c == 'N' || c == 'n') return;
   }
 
    if(delete(parsebuf[1]) == -1) writeerror();
  }

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
unsigned int count;

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
 exit(0);
}

/*
 * type: display file
 *
 * type [file]
 *
 */


int type_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
char *readbuf;
unsigned int handle;
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
 
 /* read until eof */
 
 findresult=0;

 while(findresult != -1) {
  findresult=read(handle,readbuf,MAX_READ_SIZE);			/* read from file */
  if(findresult == -1) {				/* can't open */
   writeerror();
   return;
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
unsigned int findresult;
char *b;
char *buffer[MAX_PATH];
unsigned int dircount;
unsigned int fcount;
char *z[MAX_PATH];

if(!*parsebuf[1]) strcpy(parsebuf[1],"*");	/* find all by default */

getfullpath(parsebuf[1],buffer);

b=buffer+strlen(buffer);

while(*b-- != '\\') ;;
b=b+2;
*b=0;

 kprintf("Directory of %s\n\n",buffer);
 
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

    kprintf("%u:%u:%u %u/%u/%u",direntry.timebuf.hours,direntry.timebuf.minutes,direntry.timebuf.seconds,\
			        direntry.timebuf.day,direntry.timebuf.month,direntry.timebuf.year);

   if(direntry.flags & FILE_DIRECTORY) {
    kprintf("     <DIR> ");
   }
   else
   {
    itoa(direntry.filesize,buffer);		/* pad out file size */		
    memset(z,0,10);
    memset(z,' ',10-strlen(buffer));
    kprintf(z);

    kprintf("%u ",direntry.filesize);
   }

   kprintf("%s\n",direntry.filename);
   fcount++;

   memset(&direntry.filename,0,MAX_PATH);

   findresult=findnext(parsebuf[1],&direntry);
   if(findresult == -1) break;
  }

  ksprintf(z,"%d",getlasterror()); 
  setvar("ERRORLEVEL",z);

  if(getlasterror() != END_OF_DIR) {
   writeerror();
   return;
 }

  kprintf("%d files(s) %d directories\n",fcount,dircount);

  return;
}

int ps_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
unsigned int findresult;
PROCESS pbuf;

findresult=findfirstprocess(&pbuf);		/* get first process */
if(findresult == -1) return;		/* no processes */

kprintf("PID   Parent     Filename        Arguments\n");

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
unsigned int savepos;
unsigned int findresult;
char *b;
char *buffer[MAX_PATH];
char c;

if(tc == 1) {		/* missing label */
 kprintf(nolabel);
 return;
}

savepos=tell(handle);		/* save position */
seek(handle,0,SEEK_SET);	/* go to start */

 while(findresult != -1) {
  findresult=readline(commandconsolein,buffer,MAX_PATH);			/* get line */

  b=buffer;
  b=b+strlen(buffer);			/* point to end */
  c=*b;				/* get end */

  if(c == ':') return;		/* found line, will continue from here */
 }

  /* reset to last line and display error message */

 seek(handle,0,SEEK_SET);	/* go to start */
 kprintf(nolabel);  
 return(-1);
}

int rem_statement(int tc,char *parsebuf[MAX_PATH][MAX_PATH]) {
}

int critical_error_handler(char *name,unsigned int drive,unsigned int flags,unsigned int error) {
 CHARACTERDEVICE *cd;
 BLOCKDEVICE *bd;
 char c;
 char *buf[10];
 char *b;

 if((flags & 0x80000000) == 0) {			/* from disk */
  flags &= 0x80000000;
  kprintf("%s %s Drive %c\n",errs[error],errty[flags],drive+'A');
 }
 else
 {
  kprintf("%s %s %s\n",errs[error],errty[flags],name);
 }

  while(1) {
   kprintf("\nAbort, Retry, Fail?");
   read(stdin,buf,1);

   b=buf;
   c=*buf;

   if(c == 'A' || c == 'a') return(CRITICAL_ERROR_ABORT);
   if(c == 'R' || c == 'r') return(CRITICAL_ERROR_RETRY);
   if(c == 'F' || c == 'f') return(CRITICAL_ERROR_FAIL);
  }

}

