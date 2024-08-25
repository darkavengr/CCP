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

#define MAX_IRQ		255

typedef struct {
	char *name[MAX_PATH];
	size_t (*charioread)(void *,size_t);			/* function pointer */
	size_t (*chariowrite)(void *,size_t);			/* function pointer */
	size_t (*ioctl)(size_t handle,unsigned long request,void *buffer);
	size_t flags;
	void *data;
	struct CHARACTERDEVICE *next;
} CHARACTERDEVICE;

typedef struct {
	uint8_t name[MAX_PATH];
	void *dbuf;
	size_t (*blockio)(size_t,size_t,uint64_t,void *);			/* function pointers */
	size_t (*ioctl)(size_t handle,unsigned long request,void *buffer);
	size_t flags;
	size_t drive;
	uint64_t startblock;
	size_t physicaldrive; 
	size_t sectorsperblock;
	size_t sectorsize;
	size_t sectorspertrack;
	size_t numberofheads;
	size_t numberofsectors;
	void *superblock;
	MUTEX mutex;
	struct BLOCKDEVICE *next;
}  BLOCKDEVICE;

size_t add_block_device(BLOCKDEVICE *driver);
size_t add_char_device(CHARACTERDEVICE *device);
size_t blockio(size_t op,size_t drive,uint64_t block,void *buf);
size_t findcharacterdevice(char *name,CHARACTERDEVICE *buf);
size_t getblockdevice(size_t drive,BLOCKDEVICE *buf);
size_t getdevicebyname(char *name,BLOCKDEVICE *buf);
size_t update_block_device(size_t drive,BLOCKDEVICE *driver);
size_t remove_block_device(char *name);
size_t remove_char_device(char *name);
size_t allocatedrive(void);
void devicemanager_init(void);

