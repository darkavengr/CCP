/* 	CCP Version 0.0.1
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
#include <stdarg.h>
#include "mutex.h"
#include "vfs.h"
#include "string.h"
#include "debug.h"

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
char *strptr=str;

if(*str == 0) return(0);			/* null string */

while((char) *strptr++ != 0) count++;		/* find size */

return(count);
}

/*
 * Get length of Unicode string
 *
 * In: string	Input string
       maxlen	Maximum length of string
 *
 * Returns string length
 *
 */

size_t strlen_unicode(char *str,size_t maxlen) {
size_t count;
char *strptr=str;

count=0;

if((char) *str == 0) return(0);				/* null string */

while(maxlen--) {
	if(((char) *strptr++ == 0) && ((char) *strptr++ == 0)) return(count);

	count++;	
}

return(count);
}

/*
 * Copy string
 *
 * In: dest		Destination string
       source		Source string
       size		Number of bytes to copy
 *
 * Returns nothing
 *
 */

char *strncpy(char *dest,char *source,size_t size) {
char *destptr=dest;
char *sourceptr=source;
size_t count=0;

while(*sourceptr != 0) {			/* until end */
	if(size-- == 0) break;	/* too large */

	*destptr++=*sourceptr++;	
}

if(count < (size-1)) *destptr--=0;
}

/*
 * Conatecate string
 *
 * In: str		String to conatecate to
       catstr		String to conatecate
       size		Maximum size of buffer
 *
 * Returns nothing
 *
 */

char *strncat(char *str,char *catstr,size_t size) {
char *strptr=str;
char *catstrptr=catstr;
size_t count=0;

while((char) *strptr++ != 0) count++;

strptr--;		/* overwrite null */
count--;

while((char) *catstrptr != 0) {
	*strptr++=*catstrptr++;			/* copy character */

	if(count++ == (size-1)) break;		/* at end */
}

*strptr=0;
return(str);
}

/*
 * Copy memory
 *
 * In: dest		Address to copy to
       source		Address to copy from
       count		Number of bytes to copy
 *
 * Returns: Pointer to destination
 *
 */

size_t memcpy(void *dest,void *source,size_t count) {
char *sourceptr=source;
char *destptr=dest;

while(count-- > 0) *destptr++=*sourceptr++;
	
return(dest);
}

/*
 * Compare memory
 *
 * In: source		Second address to compare
       dest		First address to compare
       count		Number of bytes to compare
 *
 * Returns different of last source and destination bytes if they do not match, 0 otherwise
 *
 */

size_t memcmp(char *source,char *dest,size_t count) {
while(count > 0) {
	if(*source != *dest) return((char) *dest - (char) *source);
	
	source++;
 	dest++;
	count--;
}

return(0);
}
 
/*
 * Compare string
 *
 * In: source	First string to compare
       dest	Second string to compare
 *
 * Returns different of last source and destination bytes if they do not match, 0 otherwise
 *
 */
size_t strncmp(char *source,char *dest,size_t size) {

while((char) *source == (char) *dest) {
	if(((char) *source == 0) && ((char) *dest == 0)) return(0);		/* same */

	source++;
	dest++;
}

return((char) *dest - (char) *source);
}

/*
 * Compare string case insensitively
 *
 * In: source	First string to compare
       dest	Second string to compare
 *
 * Returns different of last source and destination bytes if they do not match, 0 otherwise
 *
 */
size_t strncmpi(char *source,char *dest,size_t size) {
char *sourcetemp[MAX_SIZE];
char *desttemp[MAX_SIZE];

touppercase(source,sourcetemp);
touppercase(dest,desttemp);

return(strncmp(sourcetemp,desttemp,size));
}

/*
 * Fill memory
 *
 * In: buf	Address to fill
       fillchar Byte to fill with
       size	Number of bytes to fill
 *
 * Returns nothing
 *
 */

