#include <stddef.h>
#include "errors.h"
#include "time.h"
#include "device.h"

#ifndef VFS_H
#define VFS_H
#define	NULL			0
#define TRUE			1
#define FALSE 			0

#define FILE_REGULAR		1
#define FILE_CHARACTER_DEVICE	2
#define FILE_BLOCK_DEVICE	4
#define FILE_DIRECTORY		8
#define FILE_POS_MOVED_BY_SEEK	16
#define FILE_FIFO		32

#define _FILE			0
#define _DIR 			1

#define O_RDONLY 		1
#define O_WRONLY 		2
#define O_CREAT			4
#define O_NONBLOCK		8
#define O_SHARED		16
#define O_TRUNC			32
#define O_RDWR 			O_RDONLY | O_WRONLY

#define MAX_PATH		255
#define VFS_MAX 		10

#define stdin			0
#define stdout			1
#define stderr			2

#define EOF 		_ERROR

#define SEEK_SET	0	/* Seek from beginning of file */
#define SEEK_CUR	1	/* Seek from current position */
#define SEEK_END	2	/* Seek from end of file */

typedef struct {
	uint8_t magicnumber[MAX_PATH];
	size_t size;
	size_t location;
} MAGIC;
	
typedef struct {
	uint8_t name[MAX_PATH];
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
	size_t magic_count;
	MAGIC magicbytes[VFS_MAX];
} FILESYSTEM;

typedef struct {
	uint8_t filename[MAX_PATH];
	uint8_t dirname[MAX_PATH];
	size_t drive;
} SPLITBUF;

typedef struct {
	char *buffer;
	char *bufptr;
	size_t size;
	MUTEX mutex;
	struct PIPE *next;
} PIPE;

typedef struct {
	uint8_t filename[MAX_PATH];
	size_t attribs;
	TIME create_time_date;
	TIME last_written_time_date;
	TIME last_accessed_time_date;
	uint64_t filesize;
	uint64_t startblock;
	size_t drive;
	uint64_t currentblock;
	size_t dirent;
	size_t access;
	size_t currentpos;
	size_t flags;
	size_t handle;
	BLOCKDEVICE blockdevice;
	size_t (*charioread)(size_t,void *);
	size_t (*chariowrite)(size_t,void *);	
	size_t (*ioctl)(size_t handle,unsigned long request,void *buffer);
	uint64_t findlastblock;
	size_t findentry;
	size_t owner_process;
	uint8_t searchfilename[MAX_PATH];
	PIPE *pipe;
	PIPE *pipereadprevious;
	PIPE *pipelast;
	PIPE *pipereadptr;
	struct FILERECORD *next;		
} __attribute__((packed)) FILERECORD;
#endif

size_t findfirst(char *name,FILERECORD *buf);
size_t findnext(char *name,FILERECORD *buf);
size_t open(char *filename,size_t access);
size_t unlink(char *name);
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
size_t dup_internal(size_t handle,size_t desthandle,size_t sourcepid,size_t destpid);
size_t seek(size_t handle,size_t pos,size_t whence);
size_t tell(size_t handle);
size_t touch(char *filename,TIME *create_time_date,TIME *last_modified_time_date,TIME *last_accessed_time_date);
size_t register_filesystem(FILESYSTEM *newfs);
size_t detect_filesystem(size_t drive,FILESYSTEM *buf);
size_t gethandle(size_t handle,FILERECORD *buf);
size_t getfilesize(size_t handle);
size_t splitname(char *name,SPLITBUF *splitbuf);
size_t updatehandle(size_t handle,FILERECORD *buf);
size_t change_file_owner_pid(size_t handle,size_t pid);
size_t init_console_device(size_t type,size_t handle,void *cptr);
size_t filemanager_init(void);
void shut(void);
size_t pipe(void);
size_t writepipe(FILERECORD *entry,void *addr,size_t size);
size_t readpipe(FILERECORD *entry,char *addr,size_t size);
void closepipe(FILERECORD *entry);

