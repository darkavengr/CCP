#include <stdint.h>
#include <stddef.h>
#include "errors.h"
#include "mutex.h"	
#include "device.h"
#include "vfs.h"
#include "bootinfo.h"
#include "debug.h"
#include "memorymanager.h"
#include "string.h"

/*
 * Get full file path from partial path
 *
 * In:  filename	Partial path to filename
	buf		Buffer to fill with full path to filename
 *
 *
 * Returns: -1 on error, 0 on success
 *
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

if((d == ':') && (e == '\\')) {               /* full path */
	strncpy(buf,filename,MAX_PATH);                     /* copy path */
}
else if((d == ':') && (e != '\\')) {		/* drive and filename only */
	memcpy(buf,filename,2); 		/* get drive */
	strncat(buf,filename+3,MAX_PATH);                 /* get filename */
}
else if(d != ':') {               /* filename only */
	if(c == '\\') {		/* path without drive */
		memcpy(buf,cwd,2);	/* copy drive letter and : */
		strncat(buf,filename,MAX_PATH);	/* copy filename */
	}
	else
	{
		b=cwd;
		b += strlen(cwd)-1;		/* point to end */

		if(*b == '\\') {		/* trailing \ */
			ksnprintf(buf,"%s%s",MAX_PATH,cwd,filename);	/* copy directory\filename */
		}
		else
		{
			ksnprintf(buf,"%s\\%s",MAX_PATH,cwd,filename);	/* copy directory\filename */
		}

	}
}

tc=tokenize_line(cwd,token,"\\");		/* tokenize line */
dottc=tokenize_line(filename,dottoken,"\\");		/* tokenize line */


for(countx=0;countx<dottc-1;countx++) {
	if(strncmp(dottoken[countx],"..",MAX_PATH) == 0 ||  strncmp(dottoken[countx],".",MAX_PATH) == 0) {

		for(count=0;count<dottc;count++) {

			if(strncmp(dottoken[count],"..",MAX_PATH) == 0 || strncmp(dottoken[countx],".",MAX_PATH) == 0) {
				tc--;
			}
			else
			{
				strncpy(token[tc],dottoken[count],MAX_PATH);
					tc++;
			}
		}


		strncpy(buf,token[0],MAX_PATH);
		strncat(buf,"\\",MAX_PATH);

		for(count=1;count<tc;count++) {
			strncat(buf,token[count],MAX_PATH);

			if(count != tc-1) strncat(buf,"\\",MAX_PATH);
		}

	}

	break;
} 

return(NO_ERROR);
}

