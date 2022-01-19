#define MAX_PATH	255
#define COMMAND_PERMENANT 1
#define COMMAND_STEP	  2

#define COPY_OVERWRITE	1
#define COPY_ASCII	2
#define COPY_BINARY	4

#define DELETE_PROMPT	1

#define DIR_PAUSE	1
#define DIR_WIDE	2
#define DIR_ORDER	4
#define DIR_SUBDIR	8
#define DIR_BARE	16
#define DIR_LOWER	32
#define DIR_ATTRIBS	64

#define	DIR_COUNT	10

#define MAX_READ_SIZE	512

typedef struct {
 char *name[MAX_PATH];
 char *val[MAX_PATH];
 struct VARIABLES *next;
} VARIABLES;

