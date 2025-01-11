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
#include <stddef.h>
#include "kernelhigh.h"
#include "errors.h"
#include "mutex.h"	
#include "device.h"
#include "vfs.h"
#include "bootinfo.h"
#include "debug.h"
#include "memorymanager.h"

FILERECORD *openfiles=NULL;
FILERECORD *openfiles_last=NULL;
FILESYSTEM *filesystems=NULL;
MUTEX vfs_mutex;
size_t highest_handle=0;
char *old;

/*
 * Find first file in directory
 *
 * In:  name	Filename or wildcard of file to find
	buffer	Buffer to hold information about files
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t findfirst(char *name,FILERECORD *buf) {
FILESYSTEM fs;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];

getfullpath(name,fullname);

splitname(fullname,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1)	return(-1);	/* detect filesystem */

if(fs.findfirst == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.findfirst(name,buf));				/* call handler */
}

/*
 * Find next file in directory
 *
 * In:  name	Filename or wildcard of file to find
		  buffer	Buffer to hold information about files
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t findnext(char *name,FILERECORD *buf) {
FILESYSTEM fs;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];

getfullpath(name,fullname);

splitname(fullname,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) return(-1);	/* detect filesystem */

if(fs.findnext == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.findnext(name,buf));		/* Call via function pointer */
}


/*
 * Open file
 *
 * In:  name	File to open
	flags	File mode, see vfs.h
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t open(char *filename,size_t access) {
size_t blockcount;
size_t handle;
FILERECORD *next;
FILERECORD *last;
char *fullname[MAX_PATH];
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
CHARACTERDEVICE chardevice;
FILERECORD dirent;

lock_mutex(&vfs_mutex);

/* check if:
	 handle exists and is owned by the caller and return handle if it is,
	 handle exists and is not owned by the called and is not opened for exclusive access. Return error if it is.
*/

next=openfiles;
handle=0;

while(next != NULL) {					/* return error if open */
	last=next;

	if(strncmpi(filename,next->filename,MAX_PATH) == 0) {	/* found filename */
	
		if(next->owner_process == getpid()) {
			unlock_mutex(&vfs_mutex);
			return(next->handle);	/* opening by owner process */
		}

		if((next->access & O_SHARED) == 0) {		/* reopening file with exclusive access */
			unlock_mutex(&vfs_mutex);

			setlasterror(ACCESS_DENIED);
			return(-1);
		}
	}

	next=next->next;
}

/* If opening block device */

if(getdevicebyname(filename,&blockdevice) == 0) {		/* get device info */
	if(openfiles == NULL) {
		openfiles=kernelalloc(sizeof(FILERECORD));					/* add new entry */
		next=openfiles;
	}
	else
	{
		last->next=kernelalloc(sizeof(FILERECORD));					/* add new entry */
		next=last->next;
	}

	strncpy(next->filename,blockdevice.name,MAX_PATH);

	next->currentblock=0;
	next->currentpos=0;
	next->owner_process=getpid();
	next->flags=FILE_BLOCK_DEVICE;
	next->handle=++highest_handle;
	next->access=access;

	memcpy(&next->blockdevice,&blockdevice,sizeof(BLOCKDEVICE));	/* copy device information */
	next->next=NULL;
	
	openfiles_last=next;				/* save pointer to end */

	unlock_mutex(&vfs_mutex);

	setlasterror(NO_ERROR);
	return(next->handle);					/* return handle */
}

/* If opening character device */

if(findcharacterdevice(filename,&chardevice) == 0) {	
	if(openfiles == NULL) {
		openfiles=kernelalloc(sizeof(FILERECORD));					/* add new entry */
		next=openfiles;
	}
	else
	{
		last->next=kernelalloc(sizeof(FILERECORD));					/* add new entry */
		next=last->next;
	}

	strncpy(next->filename,chardevice.name,MAX_PATH);

	next->charioread=chardevice.charioread;
	next->chariowrite=chardevice.chariowrite;
	next->ioctl=chardevice.ioctl;
	next->flags=FILE_CHARACTER_DEVICE;
	next->currentpos=0;
	next->access=access;
	next->owner_process=getpid();		/* owner process */
	next->handle=++highest_handle;
	next->currentblock=0;
	next->next=NULL;

	openfiles_last=next;				/* save pointer to end */

	unlock_mutex(&vfs_mutex);

	setlasterror(NO_ERROR);
	return(next->handle);					/* return handle */
}

/* If opening regular file */

getfullpath(filename,fullname);				/* get full path of file */

if(access & O_TRUNC) {			/* truncate file */
	if(unlink(fullname) == -1) return(-1);	/* delete it */
	if(create(fullname) == -1) return(-1);	/* recreate it */
}

