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
#include "../header/errors.h"
#include "../devicemanager/device.h"
#include "../filemanager/vfs.h"

VARIABLES *vars=NULL;
VARIABLES *lastvar=NULL;
		
/* set shell variable */
unsigned long setvar(char *name,char *val) {
 VARIABLES *next;
 VARIABLES *last;


 if(vars == NULL) {                  /* first */
  vars=alloc(sizeof(VARIABLES));
  if(vars == NULL) return(-1);

  next=vars;
 }
 else
 {
  next=vars;
  last=vars; 

   while(next != NULL) {				/* until end */
    last=next; 

     if(strcmp(next->name,name) == 0) {		/* found name */
      strcpy(next->val,val);				/* copy value */
      return(NO_ERROR);
     }
 
     next=next->next;   
    }

   last->next=alloc(sizeof(VARIABLES));		/* add new link */
   if(last->next == NULL) return(-1);				/* can't allocate */   
 
   next=last->next;
  }

  strcpy(next->name,name);					/* create new variable */
  strcpy(next->val,val);
  next->next=NULL;

  return(NO_ERROR);
}

/* get shell variable
 */
unsigned long getvar(char *name,char *buf) {
 VARIABLES *next=vars;

 while(next != NULL) {				/* until end */


   if(strcmp(next->name,name) == 0) {		/* found name */
    strcpy(buf,next->val);
    return(0);
  }



  next=next->next;   
 }

  return(-1);
}

unsigned long findvars(VARIABLES *buf) {
 if(lastvar == NULL) {
  lastvar=vars;
 }
 else
 {
  lastvar=lastvar->next;
 }

 if(lastvar == NULL) return(-1);

 memcpy(buf,lastvar,sizeof(VARIABLES));
 return(0);
}

