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

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#define stdout 1

void kprintf(char *format, ...);

/*
 * Print formatted string
 *
 * In: char *format	Formatted string to print, uses same format placeholders as printf
       ...		Variable number of arguments to print
 *
 * Returns nothing
 *
 */
void kprintf(char *format, ...) {
 va_list args;
 char *b;
 char c;
 char *s;
 int num;
 double d;
 char *z[10];

 va_start(args,format);			/* get start of variable args */

 b=format;

 while(*b != 0) {
  c=*b;

  if(c == '%') {
   c=*++b;
 
  switch(c) {

   case 's':				/* string */
    s=va_arg(args,const char*);
    write(stdout,s,strlen(s));
    b++;
    break;

  case 'd':				/* signed decimal */
    num=va_arg(args,int);

    if((num >> ((sizeof(int)*8)-1)) == 1)  write(stdout,"-",1);
  
    itoa(num,z);
    write(stdout,z,strlen(z));
    b++;
    break;

   case 'u':				/* unsigned decimal */
    num=va_arg(args,size_t);
    itoa(num,z);

    if(num < 10) write(stdout,"0",1);
    write(stdout,z,strlen(z));
    
    b++;
    break;

   case 'o':				/* octal */
    num=va_arg(args,size_t);
    itoa(num,z);

    if(num < 8) write(stdout,"0",1);
    write(stdout,z,strlen(z));

    b++;
    break;

   case 'p':				/* same as x */
   case 'x':				/*  lowercase x */
   case 'X':
    num=va_arg(args,size_t);

    tohex(num,z);
    write(stdout,z,strlen(z));

    b++;
    break;
   
   case 'c':				/* character */
    c=va_arg(args,int);
    write(stdout,&c,1);  
    b++;
    break;

  case '%':
   write(stdout,"%",1);

  case 'f':
   d=va_arg(args,double);
   itoa(num,z);
   write(stdout,z,strlen(z));

   }

 }
 else
 {
  write(stdout,&c,1);
 b++;
}

}

va_end(args);

}

/*
 * Print formatted string to string
 *
 * In:  char *buf	Buffer to store output
	char *format	Formatted string to print, uses same format placeholders as printf
       ...		Variable number of arguments to write to stringnt
 *
 * Returns nothing
 */

void ksprintf(char *buf,char *format, ...) {
 va_list args;
 char *b;
 char c;
 char *s;
 int num;
 double d;
 char *z[10];
 char *bufptr;

 bufptr=buf;

 va_start(args,format);			/* get start of variable args */

 b=format;

 while(*b != 0) {
  c=*b;

  if(c == '%') {
   c=*++b;
 
   switch(c) {

   case's':				/* string */
    s=va_arg(args,const char*);

    strcat(bufptr,s);
    bufptr=bufptr+strlen(s);
    b++;
    break;

  case 'd':				/* signed decimal */
    num=va_arg(args,int);

    if((num >> ((sizeof(int)*8)-1)) == 1)  strcat(bufptr,"-");
  
    itoa(num,z);
    strcat(bufptr,z);
    bufptr=bufptr+strlen(z)+1;

    b++;
    break;

   case 'u':				/* unsigned decimal */
    num=va_arg(args,size_t);

    if(num < 10) strcat(bufptr,"0");

    itoa(num,z);
    strcat(bufptr,z);
    bufptr=bufptr+strlen(z)+1;
    
    b++;
    break;

   case 'o':				/* octal */
    num=va_arg(args,size_t);

    if(num < 8) strcat(bufptr,"0");
    itoa(num,z);
    strcat(bufptr,z);
    bufptr=bufptr+strlen(z)+1;

    b++;
    break;

   case 'p':				/* same as x */
   case 'x':				/*  lowercase x */
   case 'X':
    num=va_arg(args,size_t);

    tohex(num,z);
    if(num < 16) strcat(bufptr,"0");
    strcat(bufptr,z);
    bufptr=bufptr+strlen(z)+1;

    b++;
    break;
   
   case 'c':				/* character */
    c=va_arg(args,size_t);
    b++;
    break;

  case '%':
   strcat(bufptr,"%");
   break;

  case 'f':
   break;

   }

 }
 else
 {

 *bufptr++=c;
 *bufptr=0;
 b++;
}

}

va_end(args);

}

