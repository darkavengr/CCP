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

 /*
 * get string length
 *
 */


#define MAX_PATH 255
#include <elf.h>
#include <stdint.h>
#include "../filemanager/vfs.h"

unsigned int strlen(char *str);
unsigned int strlen_unicode(char *str,size_t maxlen);
unsigned int strcpy(char *d,char *s);
unsigned int strcat(char *d,char *s);
int memcpy(void *d,void *s,size_t c);
int memcmp(char *source,char *dest,size_t count);
int strcmp(char *source,char *dest);
unsigned int memset(void *buf,char i,size_t size);
unsigned int itoa(unsigned int n, char s[]);
void reverse(char s[]);
unsigned int wildcard(char *mask,char *filename);
unsigned int touppercase(char *string);
unsigned int tolowercase(char *string);
unsigned int wildcard_rename(char *name,char *mask,char *out);
int strtrunc(char *str,int c);
int atoi(char *hex,int base);

unsigned int strlen(char *str) {
 size_t count=0;
 char *s;

 s=str;

 if(*str == 0) return(0);			/* null string */

 while(*s++ != 0) count++;		/* find size */

 return(count);
}

unsigned int strlen_unicode(char *str,size_t maxlen) {
 size_t count;
 char *s;

 count=0;
 s=str;

 if(*s == 0) return(0);				/* null string */

 while(count < maxlen) {
  if(*s == 0 && *s+1 == 0) return(count++);

  count++;	
  s=s+2;
 }

 return(count--);
}

unsigned int strcpy(char *d,char *s) {
 char *x;
 char *y;

 x=s;
 y=d;

 while(*x != 0) *y++=*x++;
 *y--=0;
}

unsigned int strcat(char *d,char *s) {
 char *x=s;
 char *y=d;

 while(*y != 0) y++;		/* find end */
 while(*x != 0) *y++=*x++;	/* append byte */

 *y--=0;
}

/*
 * copy memory
 *
*/

int memcpy(void *d,void *s,size_t c) {
 char *x;
 char *y;
 size_t count=0;
 
 x=s;
 y=d;

 while(count++ < c) *y++=*x++;

 return(d);   
}

/*
 * compare memory
 *
 */

int memcmp(char *source,char *dest,size_t count) {
 char c,d;

 while(count > 0) {
  if(*source != *dest) {
   c=*source;
   d=*dest;

   return(d-c);
  }

  source++;
  dest++;
  count--;

 }

 return(0);
}
 
/*
 * Compare string and return 0 if same
 *
 */


int strcmp(char *source,char *dest) {
 char c,d;

 while(*source == *dest) {
  if(*source == 0 && *dest == 0) return(0);		/* same */

  source++;
  dest++;
 }

 c=*source;
 d=*dest;

 return(d-c);
}


/* 
 * fill memory
 *
 */

unsigned int memset(void *buf,char i,size_t size) {
 size_t count=0;
 char *b;
 
 b=buf;					/* point to buffer */

 for(count=0;count<size;count++) { 
  *b++=i;				/* put byte */
 
 }
 return;
}



 /* itoa:  convert n to characters in s */
unsigned int itoa(unsigned int n, char s[]) {
unsigned int i, sign;
 
//     if ((sign = n) < 0) n = -n;          /* make n positive */

     i = 0;

     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */

     s[i] = '\0';

     reverse(s);
 }

void reverse(char s[]) {
unsigned int c;
unsigned int i;
unsigned int j;

for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
 c = s[i];
 s[i] = s[j];
 s[j] = c;
 }
}

unsigned int wildcard(char *mask,char *filename) {
 size_t count;
 size_t countx;
 char *x;
 char *y;
 char *buf[MAX_PATH];
 char *b;
 char *d;
 char *mmask[MAX_PATH];
 char *nname[MAX_PATH];

 strcpy(mmask,mask);
 strcpy(nname,filename);

 touppercase(mmask);					/* to upper case */
 touppercase(nname);

 memset(buf,0,MAX_PATH);				/* clear buffer */
 
 x=mmask;
 y=nname;

 for(count=0;count<strlen(mmask);count++) {
  if(*x == '*') {					/* match multiple characters */
   b=buf;
   x++;
   
   while(*x != 0) {					/* match multiple characters */
    if(*x == '*' || *x == '?') break;
    *b++=*x++;
   }

   b=filename;
   d=buf;

   for(countx=0;countx<strlen(nname);countx++) {
    if(memcmp(b,d,strlen(buf)) == 0) return(0);
    b++;
   }

   return(-1);
  }

  if(*x++ != *y++)  return(-1);
 }


return(0);
}

unsigned int touppercase(char *string) {
char *s;

s=string;

while(*s != 0) {			/* until end */

 if(*s >= 'a' && *s <= 'z') *s=*s-32;			/* to uppercase */
 s++;

 }
}

unsigned int tolowercase(char *string) {
char *s;

s=string;

while(*s != 0) {			/* until end */

 if(*s >= 'A' && *s <= 'Z') *s=*s+32;			/* to uppercase */
 s++;

 }
}
unsigned int wildcard_rename(char *name,char *mask,char *out) {
size_t count;
char *m;
char *n;
char *o;
char c;
size_t countx;
char *newmask[MAX_PATH];
char *mmask[MAX_PATH];
char *nname[MAX_PATH];

strcpy(mmask,mask);
strcpy(nname,name);

 touppercase(mmask);					/* convert to uppercase */
 touppercase(nname);

/*replace any * wild cards with ? */

n=newmask;
m=mask;

for(count=0;count<strlen(mmask);count++) {
 c=*m;						/* get character from mask */

 if(c == '*') {					/* match many characters */
  for(countx=count;countx<8-count;countx++) {
    *n++='?';    
   }

 }
 else
 {
  *n++=*m;
 }

m++;

}

n=nname;
m=newmask;
o=out;


for(count=0;count<strlen(newmask);count++) {
 c=*m;						/* get character from mask */

 if(c == '?') {					/* match single character */
  *o++=*n++;
 }
 else
 {
  *o++=*m;
 }

 m++;
}

return;
}			

int strtrunc(char *str,int c) {
char *s;
int count;
int pos;

s=str;
while(*s++ != 0) ;;

s -= 2;
*s=0;

return;
}

int atoi(char *hex,int base) {
int num=0;
char *b;
char c;
int count=0;
int shiftamount=0;

if(base == 10) shiftamount=1;	/* for decimal */

b=hex;
count=strlen(hex);		/* point to end */

b=b+(count-1);

while(count > 0) {
 c=*b;
 
 if(base == 16) {
  if(c >= 'A' && c <= 'F') num += (((int) c-'A')+10) << shiftamount;
  if(c >= 'a' && c <= 'f') num += (((int) c-'a')+10) << shiftamount;
  if(c >= '0' && c <= '9') num += ((int) c-'0') << shiftamount;

  shiftamount += 4;
  count--;
 }

 if(base == 8) {
  if(c >= '0' && c <= '7') num += ((int) c-'0') << shiftamount;

  shiftamount += 3;
  count--;
 }

 if(base == 2) {
  if(c >= '0' && c <= '1') num += ((int) c-'0') << shiftamount;

  shiftamount += 1;
  count--;
 }

 if(base == 10) {
  num += (((int) c-'0')*shiftamount);

  shiftamount =shiftamount*10;
  count--;
 }

 b--;
}

return(num);
}


