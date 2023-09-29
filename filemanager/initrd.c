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
size_t initrd_read(size_t handle,void *buf,size_t size);
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
BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;
size_t newfindblock;

if(op == FALSE) {
	tarptr=boot_info->initrd_start+KERNEL_HIGH;		/* first */

	memset(buf,0,sizeof(FILERECORD));
}
else
{
	tarptr=(boot_info->initrd_start+KERNEL_HIGH)+(buf->findblock*TAR_BLOCK_SIZE);		/* first */
}

if(*tarptr->name == 0) {			/* last file */
	setlasterror(FILE_NOT_FOUND);
	return(-1);
}

strcpy(buf->filename,tarptr->name);	/* copy information */

buf->filesize=atoi(tarptr->size,8);
buf->drive=25;

newfindblock=(buf->filesize / TAR_BLOCK_SIZE)+1;
if(newfindblock % TAR_BLOCK_SIZE) newfindblock++;

buf->findblock += newfindblock;

if(op == FALSE) {
	buf->startblock=1;
}
else
{
	buf->startblock=buf->findblock+1;
}

setlasterror(NO_ERROR);
return(0);
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

size_t initrd_read(size_t handle,void *buf,size_t size) {
FILERECORD onext;
char *dataptr;
BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;

if(gethandle(handle,&onext) == -1) {	/* bad handle */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

//if((onext.currentpos+size) > (boot_info->initrd_start+boot_info->initrd_size)) {	/* reading past end of file */
// setlasterror(INPUT_PAST_END);
// return(-1);
//}

dataptr=boot_info->initrd_start+KERNEL_HIGH;		/* point to start */

dataptr += (onext.startblock*TAR_BLOCK_SIZE);
dataptr += onext.currentpos;				/* point to current position */

memcpy(buf,dataptr,size);				/* copy data */

onext.currentpos += size;				/* point to next data */

updatehandle(handle,&onext);				/* update file information */

return(size);
}

size_t initrd_seek(size_t handle,size_t pos,size_t whence) {
size_t count=0;
size_t sb;
size_t posp;
FILERECORD onext;
SPLITBUF splitbuf;

if(gethandle(handle,&onext) == -1) {	/* bad handle */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

switch(whence) {

	case SEEK_SET:				/*set current position */
		onext.currentpos=pos;
		break;

	case SEEK_END:
		onext.currentpos=onext.filesize+pos;	/* end+pos */
		break;

	case SEEK_CUR:				/* current location+pos */
		onext.currentpos=onext.currentpos+pos;
		break;
}

setlasterror(NO_ERROR);
updatehandle(handle,&onext);		/* update */
return(onext.currentpos);
}

uint64_t initrd_getstartblock(char *filename) {
FILERECORD find;

findfirst(filename,&find);

return(find.startblock);
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
fs.getstartblock=&initrd_getstartblock;
fs.seek=&initrd_seek;

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

if((block*TAR_BLOCK_SIZE) > boot_info->initrd_size) {			/* out of range */
	setlasterror(INVALID_BLOCK);
	return(-1);
}

tptr=boot_info->initrd_start;
tptr += (block*TAR_BLOCK_SIZE);
tptr += KERNEL_HIGH;

memcpy(buf,tptr,TAR_BLOCK_SIZE);		/* copy data */
	
setlasterror(NO_ERROR);
return(0);
}

size_t load_modules_from_initrd(void) {
FILERECORD findmodule;
char *filename[MAX_PATH];
SPLITBUF split;
size_t findresult;

/* loop through modules in initrd and load them */

findresult=findfirst("Z:\\*.o",&findmodule);
if(findresult == -1) return(FILE_NOT_FOUND);


do {
	kprintf_direct("Loading module Z:\\%s\n",findmodule.filename);

	ksprintf(filename,"Z:\\%s",findmodule.filename);

	if(strcmp(findmodule.filename,"null.o") == 0) asm("xchg %bx,%bx");

	if(load_kernel_module(filename,NULL) == -1) {	/* load module */
		kprintf_direct("kernel: Error loading kernel module %s from initrd\n",filename);	
	}

	  findresult=findnext("Z:\\*.o",&findmodule);
	  if(findresult == -1) break;
} while(findresult != -1);


return(NO_ERROR);
}