if(findfirst(fullname,&dirent) == -1) {			/* check if file exists */
	if(access & O_CREAT) {		/* if O_CREAT is set, create file if it does not exist */
		if(create(fullname) == -1) return(-1);

		unlock_mutex(&vfs_mutex);

		setlasterror(NO_ERROR);
		return(NO_ERROR);
	}

	unlock_mutex(&vfs_mutex);
	return(-1);
}

/* check if file can be opened */

if((dirent.flags & FILE_DIRECTORY) == FILE_DIRECTORY) {
	unlock_mutex(&vfs_mutex);

	setlasterror(ACCESS_DENIED);			/* can't open directory */
	return(-1);
}

if(openfiles == NULL) {
	openfiles=kernelalloc(sizeof(FILERECORD));					/* add new entry */
	next=openfiles;
}
else
{
	last->next=kernelalloc(sizeof(FILERECORD));					/* add new entry */
	next=last->next;
}

getfullpath(filename,fullname);

memset(next,0,sizeof(FILERECORD));
memcpy(next,&dirent,sizeof(FILERECORD));  /* copy data */

strncpy(next->filename,fullname,MAX_PATH);	/* copy full filename */

next->currentpos=0;			/* set position to start of file */
next->access=access;			/* access mode */
next->flags=FILE_REGULAR;		/* file information flags */
next->handle=++highest_handle;		/* file handle */
next->owner_process=getpid();		/* owner process */
next->currentblock=next->startblock;
next->next=NULL;
	
openfiles_last=next;				/* save pointer to end */

setlasterror(NO_ERROR);
unlock_mutex(&vfs_mutex);

return(next->handle);					/* return handle */
} 

/*
 * Delete file
 *
 * In:  name	File to delete
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t unlink(char *name) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];

getfullpath(name,fullname);

splitname(name,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) return(-1);	/* detect filesystem */

getblockdevice(splitbuf.drive,&blockdevice);

if(update_block_device(splitbuf.drive,&blockdevice) == -1) return(-1);		/* get block device */

if(fs.unlink == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.unlink(name));
}

/*
 * Rename file
 *
 * In:  olname	File to rename
	newname	New filename
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t rename(char *olname,char *newname) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];

getfullpath(olname,fullname);

splitname(olname,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) return(-1);	/* detect filesystem */

getblockdevice(splitbuf.drive,&blockdevice);

if(update_block_device(splitbuf.drive,&blockdevice) == -1) return(-1);		/* get block device */

if(fs.rename == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.rename(olname,newname));
}

/*
 * Create file
 *
 * In:  name	File to create
 *
 * Returns: -1 on error, file handle on success
 *
 */
size_t create(char *name) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];

getfullpath(name,fullname);

splitname(name,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) return(-1);	/* detect filesystem */

getblockdevice(splitbuf.drive,&blockdevice);
if(update_block_device(splitbuf.drive,&blockdevice) == -1) return(-1);		/* get block device */

if(fs.create == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

if(fs.create(name) == -1) return(-1);

return(open(name,O_RDWR));
}

/*
 * Remove directory
 *
 * In:  name	Directory to remove
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t rmdir(char *name) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];
FILERECORD dirent;

getfullpath(name,fullname);

splitname(name,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) return(-1);	/* detect filesystem */

getblockdevice(splitbuf.drive,&blockdevice);
if(update_block_device(splitbuf.drive,&blockdevice) == -1) return(-1);		/* get block device */

if(fs.rmdir == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.rmdir(name));
}

/*
 * Create directory
 *
 * In:  name	Directory to create
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t mkdir(char *name) {
FILESYSTEM fs;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];

getfullpath(name,fullname);

splitname(name,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) return(-1);	/* detect filesystem */

if(fs.mkdir == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.mkdir(name));
}

/*
 * Set file attributes
 *
 * In:  name	File to change attributes
	attribs Attributes. The specific attributes depend on the filesystem
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t chmod(char *name,size_t attribs) {
FILESYSTEM fs;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];

getfullpath(name,fullname);

splitname(name,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) return(-1);	/* detect filesystem */

if(fs.chmod == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.chmod(name,attribs));
}

/*
 * Read from file or device
 *
 * In:  handle	File handle
	addr	Buffer to read to
	size	Number of bytes to read
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t read(size_t handle,void *addr,size_t size) {
FILESYSTEM fs;
FILERECORD *next;
SPLITBUF splitbuf;
BLOCKDEVICE *blockdevice;

lock_mutex(&vfs_mutex);

next=openfiles;
while(next != NULL) {
	if((next->handle == handle) && (next->owner_process == getpid())) break;

	next=next->next;
}

if(next == NULL) {
	unlock_mutex(&vfs_mutex);

	setlasterror(INVALID_HANDLE);
	return(-1);
}

if((next->access & O_RDONLY) == 0) {		/* not opened for read access */
	setlasterror(ACCESS_DENIED);
	return(-1);
}

