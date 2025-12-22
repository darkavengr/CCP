#include <stdint.h>
#include <stddef.h>

#define MAX_SIZE	256
#define TRUE	1
#define FALSE	0

#define SECONDS_IN_YEAR		31557600 
#define SECONDS_IN_MONTH 2629800
#define SECONDS_IN_DAY	86400
#define SECONDS_IN_HOUR	3600
#define SECONDS_IN_MINUTE 60

size_t strlen(char *str);
size_t strlen_unicode(char *str,size_t maxlen);
char *test_strncpy(char *dest,char *source,size_t size);
char *strncat(char *d,char *s,size_t size);
size_t memcpy(void *d,void *s,size_t c);
size_t memcmp(char *source,char *dest,size_t count);
size_t strncmp(char *source,char *dest,size_t size);
void *memset(void *buf,char fillchar,size_t count);
size_t itoa(size_t n, char s[]);
void reverse(char s[]);
size_t wildcard(char *mask,char *filename);
void touppercase(char *string,char *out);
size_t wildcard_rename(char *name,char *mask,char *out);
size_t atoi(char *numstr,size_t base);
size_t ksnprintf(char *buf,char *format,size_t size, ...);
void tohex(size_t hex,char *buf);
size_t tokenize_line(char *linebuf,char *tokens[MAX_SIZE][MAX_SIZE],char *delim);
size_t signextend(size_t num,size_t bitnum);
void tolowercase(char *string,char *out);
char *strtrunc(char *str,int newsize);

