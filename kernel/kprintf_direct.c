#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "string.h"

#define LEFT_JUSTIFY_FLAG		1
#define PLUS_MINUS_FLAG			2
#define SPACE_BEFORE_DIGIT_FLAG		4
#define ZERO_BEFORE_DIGIT_FLAG		8
#define USE_HEX_OCTAL_SIGN		16
#define USE_HALFHALF_NUMBER_FLAG	32
#define USE_HALF_NUMBER_FLAG		64
#define USE_LONG_NUMBER_FLAG		128
#define USE_LONGLONG_NUMBER_FLAG	256
#define USE_INTMAXT_NUMBER_FLAG		512
#define USE_SIZET_NUMBER_FLAG		1024
#define USE_PTRDIFFT_NUMBER_FLAG	2048

/*
 * Print formatted string to a fixed console
 *
 * In: format	Formatted string to print, uses same format placeholders as printf
	...	Variable number of arguments to print
 *
 * Returns nothing
 * 
 * This function is used early in intializtion and by exception()
 */
size_t kprintf_direct(char *format,...) {
va_list args;
char *formatptr;
char formatchar;
char *s;
char *ptr;
size_t num;
double d;
char *tempbuffer[MAX_SIZE];
size_t flags=0;
size_t count;
size_t width;
size_t paramsize;
size_t outcount=0;
void *nptr;

va_start(args,format);			/* get start of variable args */

formatptr=format;

while(*formatptr != 0) {
	width=0;

 	formatchar=*formatptr;

	if(formatchar == '%') {					/* format specifier */
		formatchar=*++formatptr;

		/* flags */

		if(formatchar == '+') {			/* print + before positive numbers */
			flags |= PLUS_MINUS_FLAG;	

			formatchar=*++formatptr;
		}

		if(formatchar == '#') {			/* precede octal numbers with 0 and hexadecimal numbers with 0x */
			flags |= USE_HEX_OCTAL_SIGN;
			formatchar=*++formatptr;
		}

		if(formatchar == '*') {			/* width in parameter */
			width=va_arg(args,size_t);
			formatchar=*++formatptr;
		}

		if(formatchar == ' ') {			/* print space before numbers */
			flags |= SPACE_BEFORE_DIGIT_FLAG;
			formatchar=*++formatptr;
		}
		else if(formatchar == '0') {			/* pad out number with zeroes */
			flags |= ZERO_BEFORE_DIGIT_FLAG;
			formatchar=*++formatptr;	
		}
		
		/* width */

		s=tempbuffer;

		if(((char) *formatptr >= '0') && ((char) *formatptr <= '9')) {
			while(((char) *formatptr >= '0') && ((char) *formatptr <= '9')) {
				*s++=*formatptr++;
			}

			*s=0;		/* put null at end */
	
			width=atoi(tempbuffer,10);

			formatchar=*formatptr;
		}

		/* length */
		if(formatchar == 'h') {			/* half or halfhalf number */
			ptr=++formatptr;

			if(*ptr == 'h') {
				flags=USE_HALFHALF_NUMBER_FLAG;
			}
			else
			{
				flags=USE_HALF_NUMBER_FLAG;
			}

			formatchar=*++formatptr;
		}
		else if(formatchar == 'l') {			/* long or longlong number */
			if((char) *++formatptr == 'l') {
				flags=USE_LONGLONG_NUMBER_FLAG;
			}
			else
			{
				flags=USE_LONG_NUMBER_FLAG;
			}

			formatchar=*++formatptr;
		}
		else if(formatchar == 'j') {			/* intmax_t */
			flags=USE_INTMAXT_NUMBER_FLAG;	
			formatchar=*++formatptr;
		}
		else if(formatchar == 'z') {			/* size_t */
			flags=USE_SIZET_NUMBER_FLAG;
			formatchar=*++formatptr;
		}
		else if(formatchar == 't') {			/* ptrdiff_t */
			flags=USE_PTRDIFFT_NUMBER_FLAG;
			formatchar=*++formatptr;
		}

		/* if no padding digit flags use default space character */

		if( ((flags & SPACE_BEFORE_DIGIT_FLAG) == 0) && ((flags & ZERO_BEFORE_DIGIT_FLAG) == 0)) flags |= SPACE_BEFORE_DIGIT_FLAG;
	
		/* specifiers */

		if(formatchar == 's') {				/* string */
				s=va_arg(args,const char*);	/* get variable argument */

				if(strlen(s) < width) {		/* pad out string */
					for(count=0;count<width-strlen(s);count++) {
						outputconsole(" ",1);
					}
				}

				outputconsole(s,strlen(s));

				outcount += strlen(s);

				formatptr++;			/* point to next format character */
		}
		else if((formatchar == 'i') || (formatchar == 'd')) {	/* decimal */
				if(flags & USE_HALFHALF_NUMBER_FLAG) {
					num=va_arg(args,signed char);		/* get variable integer argument */
				}
				else if(flags & USE_HALF_NUMBER_FLAG) {
					num=va_arg(args,short int);
				}
				else if(flags & USE_LONG_NUMBER_FLAG) {
					num=va_arg(args,long int);
				}
				else if(flags & USE_LONGLONG_NUMBER_FLAG) {
					num=va_arg(args,long long int);
				}
				else if(flags & USE_INTMAXT_NUMBER_FLAG) {
					num=va_arg(args,intmax_t);
				}
				else if(flags & USE_SIZET_NUMBER_FLAG) {
					num=va_arg(args,size_t);
				}
				else if(flags & USE_PTRDIFFT_NUMBER_FLAG) {
					num=va_arg(args,ptrdiff_t);
				}
				else
				{
					num=va_arg(args,size_t);
				}

				if(num & ( 1 << ((sizeof(size_t) * 8)-1))) {
					outputconsole("-",1);	/* write minus sign if negative */

					outcount++;			 
				}
				else
				{
					if(flags & PLUS_MINUS_FLAG) {
						outputconsole("+",1);
						outcount++;
					}
				}

				itoa(num,tempbuffer);		/* convert it to string */

				/* pad out number */
		
		/*		if(width > 0) {
					for(count=width-strlen(tempbuffer);count > 0;count--) {
						if(flags & ZERO_BEFORE_DIGIT_FLAG) {
							outputconsole("0",1);
						}
						else if(flags & SPACE_BEFORE_DIGIT_FLAG) {
							outputconsole(" ",1);
						}

						outcount++;
					}
				}*/

				outputconsole(tempbuffer,strlen(tempbuffer));
				outcount += strlen(tempbuffer);

				formatptr++;			/* point to next format character */
			}
			else if((formatchar == 'u') || (formatchar == 'o') || (formatchar == 'x') || (formatchar == 'X')) {	/* unsigned decimal,hexadecimal or octal */
				if(flags & USE_HALFHALF_NUMBER_FLAG) {
					num=va_arg(args,unsigned char);		/* get variable integer argument */
					paramsize=sizeof(unsigned char);
				}
				else if(flags & USE_HALF_NUMBER_FLAG) {
					num=va_arg(args,unsigned short int);
					paramsize=sizeof(unsigned short int);
				}
				else if(flags & USE_LONG_NUMBER_FLAG) {
					num=va_arg(args,unsigned long int);
					paramsize=sizeof(unsigned long int);
				}
				else if(flags & USE_LONGLONG_NUMBER_FLAG) {
					num=va_arg(args,unsigned long long int);
					paramsize=sizeof(unsigned long long int);
				} 
				else if(flags & USE_INTMAXT_NUMBER_FLAG) {
					num=va_arg(args,intmax_t);
					paramsize=sizeof(intmax_t);
				}
				else if(flags & USE_SIZET_NUMBER_FLAG) {
					num=va_arg(args,size_t);
					paramsize=sizeof(size_t);
				}
				else if(flags & USE_PTRDIFFT_NUMBER_FLAG) {
					num=va_arg(args,ptrdiff_t);
				}
  				else
				{
					num=va_arg(args,size_t);
					paramsize=sizeof(size_t);
				}

				if(formatchar == 'u') {
					itoa(num,tempbuffer);		/* convert it to string */
				}
				else if(formatchar == 'o') {
					if(flags & USE_HEX_OCTAL_SIGN) outputconsole("0",1);		/* print 0 before octal numbers */

					tooctal(num,tempbuffer);

				}
				else if((formatchar == 'x') || (formatchar == 'X')) {
					if(flags & USE_HEX_OCTAL_SIGN) outputconsole("0x",2);		/* print 0x before hexadecimal numbers */

					tohex(num,tempbuffer);

					if(formatchar == 'x') tolowercase(tempbuffer,tempbuffer);	/* print lowercase hexadecimal number */
				}

				if(width > 0) {		/* pad out number */
					for(count=width-strlen(tempbuffer);count > 0;count--) {
						if(flags & ZERO_BEFORE_DIGIT_FLAG) {
							outputconsole("0",1);
						}
						else if(flags & SPACE_BEFORE_DIGIT_FLAG) {
							outputconsole(" ",1);
						}

						outcount++;
					}
				}

				outputconsole(tempbuffer,strlen(tempbuffer));
				outcount += strlen(tempbuffer);

				formatptr++;			/* point to next format character */

			}
			if(formatchar == 'c') {			/* character */
	  			num=(unsigned char) va_arg(args,int);

				ptr=tempbuffer;
				*ptr++=(char)num;
				*ptr++=0;

				outputconsole(tempbuffer,strlen(tempbuffer));

	  			formatptr++;
			}
			if(formatchar == '%') {			/* % character */
				outputconsole("%",1);
				
				outcount++;
	  			formatptr++;
   			}
			if(formatchar == 'p') {			/* pointer */
				num=(unsigned char) va_arg(args,size_t);

				tohex(num,tempbuffer);

				for(outcount=sizeof(size_t)*4-strlen(tempbuffer);count > 0;count--) {
					outputconsole("0",1);
				}

				outputconsole(tempbuffer,strlen(tempbuffer));

				outcount += sizeof(size_t)*4;
			}
			if(formatchar == 'n') {			/* store count in parameter */
				num=(unsigned char) va_arg(args,signed int);

				if(flags & USE_HALFHALF_NUMBER_FLAG) {
					num=va_arg(args,unsigned char);		/* get variable integer argument */

					signed char *nptr_hh;
					*nptr_hh=num;
				}
				else if(flags & USE_HALF_NUMBER_FLAG) {
					num=va_arg(args,unsigned short int);
					short int *nptr_h;

					*nptr_h=num;
				}
				else if(flags & USE_LONG_NUMBER_FLAG) {
					num=va_arg(args,unsigned long int);
					long int *nptr_l;

					*nptr_l=num;
				}
				else if(flags & USE_LONGLONG_NUMBER_FLAG) {
					num=va_arg(args,unsigned long long int);
					long long int *nptr_ll;

					*nptr_ll=num;
				} 
				else if(flags & USE_INTMAXT_NUMBER_FLAG) {
					num=va_arg(args,intmax_t);
					intmax_t *nptr_imt;

					*nptr_imt=num;
				}
				else if(flags & USE_SIZET_NUMBER_FLAG) {
					num=va_arg(args,size_t);
					size_t *nptr_sizet;

					*nptr_sizet=num;
				}
				else if(flags & USE_PTRDIFFT_NUMBER_FLAG) {
					num=va_arg(args,ptrdiff_t);
					ptrdiff_t *nptr_ptrdifft;

					*nptr_ptrdifft=num;
				}
  				else
				{
					num=va_arg(args,size_t);
					size_t *nptr_sizet;

					*nptr_sizet=num;
				}
				ptr=num;
				*ptr=outcount;		/* save output count */

				outcount += sizeof(signed int);
			}				

	}
	else								/* output character */
	{
			outputconsole(&formatchar,1);

			formatptr++;
			outcount++;
	}
}

va_end(args);
return(outcount);
}