/*
 * if reading from character device, call device handler and return
 */

if((next->flags & FILE_CHARACTER_DEVICE)) {		/* character device I/O */

	if(next->charioread(addr,size) == -1) {
		unlock_mutex(&vfs_mutex);
		return(-1);
	}

	unlock_mutex(&vfs_mutex);

	setlasterror(NO_ERROR);
	return(NO_ERROR);
}

/*
 * if reading from block device, call device handler and return
 */

if((next->flags & FILE_BLOCK_DEVICE)) {		/* block device I/O */			
	if(next->blockdevice.blockio(DEVICE_READ,next->drive,(next->currentblock/(next->blockdevice.sectorsize*next->blockdevice.sectorsperblock)),addr) == -1) {
		unlock_mutex(&vfs_mutex);
		return(-1);
	}

	next->currentblock++;			/* point to next block */
	next->currentpos += (next->blockdevice.sectorsize*next->blockdevice.sectorsperblock);	/* update current position */

	unlock_mutex(&vfs_mutex);

	setlasterror(NO_ERROR);
	return(NO_ERROR);
}

/* 
 * if reading from a pipe
 *
 */

if((next->flags & FILE_FIFO)) return(readpipe(next,addr,size));

/* reading from a regular file */

splitname(next->filename,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) return(-1);	/* detect filesystem */

if(fs.read == NULL) {			/* not implemented */
	unlock_mutex(&vfs_mutex);

	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

unlock_mutex(&vfs_mutex);

return(fs.read(handle,addr,size));
}

/*
 * Write to file or device
 *
 * In:  handle	File handle
	addr	Buffer to write from
	size	Number of bytes to write
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t write(size_t handle,void *addr,size_t size) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
FILERECORD *next;
SPLITBUF splitbuf;
char *newbuf;

lock_mutex(&vfs_mutex);

next=openfiles;
while(next != NULL) {
	if((next->handle == handle) && (next->owner_process == getpid())) break;
	next=next->next;
}

if(next == NULL) {
	unlock_mutex(&vfs_mutex);

	setlasterror(INVALID_HANDLE);
	return(-1);
}

if((next->flags & FILE_CHARACTER_DEVICE)) {		/* device i/o */		
	if(next->chariowrite(addr,size) == -1) {
		unlock_mutex(&vfs_mutex);

		return(-1);
	}

	unlock_mutex(&vfs_mutex);

	setlasterror(NO_ERROR);
	return(NO_ERROR);
}

/*
 * If writing to block device, call device handler and return
 */

if((next->flags == FILE_BLOCK_DEVICE)) {

	if(next->blockdevice.blockio(DEVICE_WRITE,next->drive,(next->currentblock/(next->blockdevice.sectorsize*next->blockdevice.sectorsperblock)),addr) == -1) {
		unlock_mutex(&vfs_mutex);
		return(-1);
	}

	next->currentblock++;			/* point to next block */
	next->currentpos += (next->blockdevice.sectorsize*next->blockdevice.sectorsperblock);	/* update current position */

	unlock_mutex(&vfs_mutex);

	setlasterror(NO_ERROR);
	return(NO_ERROR);
}

/* 
 * Writing to a pipe
 *
 */

if((next->flags & FILE_FIFO)) {			/* pipe I/O */
	return(writepipe(next,addr,size));
}

/* Writing to a regular file */

if((next->access & O_WRONLY) == 0) {		/* not opened for write access */
	setlasterror(ACCESS_DENIED);
	return(-1);
}

splitname(next->filename,&splitbuf);				/* split name */

unlock_mutex(&vfs_mutex);
if(detect_filesystem(splitbuf.drive,&fs) == -1) return(-1);	/* detect filesystem */

if(fs.write == NULL) {			/* not implemented */
	unlock_mutex(&vfs_mutex);

	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

unlock_mutex(&vfs_mutex);

return(fs.write(handle,addr,size));
}


/*
 * Ioctl - device specific I/O operations
 *
 * In:  handle	File handle
	request Request number - device specific
	buffer	Buffer
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t ioctl(size_t handle,unsigned long request,void *buffer) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
FILERECORD *next;
SPLITBUF splitbuf;

lock_mutex(&vfs_mutex);

next=openfiles;
while(next != NULL) {
	if(next->handle == handle) break;

	next=next->next;
}

if(next == NULL) {
	unlock_mutex(&vfs_mutex);

	setlasterror(INVALID_HANDLE);
	return(-1);
}

if(((next->flags & FILE_CHARACTER_DEVICE) == 0) || ((next->flags & FILE_BLOCK_DEVICE) == 0)) {	/* not device */
	setlasterror(NOT_DEVICE);
	return(-1);
}

