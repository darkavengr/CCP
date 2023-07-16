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

#include <stdint.h>
#include "../header/kernelhigh.h"
#include "../processmanager/mutex.h"
#include "vfs.h"
#include "../devicemanager/device.h"
#include "initrd.h"
#include "../header/errors.h"
#include "../header/bootinfo.h"

size_t initrd_findfirst(char *filename,FILERECORD *buf);
size_t initrd_findnext(char *filename,FILERECORD *buf);
size_t initrd_find(size_t op,char *filename,FILERECORD *buf);
initrd_read(int ignore,size_t size,void *buf);
size_t initrd_init(void);
size_t initrd_io(size_t op,size_t drive,size_t block,char *buf);

TAR_HEADER *tarptr;

/*
 * Find first file in INITRD
 *
 * In:  name	Filename or wildcard of file to find
        buffer	Buffer to hold information about files
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t initrd_findfirst(char *filename,FILERECORD *buf) {
 return(initrd_find(FALSE,filename,buf));
}

/*
 * Find next file in INITRD
 *
 * In:  name	Filename or wildcard of file to find
        buffer	Buffer to hold information about files
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t initrd_findnext(char *filename,FILERECORD *buf) {
 return(initrd_find(TRUE,filename,buf));
}

/*
 * Find file in INITRD
 *
 * In:  name	Filename or wildcard of file to find
        buffer	Buffer to hold information about files
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t initrd_find(size_t op,char *filename,FILERECORD *buf) {
size_t size;
BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;
size_t ptr;

if(op == FALSE) tarptr=boot_info->initrd_start+KERNEL_HIGH;		/* first */

if(*tarptr->name == 0) {			/* last file */
 setlasterror(FILE_NOT_FOUND);
 return(-1);
}

memset(buf,0,sizeof(FILERECORD));

strcpy(buf->filename,tarptr->name);	/* copy information */

buf->filesize=atoi(tarptr->size,8);
buf->drive='Z';

ptr=tarptr;
ptr += ((buf->filesize / 512) + 1) * 512;

if(buf->filesize % TAR_BLOCK_SIZE) ptr += TAR_BLOCK_SIZE;

tarptr=ptr;
}

/*
 * Read from INITRD
 *
 * In:  handle	File handle
	addr	Buffer to read to
	size	Number of bytes to read
 *
 * Returns: -1 on error, 0 on success
 *
 */

initrd_read(int ignore,size_t size,void *buf) {
void *dataptr;

size=atoi(tarptr->size,8);	/* get size */
dataptr=tarptr+sizeof(TAR_HEADER);	/* data follows header */

memcpy(buf,dataptr,size);

return(size);
}

/*
 * Initialize INITRD
 *
 * In:  nothing
 *
 * Returns: nothing
 *
 */

size_t initrd_init(void) {
FILESYSTEM fs;
FILERECORD filerecord;
size_t retval;
BLOCKDEVICE bd;
uint8_t magic[] = { 'u','s','t','a','r',' ',' ' };

/* create block device */
strcpy(bd.dname,"INITRD");
bd.blockio=&initrd_io;		/*setup struct */
bd.ioctl=NULL;
bd.physicaldrive=0;
bd.drive=25;
bd.flags=0;
bd.startblock=0;
bd.sectorsperblock=1;
add_block_device(&bd);

memset(&fs,0,sizeof(FILESYSTEM));

/* register filesystem driver */
strcpy(fs.name,"INITRDFS");	/* name */

memcpy(fs.magicnumber,magic,INITRD_MAGIC_SIZE);
fs.size=INITRD_MAGIC_SIZE;
fs.location=257;
fs.findfirst=&initrd_findfirst;
fs.findnext=&initrd_findnext;
fs.read=&initrd_read;
fs.write=NULL;			/* not used */
fs.rename=NULL;
fs.delete=NULL;
fs.mkdir=NULL;
fs.rmdir=NULL;
fs.create=NULL;
fs.chmod=NULL;
fs.setfiletd=NULL;
fs.write=NULL;
fs.write=NULL;
fs.getstartblock=NULL;
fs.getnextblock=NULL;
fs.seek=NULL;

register_filesystem(&fs);
}

/*
 * INITRD I/O function
 *
 * In:  op	Ignored
        buf	Buffer
	len	Number of bytes to read
 *
 *  Returns: 0 on success or -1
 *
 */
size_t initrd_io(size_t op,size_t drive,size_t block,char *buf) { 
 BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;
 char *tptr;

 if((block*512) > boot_info->initrd_size) {			/* out of range */
  setlasterror(INVALID_BLOCK);
  return(-1);
 }

 tptr=boot_info->initrd_start;
 tptr += KERNEL_HIGH;

 memcpy(buf,tptr,TAR_BLOCK_SIZE);		/* copy data */
 
 setlasterror(NO_ERROR);
 return(0);
}
