#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "string.h"

/*
 * Print formatted string to a fixed console
 *
 * In: char *format	Formatted string to print, uses same format placeholders as printf
	   ...		Variable number of arguments to print
 *
 * Returns nothing
 * 
 * This function is used early in intializtion and by exception()
 */
void kprintf_direct(char *format, ...) {
va_list args;
char *formatptr;
char c;
char *s;
char *ptr;
size_t num;
double d;
char *tempbuffer[MAX_SIZE];
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

				outputconsole(s,strlen(s));

				formatptr++;			/* point to next format character */
				break;

			case 'd':				/* decimal */
				num=va_arg(args,int);		/* get variable integer argument */

				if(num & ( 1 << ((sizeof(size_t) * 8)-1))) outputconsole("-",1);	/* write minus sign if negative */			 

				itoa(num,tempbuffer);		/* convert it to string */

				outputconsole(tempbuffer,strlen(tempbuffer));

				formatptr++;			/* point to next format character */
				break;

			case 'u':				/* unsigned decimal */
				num=va_arg(args,int);		/* get variable integer argument */
  
				itoa(num,tempbuffer);		/* convert it to string */

				outputconsole(tempbuffer,strlen(tempbuffer));

				formatptr++;			/* point to next format character */
				break;

			case 'o':		/* octal */
				num=va_arg(args,size_t);

			 	itoa(num,tempbuffer);

				outputconsole(tempbuffer,strlen(tempbuffer));
	
				formatptr++;
			 	break;

			case 'p':				/* same as x */
			case 'x':				/*  lowercase x */
			case 'X':
				num=va_arg(args,size_t);

	 			tohex(num,tempbuffer,paramsize);

				outputconsole(tempbuffer,strlen(tempbuffer));
	
				formatptr++;
				break;
   
			case 'c':				/* character */
	  			num=(unsigned char) va_arg(args, int);

				ptr=tempbuffer;
				*ptr++=(char)num;
				*ptr++=0;

				outputconsole(tempbuffer,strlen(tempbuffer));

	  			formatptr++;
	  			break;

	 		case '%':
				outputconsole("%",1);

	  			formatptr++;
	  			break;
   			}

	}
	else								/* output character */
	{
			outputconsole(&c,1);

			formatptr++;
	}
}

va_end(args);
}