if(next->ioctl != NULL) {
	if(next->ioctl(handle,request,buffer) == -1) {
		lock_mutex(&vfs_mutex);
		return(-1);
	}

	unlock_mutex(&vfs_mutex);
	setlasterror(NO_ERROR);

	return(NO_ERROR);
}
else
{
	unlock_mutex(&vfs_mutex);

	setlasterror(INVALID_DEVICE);
	return(-1);
}

return(NO_ERROR);
}

/*
 * Close file
 *
 * In:  handle	File handle
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t close(size_t handle) { 
size_t count;
FILERECORD *last;
FILERECORD *next;

last=next;
	
lock_mutex(&vfs_mutex);

next=openfiles;

while(next != NULL) {					/* find filename in struct */
	if(next->handle == handle) {

		if(next->owner_process != getpid()) {		/* if file is owned by process */
			unlock_mutex(&vfs_mutex);

			setlasterror(ACCESS_DENIED);
			return(-1);
		}

		break;
	}

	last=next;
	next=next->next;
}

if(next == NULL) {			/* invalid handle */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

if((next->flags & FILE_FIFO)) closepipe(next);		/* if closing pipe */

if(next == openfiles) {			/* first entry */
	last=openfiles;			/* save pointer to start */

	openfiles=last->next;		/* set new beginning of list */
	
	if(last != NULL) kernelfree(last);		/* free old beginning */

	if(last->next == NULL) openfiles_last=next;		/* save last */
}
else if(next->next == NULL) {		/* last entry */	
	kernelfree(last->next);		/* free last */
	last->next=NULL;		/* remove from end of list */

	openfiles_last=next;		/* save last */
}
else
{
	last->next=next->next;		/* remove from list */

	if(next->next == NULL) openfiles_last=next;		/* save last */

	kernelfree(next);
}

unlock_mutex(&vfs_mutex);

setlasterror(NO_ERROR);  
return(NO_ERROR);
}

/*
 * Close all open files for process
 *
 * In:  nothing
 *
 * Returns: nothing
 *
 */

void shut(void) { 
FILERECORD *last;
FILERECORD *next;
	
lock_mutex(&vfs_mutex);

next=openfiles;

while(next != NULL) {

	if(next->owner_process == getpid()) {		/* if file is owned by process */
		last->next=next->next;			/* remove it from then list */
		kernelfree(next);			/* free list entry */
	}

	next=next->next;
}

unlock_mutex(&vfs_mutex);

setlasterror(INVALID_HANDLE);
return(-1);
}

/*
 * Duplicate handle for process
 *
 * In:  handle		File handle
 *	sourcepid	Process ID to create new file handle from
 *	destpid		Process ID to assign to new handle
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t dup_internal(size_t handle,size_t desthandle,size_t sourcepid,size_t destpid) {
FILERECORD *source=NULL;
FILERECORD *dest=NULL;
FILERECORD *next;
FILERECORD *saveend;
size_t count=0;

lock_mutex(&vfs_mutex);

/* find source and destination handles */
next=openfiles;
	
while(next != NULL) {					/* find handle in list */
	if((next->handle == handle) && (next->owner_process == sourcepid)) source=next;
	if((next->handle == desthandle) && (next->owner_process == destpid)) dest=next;		/* if desthandle == -1, it will not match any */
												/* in the list */
	next=next->next;
	count++;
}

if(source == NULL) {
	unlock_mutex(&vfs_mutex);

	setlasterror(INVALID_HANDLE);
	return(-1);
}

if(desthandle == -1) {				/* if desthandle == -1, add to end */
	openfiles_last->next=kernelalloc(sizeof(FILERECORD));					/* add new entry */

	if(openfiles_last->next == NULL) {
		unlock_mutex(&vfs_mutex);
		setlasterror(NO_MEM);				/* out of memory */
		return(-1);
	}

	openfiles_last=openfiles_last->next;		/* point to new end */

	dest=openfiles_last;
}

memcpy(dest,source,sizeof(FILERECORD));		/* copy file handle */

dest->owner_process=destpid;
dest->next=NULL;

//if(desthandle == -1) dest->handle=++highest_handle;		/* set handle of destination */

unlock_mutex(&vfs_mutex);

setlasterror(NO_ERROR);
return(count);
}

