#include <stddef.h>

#define	NULL			0
#define TRUE			1
#define FALSE 			0

#define FILE_REGULAR		0
#define FILE_CHAR_DEVICE	1
#define FILE_BLOCK_DEVICE	2
#define FILE_DIRECTORY		4

#define _FILE			0
#define _DIR 			1

#define _O_RDONLY 		1
#define _O_WRONLY 		2
#define _O_RDWR 		_O_RDONLY | _O_WRONLY

#define FILE_ACCESS_EXCLUSIVE	4
#define MAX_PATH		255
#define VFS_MAX 		1024

#define	_READ 			0
#define _WRITE 			1

#define stdin			0
#define stdout			1
#define stderr			2

#define EOF 		_ERROR

#define SEEK_SET	0	/* Seek from beginning of file */
#define SEEK_CUR	1	/* Seek from current position */
#define SEEK_END	2	/* Seek from end of file */


typedef struct {
 uint8_t name[MAX_PATH];
 uint8_t magicnumber[MAX_PATH];
 size_t size;
 size_t location;
 size_t (*findfirst)(char *name,void *);	/* handlers */
 size_t (*findnext)(char *name,void *);
 size_t (*read)(size_t,void *,size_t);
 size_t (*write)(size_t,void *,size_t);
 size_t (*rename)(char *,char *);
 size_t (*delete)(char *);
 size_t (*mkdir)(char *);
 size_t (*rmdir)(char *);
 size_t (*create)(char *);
 size_t (*chmod)(char *,size_t);
 size_t (*getfiletd)(char *);
 size_t (*setfiletd)(char *,size_t,size_t);
 size_t (*getstartblock)(char *);
 size_t (*getnextblock)(size_t);
 size_t (*seek)(size_t,size_t,size_t);
 struct FILESYSTEM *next;
} FILESYSTEM;

typedef struct {
 uint8_t seconds;
 uint8_t minutes;
 uint8_t hours;
 uint8_t day;
 uint8_t month;
 uint16_t year;
}  __attribute__((packed)) TIMEBUF;


typedef struct {
 char *filename[MAX_PATH];
 char *dirname[MAX_PATH];
 size_t drive;
} SPLITBUF;


typedef struct {
 uint8_t filename[MAX_PATH]; 		
 size_t attribs;
 TIMEBUF timebuf;
 uint32_t filesize;
 uint64_t startblock;
 size_t drive;
 size_t currentblock;
 size_t findblock; 	/* block containing the directory entry for this file */
 size_t dirent; 		/* directory entry number */
 size_t access; 		/* access */
 size_t currentpos;
 size_t flags;
 size_t handle;
 size_t (*blockio)(size_t,size_t,size_t,void *);			/* function pointer */
 size_t (*charioread)(size_t,void *);			/* function pointer */
 size_t (*chariowrite)(size_t,void *);			/* function pointer */
 size_t (*ioctl)(size_t handle,unsigned long request,void *buffer);
 uint32_t findlastblock; /* last block read by find() */
 size_t findentry;
 size_t owner_process;
 uint8_t searchfilename[MAX_PATH]; 		
 struct FILERECORD *next;
} __attribute__((packed)) FILERECORD;

