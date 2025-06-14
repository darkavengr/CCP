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
#include "kernelhigh.h"
#include "mutex.h"
#include "vfs.h"
#include "initrd.h"
#include "errors.h"
#include "bootinfo.h"
#include "debug.h"
#include "memorymanager.h"
#include "string.h"

size_t initrd_findfirst(char *filename,FILERECORD *buf);
size_t initrd_findnext(char *filename,FILERECORD *buf);
size_t initrd_find(size_t op,char *filename,FILERECORD *buf);
size_t initrd_read(size_t handle,void *buf,size_t size);
size_t initrd_init(void);
size_t initrd_io(size_t op,size_t drive,uint64_t block,char *buf);

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
//	asm("xchg %bx,%bx");

	return(initrd_find(FALSE,filename,buf));
}

/*
 * Find next file in INITRD
 *
 * In:  name	Filename or wildcard of file to find
	buffer	Buffer
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
 * In:  
	op	0=First, 1=Next
	name    Filename or wildcard
	buffer	Buffer to hold file information
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t initrd_find(size_t op,char *filename,FILERECORD *buf) {
BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;
size_t newfindblock;
char *fullpath[MAX_PATH];
SPLITBUF split;
size_t filefound=FALSE;
char *tempname[MAX_PATH];
TAR_HEADER *tarptr;

getfullpath(filename,fullpath);

splitname(fullpath,&split);

if(op == 0) {
	buf->findlastblock=0;

	tarptr=boot_info->initrd_start+KERNEL_HIGH;		/* first */

	memset(buf,0,sizeof(FILERECORD));
}
else
{
	tarptr=(boot_info->initrd_start+KERNEL_HIGH)+(buf->findlastblock*TAR_BLOCK_SIZE);		/* first */
}

touppercase(split.filename,split.filename);

/* search for file */

do {
	tarptr=(boot_info->initrd_start+KERNEL_HIGH)+(buf->findlastblock*TAR_BLOCK_SIZE);	/* point to next file */

	if(*tarptr->name == 0) {		/* end of directory */
		setlasterror(END_OF_DIRECTORY);
		return(-1);
	}

	filefound=FALSE;
	
	touppercase(tarptr->name,tempname);			/* convert to uppercase */

	if(wildcard(split.filename,tempname) == 0) { 	/* if file found */      	

		strncpy(buf->filename,tarptr->name,MAX_PATH);	/* copy information */

		buf->filesize=atoi(tarptr->size,8);
		buf->drive=25;

		buf->startblock=buf->findlastblock+1;

		filefound=TRUE;
	}

	newfindblock=(atoi(tarptr->size,8) / TAR_BLOCK_SIZE)+1;		/* get number of blocks */
	if(atoi(tarptr->size,8) % TAR_BLOCK_SIZE) newfindblock++;		/* round up */

	buf->findlastblock += newfindblock;

	if(filefound == TRUE) break;

} while(*tarptr->name != 0);

if(filefound == FALSE) {
	setlasterror(FILE_NOT_FOUND);
	return(-1);
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

if((onext.currentpos+size) > (boot_info->initrd_start+boot_info->initrd_size)) {	/* reading past end of file */
	setlasterror(INPUT_PAST_END);
	return(-1);
}


dataptr=boot_info->initrd_start+KERNEL_HIGH;		/* point to start */
dataptr += (onext.startblock*TAR_BLOCK_SIZE);
dataptr += onext.currentpos;				/* point to current position */

memcpy(buf,dataptr,size);				/* copy data */

onext.currentpos += size;				/* point to next data */

if(updatehandle(handle,&onext) == -1) {				/* update file information */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

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
MAGIC magicdata[] = { { "ustar  ",7,257 } };
BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;

if(boot_info->initrd_size == 0) {	/* no initrd */
	kprintf_direct("kernel: No initrd found\n");
	return(-1);
}

/* create block device */
strncpy(bd.name,"INITRD",MAX_PATH);
bd.blockio=&initrd_io;		/*setup struct */
bd.ioctl=NULL;
bd.physicaldrive=0;
bd.drive=25;
bd.flags=0;
bd.startblock=(uint64_t) 0;
bd.sectorsperblock=1;
add_block_device(&bd);

memset(&fs,0,sizeof(FILESYSTEM));

/* register filesystem driver */
strncpy(fs.name,"INITRDFS",MAX_PATH);	/* name */

fs.magic_count=1;		/* number of magic number entries */
memcpy(&fs.magicbytes,&magicdata,fs.magic_count*sizeof(MAGIC));	/* copy magic number data */

fs.findfirst=&initrd_findfirst;	/* add handlers */
fs.findnext=&initrd_findnext;
fs.read=&initrd_read;
fs.write=NULL;			/* not used */
fs.rename=NULL;
fs.unlink=NULL;
fs.mkdir=NULL;
fs.rmdir=NULL;
fs.create=NULL;
fs.chmod=NULL;
fs.touch=NULL;
fs.write=NULL;
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
size_t initrd_io(size_t op,size_t drive,uint64_t block,char *buf) { 
BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;
char *tptr;

if((block*TAR_BLOCK_SIZE) > boot_info->initrd_size) {			/* out of range */
	setlasterror(INVALID_BLOCK_NUMBER);
	return(-1);
}

tptr=boot_info->initrd_start+(block*TAR_BLOCK_SIZE)+KERNEL_HIGH;

memcpy(buf,tptr,TAR_BLOCK_SIZE);		/* copy data */
	
setlasterror(NO_ERROR);
return(0);
}

size_t load_modules_from_initrd(void) {
FILERECORD findmodule;
char *filename[MAX_PATH];
SPLITBUF split;
BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;

if(boot_info->initrd_size == 0) return(-1);			/* no initrd */

/* loop through modules in initrd and load them */

if(findfirst("Z:\\*.o",&findmodule) == -1) return(FILE_NOT_FOUND);

do {
	kprintf_direct("Loading module Z:\\%s\n",findmodule.filename);

	memset(filename,0,MAX_PATH);
	ksnprintf(filename,"Z:\\%s",MAX_PATH,findmodule.filename);

	if(load_kernel_module(filename,NULL) == -1) {
		kprintf_direct("kernel: Error loading kernel module %s from initrd (%s)\n",filename,kstrerr(getlasterror()));
		return(-1);
	}

} while(findnext("Z:\\*.o",&findmodule) != -1);

setlasterror(NO_ERROR);
return(0);
}