/*
 * Duplicate handle
 *
 * In:  handle	File handle
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t dup(size_t handle) {
dup_internal(handle,-1,getpid(),getpid());
}

/*
 * Duplicate to existing handle
 *
 * In:  oldhandle	Existing file handle
	newhandle	Destination file handle
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t dup2(size_t oldhandle,size_t newhandle) {
return(dup_internal(oldhandle,newhandle,getpid(),getpid()));
}

/*
 * Register filesystem
 *
 * In:  newfs	struct of filesystem information
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t register_filesystem(FILESYSTEM *newfs) {
FILESYSTEM *next;
FILESYSTEM *last;

lock_mutex(&vfs_mutex);

if(filesystems == NULL) {

	filesystems=kernelalloc(sizeof(FILESYSTEM));		/* add to end */
	if(filesystems == NULL) return(-1);

	next=filesystems;

}
else
{
	next=filesystems;

	while(next != NULL) {

		if(strncmp(newfs->name,next->name,MAX_PATH) == 0) {		/* already loaded */
			unlock_mutex(&vfs_mutex);
			setlasterror(MODULE_ALREADY_LOADED);
			return(-1);
	}

	last=next;
	next=next->next;
}

last->next=kernelalloc(sizeof(FILESYSTEM));		/* add to end */

if(last->next == NULL) {
	lock_mutex(&vfs_mutex);
	return(-1);
}

next=last->next;
}

memcpy(next,newfs,sizeof(FILESYSTEM));			/* copy filesystem data */

next->next=NULL;

unlock_mutex(&vfs_mutex);
setlasterror(NO_ERROR);
return(0);
}

/*
 * Detect filesystem
 *
 * In:  drive	Drive
	buf	Buffer to fill with information about filesystem
 *
 * 
 * Returns: -1 on error, 0 on success
 *
 */

size_t detect_filesystem(size_t drive,FILESYSTEM *buf) {
void *blockbuf;
FILESYSTEM *next;
void *blockptr;
size_t count;

lock_mutex(&vfs_mutex);

blockbuf=kernelalloc(VFS_MAX);
if(blockbuf == NULL) return(-1);


if(blockio(0,drive,(uint64_t) 0,blockbuf) == -1) return(-1);		/* read block */

/* check magic number */

next=filesystems;

while(next != NULL) {
	for(count=0;count<next->magic_count;count++) {
		blockptr=blockbuf;			/* point to magic data location in superblock */
		blockptr += next->magicbytes[count].location;

		if(memcmp(blockptr,&next->magicbytes[count].magicnumber,next->magicbytes[count].size) == 0) {	/* if magic number matches */
			memcpy(buf,next,sizeof(FILESYSTEM));

			kernelfree(blockbuf);
			unlock_mutex(&vfs_mutex);

			setlasterror(NO_ERROR);
			return(0);
		}
	}

	next=next->next;
}

unlock_mutex(&vfs_mutex);

setlasterror(UNKNOWN_FILESYSTEM);
return(-1);
}

/*
 * Get full file path from partial path
 *
 * In:  filename	Partial path to filename
	buf		Buffer to fill with full path to filename
 *
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t getfullpath(char *filename,char *buf) {
char *token[10][MAX_PATH];
char *dottoken[10][MAX_PATH];
int tc=0;
int dottc=0;
int count;
int countx;
char *b;
char c,d,e;
char *cwd[MAX_PATH];
BOOT_INFO *boot_info=BOOT_INFO_ADDRESS+KERNEL_HIGH;

if(filename == NULL || buf == NULL) return(-1);

getcwd(cwd);

memset(buf,0,MAX_PATH);

b=filename;

c=*b++;
d=*b++;
e=*b++;

if((d == ':') && (e == '\\')) {               /* full path */
	strncpy(buf,filename,MAX_PATH);                     /* copy path */
}
else if((d == ':') && (e != '\\')) {		/* drive and filename only */
	memcpy(buf,filename,2); 		/* get drive */
	strncat(buf,filename+3,MAX_PATH);                 /* get filename */
}
else if(d != ':') {               /* filename only */
	if(c == '\\') {		/* path without drive */
		memcpy(buf,cwd,2);	/* copy drive letter and : */
		strncat(buf,filename,MAX_PATH);	/* copy filename */
	}
	else
	{
		b=cwd;
		b += strlen(cwd)-1;		/* point to end */

		if(*b == '\\') {		/* trailing \ */
			ksnprintf(buf,"%s%s",MAX_PATH,cwd,filename);	/* copy directory\filename */
		}
		else
		{
			ksnprintf(buf,"%s\\%s",MAX_PATH,cwd,filename);	/* copy directory\filename */
		}

	}
}

tc=tokenize_line(cwd,token,"\\");		/* tokenize line */
dottc=tokenize_line(filename,dottoken,"\\");		/* tokenize line */


for(countx=0;countx<dottc-1;countx++) {
	if(strncmp(dottoken[countx],"..",MAX_PATH) == 0 ||  strncmp(dottoken[countx],".",MAX_PATH) == 0) {

		for(count=0;count<dottc;count++) {

			if(strncmp(dottoken[count],"..",MAX_PATH) == 0 || strncmp(dottoken[countx],".",MAX_PATH) == 0) {
				tc--;
			}
			else
			{
				strncpy(token[tc],dottoken[count],MAX_PATH);
					tc++;
			}
		}


		strncpy(buf,token[0],MAX_PATH);
		strncat(buf,"\\",MAX_PATH);

		for(count=1;count<tc;count++) {
			strncat(buf,token[count],MAX_PATH);

			if(count != tc-1) strncat(buf,"\\",MAX_PATH);
		}

	}

	break;
} 

