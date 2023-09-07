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

#include <elf.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "errors.h"
#include "../filemanager/vfs.h"
#include "string.h"

size_t strlen(char *str);
size_t strlen_unicode(char *str,size_t maxlen);
void strcpy(char *d,char *s);
void strcat(char *d,char *s);
size_t memcpy(void *d,void *s,size_t c);
size_t memcmp(char *source,char *dest,size_t count);
size_t strcmp(char *source,char *dest);
void memset(void *buf,char i,size_t size);
size_t itoa(size_t n, char s[]);
void reverse(char s[]);
size_t wildcard(char *mask,char *filename);
size_t touppercase(char *string);
size_t wildcard_rename(char *name,char *mask,char *out);
int strtrunc(char *str,int c);
int atoi(char *hex,int base);
void kprintf(char *format, ...);
void ksprintf(char *buf,char *format, ...);
void tohex(uint32_t hex,char *buf);
size_t tokenize_line(char *linebuf,char *tokens[MAX_PATH][MAX_PATH],char *split);
void tohex64(uint64_t hex,char *buf);
void kprintf_direct(char *format, ...);

/*
 * Get string length
 *
 * In: char *str	String buffer
 *
 * Returns string length
 *
 */

size_t strlen(char *str) {
 size_t count=0;
 char *s;

 s=str;

 if(*str == 0) return(0);			/* null string */

 while(*s++ != 0) count++;		/* find size */

 return(count);
}

/*
 * Get length of Unicode string
 *
 * In: char *string	String to get length of
	   size_t maxlen	Maximum length of string
 *
 * Returns string length
 *
 */

size_t strlen_unicode(char *str,size_t maxlen) {
 size_t count;
 char *s;
 char c;
 char d;

 count=0;
 s=str;

 if(*s == 0) return(0);				/* null string */

 while(count < maxlen) {
  c=*s++;
  d=*s++;
  if((c == 0) && (d == 0)) return(count);

  count++;	
 }

 return(count);
}

/*
 * Copy string
 *
 * In: char *d		Destination string
	   char *s		Source string
 *
 * Returns nothing
 *
 */

void strcpy(char *d,char *s) {
 char *x;
 char *y;

 x=s;
 y=d;

 while(*x != 0) *y++=*x++;
 *y--=0;
}

/*
 * Conatecate string
 *
 * In: char *d		String to conatecate to
	   char *s		String to conatecate
 *
 * Returns nothing
 *
 */

void strcat(char *d,char *s) {
char *x=s;
char *y=d;

while(*y != 0) y++;		/* find end */
while(*x != 0) *y++=*x++;	/* append byte */

*y--=0;
}

/*
 * Copy memory
 *
 * In: void *d		Address to copy to
	   void *s		Address to copy from
	   size_t c		Number of bytes to copy
 *
 * Returns number of bytes copied
 *
 */

size_t memcpy(void *d,void *s,size_t c) {
char *x;
char *y;
size_t count=0;
 
x=s;
y=d;

while(count++ < c) *y++=*x++;

return(d);   
}

/*
 * Compare memory
 *
 * In: void *d		Second address to compare
	   void *s		First address to compare
	   size_t c		Number of bytes to compare
 *
 * Returns different of last source and destination bytes if they do not match, 0 otherwise
 *
 */

