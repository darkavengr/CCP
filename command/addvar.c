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
#include "memorymanager.h"
#include "process.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"

	
/*
 * Set variable
 *
 * In: name	Variable name
	     val	Variable val
 *
 * Returns 0 on success or -1 on error
 * 
 */

unsigned long setvar(char *name,char *val) {
char *varptr=getenv();
size_t diff;
char *b;
char *buffer[MAX_PATH];

while((size_t) *varptr != NULL) {
	b=varptr;

	/* copy variable and val */

	while(*varptr != 0) {
		if(*varptr == '=') {		/* found end of name */
			varptr++;
	  		break;
	  	}

	  	*b++=*varptr++;
	}

	if(strncmpi(buffer,name,MAX_PATH) == 0) {	/* variable found */
		if(strlen(varptr) == strlen(val)) { /* same size */
			strncpy(varptr,val);
	  		return(0);
		}
		else if(strlen(buffer) < strlen(val)) { /* smaller size */
	   		diff=(strlen(val)-strlen(buffer))+1;	/* get difference */
		
	   		strncpy(varptr,val);
	   
			varptr += strlen(varptr);		/* point to end */
	   		*varptr++=0;				/* put null at end */
	
	   		memcpy(varptr,varptr+diff,ENVIROMENT_SIZE-diff);	/* copy back over gap */
	   		return(0);
	      }
		else if(strlen(buffer) > strlen(val)) { /* larger size */     	 
			/* copy forward */
	   		memcpy(varptr+strlen(val),varptr,ENVIROMENT_SIZE-strlen(val));

 	   		strncpy(varptr,val);
	   		varptr += strlen(varptr);		/* point to end */
	   		*varptr++=0;				/* put null at end */
	   
			return(0);
		}
	}

	varptr += strlen(varptr);			/* point to next */
 }

/* add new variable */
strncpy(varptr,name);	/* name */

varptr += strlen(varptr);	/* point to end */
*varptr++='=';
strncpy(varptr,val);	/* value */

return(0);
}

/*
 * Get variable
 *
 * In: name	Variable name
 *     buf	Buffer for value
 *
 * Returns 0 on success or -1 on error
 * 
 */

unsigned long getvar(char *name,char *buf) {
char *varptr=getenv();
char *varname[MAX_PATH];
char *nameptr;

while((size_t) *varptr != NULL) {

 /* copy variable and val */

nameptr=varname;

while(*varptr != 0) {
	 if(*varptr == '=') {		/* found end of name */    
	 	varptr++;
	 	break;
	 }

	 *nameptr++=*varptr++;
	}

	if(strncmpi(varname,name,MAX_PATH) == 0) {	/* variable found */
		strncpy(buf,varptr);
		return(0);
	}

	varptr += strlen(varptr);			/* point to next */
 }

 return(-1);
}