return(NO_ERROR);
}

/*
 * Initialize file manager
 *
 * In:  nothing
 *
 * Returns: nothing
 *
 */


size_t filemanager_init(void) {
FILERECORD *next;

filesystems=NULL;						/* no filesystem drivers for now */

/* Create initial stdin, stdout and stderr handles */

openfiles=kernelalloc(sizeof(FILERECORD));			/* stdin */
if(openfiles == NULL) {						/*can't allocate */
	kprintf_direct("kernel: can't allocate memory for stdin\n");
	return(-1);
}

strncpy(openfiles->filename,"stdin",MAX_PATH);
openfiles->handle=0;
openfiles->charioread=NULL;			/* for now */
openfiles->chariowrite=NULL;
openfiles->flags=FILE_CHARACTER_DEVICE;
openfiles->access=O_RDONLY;

openfiles->next=kernelalloc(sizeof(FILERECORD));			/* stdin */
if(openfiles->next == NULL) {	/*can't allocate */
	kprintf_direct("kernel: can't allocate memory for stdout\n");
	return(-1);
}

next=openfiles->next;

strncpy(next->filename,"stdout",MAX_PATH);
next->handle=1;
next->charioread=NULL;			/* for now */
next->chariowrite=NULL;
next->flags=FILE_CHARACTER_DEVICE;
next->access=O_WRONLY;

next->next=kernelalloc(sizeof(FILERECORD));			/* stdin */
if(next->next == NULL) {	/*can't allocate */
	kprintf_direct("kernel: can't allocate memory for stderr\n");
	return(-1);
}

next=next->next;

strncpy(next->filename,"stderr",MAX_PATH);
next->handle=2;
next->charioread=NULL;			/* for now */
next->chariowrite=NULL;
next->flags=FILE_CHARACTER_DEVICE;
next->access=O_WRONLY;
next->next=NULL;

openfiles_last=next;			/* save last */
highest_handle=2;			/* save highest handle number */

return;
}

/*
 * Initialize console
 *
 * In:  handle	handle of console device (0,1 or 2)
	cptr	Pointer to console I/O function	
 *
 * Returns: -1 on error, 0 on success
 *
 */	

size_t init_console_device(size_t type,size_t handle,void *cptr) {
FILERECORD *next;
	
next=openfiles;

while(next != NULL) {

	if(next->handle == handle) {

		if(type == DEVICE_READ) {
			next->charioread=cptr;
		}
		else if(type == DEVICE_WRITE) {
			next->chariowrite=cptr;
		}

		return(0);
	}

	next=next->next;
}

initialize_mutex(&vfs_mutex);		/* intialize mutex */

return(-1);
}

/*
 * Get file position
 *
 * In:  handle	File handle
 *
 * Returns: -1 on error, file position on success
 *
 */

size_t tell(size_t handle) {
FILERECORD *next;
size_t count=0;

unlock_mutex(&vfs_mutex);

next=openfiles;

while(next != NULL) {					/* find filename in struct */
	if(next->handle == handle) {
		lock_mutex(&vfs_mutex);
		break;
	}

	next=next->next;
	count++;
}

if(next == NULL) {
	unlock_mutex(&vfs_mutex);

	setlasterror(INVALID_HANDLE);			/* bad handle */
	return(-1);
}

	unlock_mutex(&vfs_mutex);

	return(next->currentpos);				/* return position */
}

/*
 * Set file time and date
 *
 * In:  filename	File to set file and date of
	createtime	struct with create time and date information
		  lastmodtime	struct with last modified time and date information
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t touch(char *filename,TIME *create_time_date,TIME *last_modified_time_date,TIME *last_accessed_time_date) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;

splitname(filename,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	setlasterror(GENERAL_FAILURE);
	return(-1);
}


return(fs.touch(filename,create_time_date,last_modified_time_date,last_accessed_time_date));
}

/*
 * Get file information from handle
 *
 * In:  handle	File handle
	buf	struct to hold information about file
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t gethandle(size_t handle,FILERECORD *buf) {
FILERECORD *next;

lock_mutex(&vfs_mutex);

next=openfiles;

while(next != NULL) {
	if((next->handle == handle) && (next->owner_process == getpid())) {		/* found handle */
		unlock_mutex(&vfs_mutex);

		memcpy(buf,next,sizeof(FILERECORD));		/* copy data */

		return(0);
	}


	next=next->next;
}

