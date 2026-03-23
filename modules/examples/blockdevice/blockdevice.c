#include <stdint.h>
#include <stddef.h>
#include "errors.h"
#include "device.h"

#define MODULE_INIT floppy_init

size_t example_block_device_init(char *init) {
BLOCKDEVICE blockdevice;

blockdevice.blockdevice_ioctl=block_device_ioctl;	/* ioctl() handler */
blockdevice.blockio=&block_device_io;	/* block I/O handler */
blockdevice.flags=0;			/* flags */
blockdevice.startblock=0;		/* start block of volume - if partitioned drive this number should be got from the partition table */
blockdevice.sectorsperblock=1;

strncpy(blockdevice.name,"EXAMPLE_BLOCK_DEVICE",MAX_PATH);	/* device name */

blockdevice.physicaldrive=0;		/* physical drive number if different from logical drive number */
blockdevice.drive=alloctedrive();	/* logical drive number */

if(add_block_device(&blockdevice) == -1) {	/* add block device */
	kprintf_direct("example_block_device_init: Can't register block device %s: %s\n",blockdevice.name,kstrerr(getlasterror()));
	return(-1);
}

setlasterror(NO_ERROR);
return(0);
}

/*
 * Block device I/O function
 *
 * In:  op	Operation (0=read,1=write)
	buf	Buffer
	len	Number of bytes to read/write
 *
 *  Returns: 0 on success, -1 on error
 *
 */	
size_t block_device_io(size_t op,size_t physdrive,uint64_t block,uint16_t *buf) {

setlasterror(NO_ERROR);
return(0);
}

/*
 * Block device ioctl handler
 *
 * In:  handle	Handle created by open() to reference device
	       request Request number
	       buffer  Buffer
 *
 *  Returns: -1 on error, 0 on success
 *
 */
size_t example_block_device_ioctl(size_t handle,unsigned long request,char *buffer) {

setlasterror(NO_ERROR);
return(0);
}