void *memset(void *buf,char fillchar,size_t count) {
char *bufptr=buf;

while(count-- > 0) *bufptr++=fillchar;				/* put byte */

return(buf);
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
 * In: wildcard Wildcard
       str 	Input string
 *
 * Returns: 0 if found, -1 if not
 *
 */
size_t wildcard(char *wildcard,char *str) {
char *wildcardptr=wildcard;
char *strptr=str;
char *buf[MAX_SIZE];
char *bufptr;
size_t IsFound=FALSE;
char *startofmultichars;

memset(buf,0,MAX_SIZE);				/* clear buffer */
 
wildcardptr=wildcard;
strptr=str;

if(((char) *strptr++ == '*') && ((char) *strptr == 0)) return(TRUE);	/* match everything */

strptr=str;

while((char) *wildcardptr != 0) {

	if((char) *wildcardptr == '?') {		/* match any single character */
		if((char) *strptr++ == 0) {
			IsFound=FALSE;
		}
		else
		{
			IsFound=TRUE;
		}

		wildcardptr++;
	}
	else if((char) *wildcardptr == '*') {		/* match multiple characters */
		wildcardptr++;

		bufptr=buf;

		/* find multiple characters to search for */

		while((char) *wildcardptr != 0) {
			*bufptr++=*wildcardptr++;

			if(((char) *wildcardptr == '?') || ((char) *wildcardptr == '*')) break;
		}

		/* find start of multi-character pattern */

		startofmultichars=strptr;

		while((char) *startofmultichars != 0) {
			if((char) *startofmultichars == (char) *buf) break;

			startofmultichars++;
		}
	
		if(memcmp(startofmultichars,buf,strlen(buf)) == 0) {
			IsFound=TRUE;
		}
		else
		{
			IsFound=FALSE;
		}
	}
	else					/* literal character */
	{
		if((char) *wildcardptr++ != (char) *strptr++) return(-1);

		IsFound=TRUE;
	}

}

if(IsFound == TRUE) return(0);

return(-1);
}


/*
 * Convert string to uppercase
 *
 * In: string	String to convert
 *     out	Output buffer
 *
 * Returns nothing
 *
 */
void touppercase(char *string,char *out) {
char *strptr=string;
char *outptr=out;

while(*strptr != 0) {			/* until end */
	*outptr=*strptr++;

	if((*outptr >= 'a') && (*outptr <= 'z')) *outptr = *outptr-32;			/* to uppercase */
	
	outptr++;
}

*outptr++=0;
}

/*
 * Convert string to uppercase
 *
 * In: char *string	String to convert
 *
 * Returns nothing
 *
 */
void tolowercase(char *string,char *out) {
char *strptr=string;
char *outptr=out;

while(*strptr != 0) {			/* until end */
	*outptr=*strptr++;

	if((*outptr >= 'A') && (*outptr <= 'Z')) *outptr = *outptr+32;			/* to uppercase */
	
	outptr++;
}

*outptr++=0;
}

/*
 * Replace string using wildcard
 *
 * In: name	Input string
       mask	Wildcard to match
       out	Output string
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
char *newmask[MAX_SIZE];
char *mmask[MAX_SIZE];
char *nname[MAX_SIZE];

touppercase(mask,mmask);					/* convert to uppercase */
touppercase(name,nname);

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
 * In: str	String to truncate
       c	Number of characters to truncate
 *
 * Returns: Address of truncated end
 *
 */
char *strtrunc(char *str,int newsize) {
char *strptr;

strptr=(strlen(str)-newsize);		/* point to new end */
*strptr=0;

return(strptr);
}

/*
 * Convert string to integer
 *
 * In: numstr	String to convert
       base	Base number to use (2,8,10,16)
 *
 * Returns converted number
 *
 */

size_t atoi(char *numstr,size_t base) {
size_t num=0;
char *strptr;
size_t shiftamount=0;

if(base == 10) shiftamount=1;		/* for decimal numbers */

strptr=numstr;
strptr += (strlen(numstr)-1);		/* point to end */

while(strptr >= numstr) {
	if(base == 16) {
		if( ((char) *strptr >= 'A') && ((char) *strptr <= 'F')) {
			num += (((size_t) *strptr-'A')+10) << shiftamount;
		}
		else if( ((char) *strptr >= 'a') && ((char) *strptr <= 'f')) {
			num += (((size_t) *strptr-'a')+10) << shiftamount;	
		}
		else if( ((char) *strptr >= '0') && ((char) *strptr <= '9')) {
			num += ((size_t)  *strptr-'0') << shiftamount;
		}

		shiftamount += 4;
 	}
	else if(base == 8) {
		if( ((char) *strptr >= '0') && ((char) *strptr <= '7')) num += ((size_t) *strptr-'0') << shiftamount;

		shiftamount += 3;
	}
	else if(base == 2) {
		if( ((char) *strptr >= '0') && ((char) *strptr <= '1')) num += ((size_t) *strptr-'0') << shiftamount;

		shiftamount++;
	}
	else if(base == 10) {
 		num += ((size_t) *strptr-'0') * shiftamount;

 		shiftamount *= 10;
	}
	
	strptr--;
}

return(num);
}
/*
 * Convert number to hexadecimal string
 *
 * In:	hex		Number to convert
 *	buf		Buffer to store converted number
 *	bytecount	number of bytes to convert
 * Returns nothing
 */

void tohex(size_t hex,char *buf) {
size_t count;
size_t h;
size_t shiftamount;
size_t mask=0;
size_t found_start=FALSE;

char *b;
char *hexbuf[] = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"};

mask=(size_t) ((size_t) 0xf << ((size_t) sizeof(size_t)*8)-4);

shiftamount=(sizeof(size_t)*8)-4;

b=buf;

for(count=0;count<(sizeof(size_t)*2);count++) {
	h=((hex & mask) >> shiftamount);	/* shift nibbles to end */

	if(h != 0) found_start=TRUE;		/* don't include trailing zeroes */

	shiftamount -= 4;
	mask=mask >> 4;  			/* shift mask */

	if(found_start == TRUE) strncpy(b++,hexbuf[h],MAX_SIZE);	/* copy hex digit */
}

if(found_start == FALSE) strncpy(buf,hexbuf[0],MAX_SIZE);		/* all zeroes */

return;
}

/*
 * Convert number to octal string
 *
 * In:	octal		Number to convert
 *	buf		Buffer to store converted number
 *	bytecount	number of bytes to convert
 * Returns nothing
 */

void tooctal(size_t oct,char *out) {
size_t count;
size_t shiftvalue;
size_t mask;
char *digits[] = { "0","1","2","3","4","5","6","7" };
size_t foundstart=FALSE;

/* size_t is not divisible by 3 so there are 2 bits at the start */

shiftvalue=(sizeof(size_t)*4)-2;
mask=3 << shiftvalue;

if((oct & mask) >> shiftvalue) foundstart=TRUE;	/* don't include trailing zeroes */

if(foundstart == TRUE) strncat(out,digits[(oct & mask) >> shiftvalue],MAX_PATH);

/* get rest of the bits */

shiftvalue=(sizeof(size_t)*4)-5; 	/* shift value */
mask=07 << shiftvalue;			/* bit mask */

for(count=shiftvalue;count != 0;count -= 3) {
	if((oct & mask) >> shiftvalue) foundstart=TRUE;	/* don't include trailing zeroes */

	if(foundstart == TRUE) strncat(out,digits[(oct & mask) >> count],MAX_SIZE);
	mask=mask >> 3;
}

if(foundstart == FALSE) strncpy(out,digits[0],MAX_SIZE);		/* all zeroes */

return;
}

/*
 * Tokenize string into array
 *
 * In: linebuf				String to tokenize
       tokens[MAX_SIZE][MAX_SIZE]	Array to hold tokens
       delim				Character delimiter
 *
 * Returns: Number of tokens
 */

size_t tokenize_line(char *linebuf,char *tokens[MAX_SIZE][MAX_SIZE],char *delim) {
char *token;
size_t tokencount=0;
char *tokenptr;
char *delimptr;

memset(tokens,0,MAX_SIZE);

token=linebuf;

tokenptr=tokens[0];

while(*token != 0) {
	delimptr=delim;			/* point to delimiter characters */

	while(*delimptr != 0) {

		if(*token == *delimptr++) {	/* end of token */
			token++;
			tokencount++;

			*tokenptr=0;		/* put null at end of token */

			delimptr=tokens[tokencount];
			break;
   		}
 	 }	

	 *tokenptr++=*token++;
 }

tokencount++;
*tokens[tokencount]=0;

return(tokencount);
}

/*
 * Raises integer to power
 *
 * In:	n				Base number
 *	e				Exponent
 */
size_t ipow(size_t n,size_t e) {
size_t num=n;				/* save number */

/* multiple n by base number until end */

do {
	num *= n;
} while(--e > 1);


return(num);
}

/*
 * Sign-extends number
 *
 * In:	number				Number to sign-extend
 *	bitnum				Bit number to sign-extend from
 */
size_t signextend(size_t num,size_t bitnum) {
size_t signextendnum;
size_t signextendcount;

signextendnum=(size_t) ((size_t) num & (((size_t) 1 << bitnum)));

for(signextendcount=0;signextendcount != (sizeof(num)*8)-bitnum;signextendcount++) {
	num |= ((size_t) signextendnum);

	signextendnum=((size_t) signextendnum << 1);
}

return((size_t) num);
}

size_t round_up(size_t num,size_t multiple) {
if(num < multiple) return(multiple);

return(num+(multiple-(num % multiple)));
}

size_t round_down(size_t num,size_t multiple) {
if(num < multiple) return(num);

return((num / multiple)*multiple);
}

size_t ksnprintf(char *buf,char *format,size_t size, ...) {
va_list args;
char *formatptr;
char c;
char *strptr;
int int_num;
double double_num;
char *temp_buf[MAX_SIZE];
char *bufptr;
size_t count=0;

bufptr=buf;
formatptr=format;

va_start(args,format);			/* get start of variable arguments */

while(*formatptr != 0) {
	if(count == size) return(count);	/* at end */

	if((char) *formatptr == '%') {
		formatptr++;
 
		switch((char) *formatptr) {
			case 's':				/* string */
				strptr=va_arg(args,const char*);
	
				count += strlen(strptr);	/* add number of bytes put in output buffer */
				if(count >= size) return(count);

				strncat(bufptr,strptr,MAX_SIZE);	/* copy to end of buffer */

				bufptr += strlen(strptr);

				formatptr++;
				break;

			case 'd':				/* signed decimal */
				int_num=va_arg(args,int);

				if((int_num >> ((sizeof(int)*8)-1)) == 1)  strncat(bufptr,"-",MAX_SIZE);	/* add negative sign */
  
				itoa(int_num,temp_buf);
	
				strncat(bufptr,temp_buf,size-count);	/* append to buffer */

				count += size;				/* add number of bytes put in output buffer */
				bufptr += strlen(temp_buf)+1;		/* add to buffer address */

				formatptr++;
				break;

   			case 'u':				/* unsigned decimal */
				int_num=va_arg(args,size_t);

				itoa(int_num,temp_buf);

				strncat(bufptr,temp_buf,size-count);
				bufptr += strlen(temp_buf)+1;
	
				formatptr++;
				break;

   			case 'o':				/* octal */
				int_num=va_arg(args,size_t);

				itoa(int_num,temp_buf);

				strncat(bufptr,temp_buf,size-count);
				count += size;

				bufptr += strlen(temp_buf)+1;

				formatptr++;
				break;

   			case 'p':				/* same as x */
   			case 'x':				/*  lowercase x */
   			case 'X':
				int_num=va_arg(args,size_t);
				tohex(int_num,temp_buf);
 
				strncat(bufptr,temp_buf,size-count);
				count += size;

				bufptr += strlen(temp_buf)+1;
				formatptr++;
 
				break;
   
			case 'c':				/* character */
				c=va_arg(args,size_t);

				*bufptr++=c;			/* add character to buffer */
				formatptr++;
				break;

		case '%':
			strncat(bufptr,"%",size-count);
			break;

		case 'f':
			break;

   		}

 	}
 	else
 	{
		*bufptr++=*formatptr;
		formatptr++;
	}

}

va_end(args);

return(count);
}