unlock_mutex(&vfs_mutex);

setlasterror(INVALID_HANDLE);
return(-1);
}

/*
 * Get file size
 *
 * In:  handle	File handle
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t getfilesize(size_t handle) {
FILERECORD *next;

lock_mutex(&vfs_mutex);

next=openfiles;

while(next != NULL) {
	if((next->handle == handle) && (next->owner_process == getpid())) {
		unlock_mutex(&vfs_mutex);
		return(next->filesize);
	}

	next=next->next;
}

unlock_mutex(&vfs_mutex); 
return(-1);
}

/*
 * Split filename into drive, directory and filename
 *
 * In:  name	Filename
	splitbuf	struct to hold information about drive, directory and filename
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t splitname(char *name,SPLITBUF *splitbuf) {
size_t count;
char *buf[MAX_PATH];
char *d;
char *b;
char *s;
char *x;
uint8_t drive;

memset(splitbuf,0,sizeof(SPLITBUF));

getfullpath(name,buf);			/* get full path */

count=0;
b=buf+strlen(buf);

while(*b-- != '\\') count++;

memcpy(splitbuf->filename,b+2,count);		/* get file name */

d=splitbuf->dirname;
*d++='\\';

x=buf;
x=x+3;

if(*b != ':') {		/* directory */
	b=b+2;

	while(x != b-1) *d++=*x++;
}

b=buf;
drive=(uint8_t) *b;

splitbuf->drive=drive-'A';

return;
} 
	

/*
 * Update file information using handle
 *
 * In:  handle	File handle
	buf	Struct holding information about the file
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t updatehandle(size_t handle,FILERECORD *buf) {
FILERECORD *next;

lock_mutex(&vfs_mutex);

next=openfiles;

while(next != NULL) {
/* update own handle */

	if((next->handle == handle) && (next->owner_process == getpid())) {		/* found handle */

		memcpy(next,buf,sizeof(FILERECORD));		/* copy data */

		unlock_mutex(&vfs_mutex);

		setlasterror(NO_ERROR);
		return(0);
	}

	next=next->next;
}

unlock_mutex(&vfs_mutex);

setlasterror(INVALID_HANDLE);
return(-1);
}

/*
 * Set file position
 *
 * In:  handle	File handle
	pos	Byte to seek to
	whence	Where to seek from(0=start,1=current position,2=end)
 *
 * Returns: -1 on error, new file position on success
 *
 */
