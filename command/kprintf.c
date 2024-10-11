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
#include "command.h"

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
char *formatptr;
char c;
char *s;
char *ptr;
size_t num;
double d;
char *tempbuffer[MAX_PATH];
size_t paramsize;

va_start(args,format);			/* get start of variable args */

formatptr=format;

while(*formatptr != 0) {
	paramsize=4;				/* default size is 4 bytes */

 	c=*formatptr;

	if(c == '%') {					/* format specifier */
		c=*++formatptr;

		if(c == 'l') {			/* long number */
			paramsize=8;
			c=*++formatptr;		
		}

		switch(c) {		
			case 's':				/* string */
				s=va_arg(args,const char*);	/* get variable argument */

				write(stdout,s,strlen(s));

				formatptr++;			/* point to next format character */
				break;

			case 'd':				/* decimal */
				num=va_arg(args,int);		/* get variable integer argument */

				if(num & ( 1 << ((sizeof(size_t) * 8)-1))) write(stdout,"-",1); /* write minus sign if negative */			 

				itoa(num,tempbuffer);		/* convert it to string */

				write(stdout,tempbuffer,strlen(tempbuffer));

				formatptr++;			/* point to next format character */
				break;

			case 'u':				/* unsigned decimal */
				num=va_arg(args,int);		/* get variable integer argument */
  
				itoa(num,tempbuffer);		/* convert it to string */

				write(stdout,tempbuffer,strlen(tempbuffer));

				formatptr++;			/* point to next format character */
				break;

			case 'o':		/* octal */
				num=va_arg(args,size_t);

			 	itoa(num,tempbuffer);

				write(stdout,tempbuffer,strlen(tempbuffer));
	
				formatptr++;
			 	break;

			case 'p':				/* same as x */
			case 'x':				/*  lowercase x */
			case 'X':
				num=va_arg(args,size_t);

	 			tohex(num,tempbuffer,paramsize);

				write(stdout,tempbuffer,strlen(tempbuffer));
	
				formatptr++;
				break;
   
			case 'c':				/* character */
	  			num=(unsigned char) va_arg(args, int);

				ptr=tempbuffer;
				*ptr++=(char)num;
				*ptr++=0;

				write(stdout,tempbuffer,strlen(tempbuffer));

	  			formatptr++;
	  			break;

	 		case '%':
				write(stdout,"%",1);

	  			formatptr++;
	  			break;
   			}

  }
 else								/* output character */
 {
			write(stdout,&c,1);

			formatptr++;
 }

}

va_end(args);
}

