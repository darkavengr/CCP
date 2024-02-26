#include <stddef.h>
#include "../header/errors.h"
#include "../header/time.h"

#define	NULL			0
#define TRUE			1
#define FALSE 			0

#define FILE_REGULAR		0
#define FILE_CHAR_DEVICE	1
#define FILE_BLOCK_DEVICE	2
#define FILE_DIRECTORY		4
#define FILE_POS_MOVED_BY_SEEK	8

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
	size_t (*unlink)(char *);
	size_t (*mkdir)(char *);
	size_t (*rmdir)(char *);
	size_t (*create)(char *);
	size_t (*chmod)(char *,size_t);
	size_t (*touch)(char *,size_t,size_t,size_t);
	size_t (*getstartblock)(char *);
	struct FILESYSTEM *next;
} FILESYSTEM;

typedef struct {
	char *filename[MAX_PATH];
	char *dirname[MAX_PATH];
	size_t drive;
} SPLITBUF;


typedef struct {
	uint8_t filename[MAX_PATH]; 		
	size_t attribs;
	TIME create_time_date;
	TIME last_written_time_date;
	TIME last_accessed_time_date;
	uint32_t filesize;
	uint64_t startblock;
	size_t drive;
	uint64_t currentblock;
	size_t dirent; 		/* directory entry number */
	size_t access; 		/* access */
	size_t currentpos;
	size_t flags;
	size_t handle;
	size_t (*blockio)(size_t,size_t,size_t,void *);			/* function pointer */
	size_t (*charioread)(size_t,void *);			/* function pointer */
	size_t (*chariowrite)(size_t,void *);			/* function pointer */
	size_t (*ioctl)(size_t handle,unsigned long request,void *buffer);
	uint64_t findlastblock; /* last block read by find() */
	size_t findentry;
	size_t owner_process;
	uint8_t searchfilename[MAX_PATH]; 		
	struct FILERECORD *next;
} __attribute__((packed)) FILERECORD;

size_t findfirst(char *name,FILERECORD *buf);
size_t findnext(char *name,FILERECORD *buf);
size_t open(char *filename,size_t access);
size_t delete(char *name);
size_t rename(char *oldname,char *newname);
size_t create(char *name);
size_t rmdir(char *name);
size_t mkdir(char *name);
size_t chmod(char *name,size_t attribs);
size_t read(size_t handle,void *addr,size_t size);
size_t write(size_t handle,void *addr,size_t size);
size_t close(size_t handle); 
size_t dup(size_t handle);
size_t dup2(size_t oldhandle,size_t newhandle);
size_t register_filesystem(FILESYSTEM *newfs);
size_t detect_filesystem(size_t drive,FILESYSTEM *buf);
size_t getfullpath(char *filename,char *buf);
size_t seek(size_t handle,size_t pos,size_t whence);
size_t tell(size_t handle);
size_t setfiletimedate(char *filename,TIME *create_time_date,TIME *last_modified_time_date,TIME *last_accessed_time_date);
size_t gethandle(size_t handle,FILERECORD *buf);
size_t getfilesize(size_t handle);
size_t splitname(char *name,SPLITBUF *splitbuf);
size_t updatehandle(size_t handle,FILERECORD *buf);
size_t get_filename_token(char *filename,void *buf);
size_t get_filename_token_count(char *filename);
size_t change_file_owner_pid(size_t handle,size_t pid);
size_t init_console_device(size_t type,size_t handle,void *cptr);
size_t openfiles_init(size_t a,size_t b);
void shut(void);
size_t seek(size_t handle,size_t pos,size_t whence);