size_t seek(size_t handle,size_t pos,size_t whence) {
FILERECORD seek_file_record;

if(gethandle(handle,&seek_file_record) == -1) {			/* bad handle */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

if(seek_file_record.flags & FILE_FIFO) {	/* not file or block device */
	setlasterror(ACCESS_ERROR);
	return(-1);
}

if((whence == SEEK_SET) || (whence == SEEK_CUR)) {		/* check new file position */
	if((seek_file_record.currentpos+pos) > seek_file_record.filesize) {	/* invalid position */
		setlasterror(SEEK_ERROR);
		return(-1);
	}
}

switch(whence) {
	case SEEK_SET:				/*set current position */
		seek_file_record.currentpos=pos;
		break;

	case SEEK_END:
		seek_file_record.currentpos=seek_file_record.filesize+pos;	/* end+pos */
		break;

	case SEEK_CUR:				/* current location+pos */
		seek_file_record.currentpos=seek_file_record.currentpos+pos;
		break;
}

seek_file_record.flags |= FILE_POS_MOVED_BY_SEEK;

updatehandle(handle,&seek_file_record);		/* update */

setlasterror(NO_ERROR);
return(seek_file_record.currentpos);
}

/*
 * Create pipe
 *
 * In:  Nothing
 *
 * Returns: -1 on error, file handle on success.
 *
 */
size_t pipe(void) {
lock_mutex(&vfs_mutex);

openfiles_last->next=kernelalloc(sizeof(FILERECORD));	/* add file descriptor */
if(openfiles_last->next == NULL) return(-1);

openfiles_last=openfiles_last->next;

openfiles_last->flags=FILE_FIFO;
openfiles_last->handle=++highest_handle;
	
openfiles_last->pipe=NULL;			/* empty, for now */
openfiles_last->pipereadprevious=NULL;
openfiles_last->pipelast=NULL;
openfiles_last->pipereadptr=NULL;
openfiles_last->access=O_RDWR;			/* open pipe for reading and writing */

unlock_mutex(&vfs_mutex);

return(openfiles_last->handle);
}

/*
 * Write to pipe (internal function)
 *
 * In:  entry	Pointer to open file descriptor
 *	addr	Pointer to buffer to write data from
 *	size	Number of bytes to write
 *
 * Returns: -1 on error,number of bytes written on success.
 *
 */
size_t writepipe(FILERECORD *entry,void *addr,size_t size) {

if(entry->pipe == NULL) {		/* no pipe */
	entry->pipe=kernelalloc(sizeof(PIPE));	
	if(entry->pipe == NULL) return(-1);
	
	entry->pipelast=entry->pipe;
	entry->pipereadprevious=entry->pipe;
	entry->pipereadptr=entry->pipe;

	initialize_mutex(&entry->pipe->mutex);	/* create mutex */
	lock_mutex(&entry->pipe->mutex);		/* lock mutex */
}
else					/* add to end */
{
	lock_mutex(&entry->pipe->mutex);		/* lock mutex */

	entry->pipereadprevious=entry->pipelast;

	entry->pipelast->next=kernelalloc(sizeof(PIPE));	
	if(entry->pipelast->next == NULL) return(-1);

	entry->pipelast=entry->pipelast->next;
}

entry->pipelast->buffer=kernelalloc(size);	/* allocate pipe buffer */
if(entry->pipelast->buffer == NULL) return(-1);

entry->pipelast->bufptr=entry->pipelast->buffer;
entry->pipelast->size=size;

memcpy(entry->pipelast->buffer,addr,size);		/* copy data */

if(entry->pipereadptr == NULL) entry->pipereadptr=entry->pipe; /* if writing after emptying pipe */

unlock_mutex(&entry->pipe->mutex);		/* unlock mutex */

return(size);
}

/*
 * Read from pipe (internal function)
 *
 * In:  entry	Pointer to open file descriptor
 *	addr	Pointer to buffer to read data to
 *	size	Number of bytes to read
 *
 * Returns: -1 on error, number of bytes read on success.
 *
 */
size_t readpipe(FILERECORD *entry,char *addr,size_t size) {
size_t readsize=0;
char *addrptr;
int numberofbytes=0;
PIPE *saveentry;
size_t atend=FALSE;
size_t bytesread=0;

lock_mutex(&entry->pipe->mutex);		/* lock mutex */

if(entry->pipe == NULL) {		/* no data in pipe */
	setlasterror(INPUT_PAST_END);

	unlock_mutex(&entry->pipe->mutex);		/* unlock mutex */
	return(-1);
}

/* read data from entries */

addrptr=addr;

do {
	if(size < entry->pipereadptr->size) {		/* normalize */
		numberofbytes=size;
	}
	else
	{
		numberofbytes=entry->pipereadptr->size;
	}

	memcpy(addr,entry->pipereadptr->bufptr,numberofbytes);	/* copy data */

	entry->pipereadptr->size -= numberofbytes;		/* decrease size */
	entry->pipereadptr->bufptr += numberofbytes;		/* point to next in buffer */	

	bytesread += numberofbytes;

	if(entry->pipereadptr->size <= 0) {			/* if entry used up, remove it */

		if(entry == entry->pipe) {			/* first entry */
			saveentry=entry->pipe;			/* save pointer to start */

			entry->pipe=entry->pipe->next;		/* set new beginning of list */
			kernelfree(saveentry);		/* free old beginning */
		}
		else if(entry->pipereadptr->next == NULL) {
			atend=TRUE;		/* at end */
			
			saveentry=entry->pipereadprevious->next;

			kernelfree(entry->pipereadprevious);		/* free last */
			entry->pipereadprevious->next=NULL;		/* remove from end of list */
		}
		else
		{
			entry->pipereadprevious->next=entry->pipereadptr->next;		/* remove from list */
			kernelfree(entry->pipereadptr);
		}
		
		if(atend == TRUE) {		/* no more data to read */
			unlock_mutex(&entry->pipe->mutex);		/* unlock mutex */

			setlasterror(END_OF_FILE);
			return(0);
		}

		entry->pipereadptr=entry->pipereadptr->next;
	}

	size -= numberofbytes;
	
} while(readsize < size);

unlock_mutex(&entry->pipe->mutex);		/* unlock mutex */
return(bytesread);
}

/*
 * Close pipe (internal function)
 *
 * In:  entry	Pointer to open file descriptor
 *
 * Returns: Nothing
 *
 */
void closepipe(FILERECORD *entry) {
PIPE *next;
PIPE *savepointer;

lock_mutex(&entry->pipe->mutex);		/* lock mutex */

next=entry->pipe;			/* point to first entry */

/* loop through entries, freeing buffers and removing pipe entries */

while(next != NULL) {
	if(next->buffer != NULL) kernelfree(next->buffer);	/* free buffer */

	savepointer=next->next;		/* save next pointer */

	kernelfree(next);		/* free entry */

	if(savepointer == NULL) break;

	next=savepointer->next;
}

entry->pipe=NULL;
entry->pipelast=NULL;
entry->pipereadprevious=NULL;
entry->pipereadptr=NULL;

return;
}

