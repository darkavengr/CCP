#include <stdint.h>
#include <stddef.h>

#define DEVICE_LOCKED			1
#define DEVICE_FIXED			2
#define DEVICE_USE_DATA			4

#define CRITICAL_ERROR_ABORT		1
#define CRITICAL_ERROR_RETRY		2
#define CRITICAL_ERROR_FAIL		0


#define MAX_PATH	255

#define ABORT 2
#define RETRY 1
#define FAIL 4

#define _READ 0
#define _WRITE 1

typedef struct {
 char *name[MAX_PATH];
 unsigned int val;
} kernelsymbol;

typedef struct {
 char *dname[MAX_PATH];
 unsigned int (*chario)(unsigned int,unsigned int,void *);			/* function pointer */
 unsigned int (*ioctl)(size_t handle,unsigned long request,void *buffer);
 unsigned int flags;
 void *data;
 struct CHARACTERDEVICE *next;
} CHARACTERDEVICE;

typedef struct {
 uint8_t dname[MAX_PATH];
 void *dbuf;
 unsigned int (*blockio)(unsigned int,unsigned int,size_t,void *);			/* function pointers */
 unsigned int (*ioctl)(size_t handle,unsigned long request,void *buffer);
 unsigned int flags;
 unsigned int drive;
 uint64_t startblock;
 unsigned int physicaldrive; 
 unsigned int sectorsperblock;
 unsigned int sectorsize;
 unsigned int sectorspertrack;
 unsigned int numberofheads;
 unsigned int numberofsectors;
 void *superblock;
 struct BLOCKDEVICE *next;
}  BLOCKDEVICE;