size_t memcmp(char *source,char *dest,size_t count) {
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
 * Compare string
 *
 * In: char *source	First string to compare
	   void *dest	Second string to compare
 *
 * Returns different of last source and destination bytes if they do not match, 0 otherwise
 *
 */
size_t strcmp(char *source,char *dest) {
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
 * Compare string case insensitively
 *
 * In: char *source	First string to compare
	   void *dest	Second string to compare
 *
 * Returns different of last source and destination bytes if they do not match, 0 otherwise
 *
 */
size_t strcmpi(char *source,char *dest) {
char a,b;
char *sourcetemp[MAX_PATH];
char *desttemp[MAX_PATH];

strcpy(sourcetemp,source);
strcpy(desttemp,dest);

touppercase(sourcetemp);
touppercase(desttemp);

return(strcmp(sourcetemp,desttemp));
}

/*
 * Fill memory
 *
 * In: void *buf	Address to fill
	   char i		Byte to fill with
	   size_t size	Number of bytes to fill
 *
 * Returns nothing
 *
 */

void memset(void *buf,char i,size_t size) {
size_t count=0;
char *b;
 
b=buf;					/* point to buffer */

for(count=0;count<size;count++) { 
	*b++=i;				/* put byte */
}

return;
}


/*
 * Convert integer to character representation
 *
 * In: size_t n	Number to convert
	   char s[]		Character buffer
 *
 * Returns nothing
 *
 */

size_t itoa(size_t n, char s[]) {
size_t i, sign;
 
//	 if ((sign = n) < 0) n = -n;		  /* make n positive */

i = 0;

do {	   /* generate digits in reverse order */
	s[i++] = n % 10 + '0';   /* get next digit */
} while ((n /= 10) > 0);	 /* delete it */

s[i] = '\0';

reverse(s);
 }

/*
 * Reverse string
 *
 * In: char s[]		String to reverse
 *
 * Returns nothing
 *
 */
void reverse(char s[]) {
size_t c;
size_t i;
size_t j;

for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
	c = s[i];
	s[i] = s[j];
	s[j] = c;
}

}

/*
 * Match string using wildcard
 *
 * In: char *mask
	   void *dest	Second string to compare
 *
 * Returns different of last source and destination bytes if they do not match, 0 otherwise
 *
 */
size_t wildcard(char *mask,char *filename) {
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

touppercase(mmask);
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

/*
 * Convert string to uppercase
 *
 * In: char *string	String to convert
 *
 * Returns nothing
 *
 */
size_t touppercase(char *string) {
char *s;

s=string;

while(*s != 0) {			/* until end */

	if(*s >= 'a' && *s <= 'z') *s=*s-32;			/* to uppercase */
	s++;

}

}

/*
 * Replace string using wildcard
 *
 * In: char *name	Input string
	   char *mask	Wildcard to match
	   char *out	Output string
 *
 * Returns nothing
 *
 */
size_t wildcard_rename(char *name,char *mask,char *out) {
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

/*
 * Truncate string
 *
 * In: char *str	String to truncate
	   int c		Number of characters to truncate
 *
 * Returns nothing
 *
 */
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

/*
 * Convert string to integer
 *
 * In: char *hex	String to convert
	   int base		Base number to use (2,8,10,16)
 *
 * Returns converted number
 *
 */
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
char *p;
int num;
double d;
char *z[MAX_SIZE];

va_start(args,format);			/* get start of variable args */

b=format;

while(*b != 0) {
 	c=*b;

	if(c == '%') {
		c=*++b;

		switch(c) {

			case's':				/* string */
				s=va_arg(args,const char*);

				write(stdout,s,strlen(s));
				b++;
				break;

			case 'd':				/* signed decimal */
				 num=va_arg(args,int);
  
				 itoa(num,z);
				 write(stdout,z,strlen(z));
				 b++;
				 break;

			case 'u':				/* unsigned decimal */
				 num=va_arg(args,size_t);

				 itoa(num,z);
				 write(stdout,z,strlen(z));
	
	 			b++;
				break;

			case 'o':		/* octal */
				num=va_arg(args,size_t);

			 	itoa(num,z);
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
	  			c=(unsigned char) va_arg(args, int);

	  			p=z;
	  			*p=c;

	  			write(stdout,p,1);
	  			b++;
	  			break;

	 		case '%':
	  			write(stdout,"%",1);
  
	  			b++;

	  			break;
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
char *b;
char c;
char *s;
char *p;
int num;
double d;
char *z[MAX_SIZE];

va_start(args,format);			/* get start of variable args */

b=format;

while(*b != 0) {
	c=*b;

	if(c == '%') {
		c=*++b;

		switch(c) {

			case's':				/* string */
				s=va_arg(args,const char*);

				outputconsole(s,strlen(s));
				b++;
				break;

			case 'd':				/* signed decimal */
	 				num=va_arg(args,int);
  					itoa(num,z);

				outputconsole(z,strlen(z));

				b++;
				break;

   			case 'u':				/* unsigned decimal */
				num=va_arg(args,size_t);

				itoa(num,z);
				outputconsole(z,strlen(z));
	
				b++;
				break;

			case 'o':			/* octal */
				num=va_arg(args,size_t);

				itoa(num,z);
				outputconsole(z,strlen(z));  
				b++;
				break;

				case 'p':				/* same as x */
				case 'x':				/* same as x */
				case 'X':
				num=va_arg(args,size_t);

				tohex(num,z);
				outputconsole(z,strlen(z));
	
				b++;
				break;
   
			case 'c':				/* character */
				c=(unsigned char) va_arg(args, int);

				p=z;
				*p=c;

				outputconsole(z,strlen(z));
				b++;

				break;

			case '%':
				outputconsole("%",1);  
				b++;

				break;
			}

  }
 else
 {
			outputconsole(&c,1);
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
char *z[MAX_SIZE];
char *bufptr;
int is64bit=FALSE;

bufptr=buf;

va_start(args,format);			/* get start of variable args */
b=format;

while(*b != 0) {
	c=*b;

	if(c == '%') {
		c=*++b;
 
		switch(c) {

		case 'l':
			is64bit=TRUE;
			b++;

		/* fall through */

		case 's':				/* string */
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

			itoa(num,z);
			strcat(bufptr,z);
			bufptr=bufptr+strlen(z)+1;
	
			b++;
			break;

   		case 'o':				/* octal */
			num=va_arg(args,size_t);

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

/*
 * Convert number to hexadecimal string
 *
 * In: uint32_t hex	Number to convert
	   char *buf	Buffer to store converted number
 *
 * Returns nothing
 */

void tohex(uint32_t hex,char *buf) {
size_t count;
uint32_t h;
uint32_t shiftamount;
uint32_t mask;

char *b;
char *hexbuf[] = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"};

mask=0xF0000000;
shiftamount=28;

b=buf;

for(count=1;count<9;count++) {
	h=((hex & mask) >> shiftamount);	/* shift nibbles to end */

	shiftamount=shiftamount-4;
	mask=mask >> 4;  
	strcpy(b++,hexbuf[h]);
}

}

/*
 * Tokenize string into array
 *
 * In: char *linebuf			String to tokenize
	   char *tokens[MAX_PATH][MAX_PATH]	Array to hold tokens
	   char *split			Character delimiter
 *
 * Returns number of tokens
 */

size_t tokenize_line(char *linebuf,char *tokens[MAX_PATH][MAX_PATH],char *split) {
char *token;
size_t tc=0;
size_t count;
char c;
char *d;
char *splitptr;

tc=0;

memset(tokens,0,MAX_PATH);

token=linebuf;

d=tokens[0];

while(*token != 0) { 		
	if(*token == 0 ) break;

	c=-1;
	splitptr=split;

	while(c != 0) {
		c=*splitptr++;
   	
		if(*token == c) {
			token++;
			tc++;
			d=tokens[tc];
   		}
 	 }	

  *d++=*token++;
 }

tc++;
return(tc);
}

/*
 * Raises integer to power
 *
 * In:	n				Base number
 *	e				Exponent
 *
 * Returns number of tokens
 */
size_t ipow(size_t n,size_t e) {
size_t num=n;				/* save number */
size_t count=e;

/* multiple n by base number until end */

do {
	num *= n;
} while(--count > 1);


return(num);
}
 
