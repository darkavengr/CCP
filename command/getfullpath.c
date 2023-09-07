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
#include "../header/errors.h"
#include "command.h"
#include <stddef.h>

/*
* Get full file path from partial name
*
* In: char *filename	Partial filename
  char *buf	Buffer to store full name
*
* Returns nothing
*/

size_t getfullpath(char *filename,char *buf) {
char *token[10][MAX_PATH];
char *dottoken[10][MAX_PATH];
int tc=0;
int dottc=0;
int count;
int countx;
char *b;
char c,d,e;
char *cwd[MAX_PATH];

if(filename == NULL || buf == NULL) return(-1);

getcwd(cwd);

memset(buf,0,MAX_PATH);

b=filename;

c=*b++;
d=*b++;
e=*b++;

/* is already full path, just exit */

if(d == ':' && e == '\\') {               /* full path */
	strcpy(buf,filename);                     /* copy path */
}


/* drive and filename only */
if(d == ':' && e != '\\') { 
	 memcpy(buf,filename,2); 	  		 /* get drive */
	 strcat(buf,filename+3);                       /* get filename */
}

/* filename only */
if(d != ':' && e != '\\') { 
	b=cwd;
	b=b+(strlen(cwd))-1;

	c=*b;
	if(c == '\\') {
		ksprintf(buf,"%s%s",cwd,filename);
	}
	else
	{
		ksprintf(buf,"%s\\%s",cwd,filename);
	}
}

tc=tokenize_line(cwd,token,"\\");		/* tokenize line */
dottc=tokenize_line(filename,dottoken,"\\");		/* tokenize line */


for(countx=0;countx<dottc;countx++) {
	if(strcmp(dottoken[countx],"..") == 0 ||  strcmp(dottoken[countx],".") == 0) {

		for(count=0;count<dottc;count++) {
			if(strcmp(dottoken[count],"..") == 0 || strcmp(dottoken[countx],".") == 0) {
				tc--;
	 		}
	 		else
	 		{
	 			strcpy(token[tc],dottoken[count]);
	 			tc++;
	 		}
		}


		strcpy(buf,token[0]);
		strcat(buf,"\\");

		for(count=1;count<tc;count++) {
			strcat(buf,token[count]);
			if(count != tc-1) strcat(buf,"\\");
		}

	  }

	  break;
} 

}
