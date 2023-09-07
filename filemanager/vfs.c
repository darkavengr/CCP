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
#include "../header/kernelhigh.h"
#include "../header/errors.h"
#include "vfs.h"
#include "../processmanager/mutex.h"
#include "../devicemanager/device.h"
#include "../header/bootinfo.h"

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
size_t setfiletimedate(char *filename,TIMEBUF *createtime,TIMEBUF *lastmodtime);
size_t getstartblock(char *name);
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

FILERECORD *openfiles=NULL;
FILESYSTEM *filesystems=NULL;
MUTEX vfs_mutex;

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

if(detect_filesystem(splitbuf.drive,&fs) == -1) {		/* detect file system */
	setlasterror(UNKNOWN_FILESYSTEM);
	return(-1);
}

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

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	setlasterror(GENERAL_FAILURE);
	return(-1);
}

if(fs.findnext == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.findnext(name,buf));		/* Call via function pointer */
}


/*
 * Open file
 *
 * In: name	File to open
		 access	File mode(O_RD_RW, O_RDONLY)
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

next=openfiles;
handle=0;

while(next != NULL) {					/* return error if open */
	if(strcmp(filename,next->filename) == 0) {	/* found filename */
	
	if(next->owner_process == getpid()) {
		unlock_mutex(&vfs_mutex);
		return(next->handle);	/* opening by owner process */
	}

	if(next->access & FILE_ACCESS_EXCLUSIVE) {		/* reopening file with exclusive access */
		unlock_mutex(&vfs_mutex);

		setlasterror(ACCESS_DENIED);
		return(-1);
	}
}

handle++;

next=next->next;
}

/* file being opened by another process - either first time or again by another process */

handle++;

if(getdevicebyname(filename,&blockdevice) == 0) {		/* get device info */

next=openfiles;

while(next != NULL) {
		last=next;
		next=next->next;		/* find second to last entry */
}

last->next=kernelalloc(sizeof(FILERECORD));			/* allocate */
next=last->next;

if(next == NULL) {
	lock_mutex(&vfs_mutex);
	return(-1);
}

strcpy(next->filename,blockdevice.dname);

next->findblock=0;
next->currentblock=getstartblock(next->filename);
next->currentpos=0;
next->owner_process=getpid();
next->blockio=blockdevice.blockio;
next->flags=FILE_BLOCK_DEVICE;
next->handle=handle;
next->access=access;
next->next=NULL;

unlock_mutex(&vfs_mutex);

setlasterror(NO_ERROR);
return(handle);					/* return handle */
}

/* If opening character device */

if(findcharacterdevice(filename,&chardevice) == 0) {
	next=openfiles;
	last=openfiles;

	while(next != NULL) {
		last=next;
		next=next->next;
	}

	last->next=kernelalloc(sizeof(FILERECORD));			/* allocate */
	next=last->next;
	
	if(next == NULL) {
		lock_mutex(&vfs_mutex);
		return(-1);
	}

	strcpy(next->filename,chardevice.dname);

	next->charioread=chardevice.charioread;
	next->chariowrite=chardevice.chariowrite;
	next->ioctl=chardevice.ioctl;
	next->flags=FILE_CHAR_DEVICE;
	next->currentpos=0;
	next->access=access;		/* access */
	next->handle=handle;
	next->next=NULL;

	unlock_mutex(&vfs_mutex);

	setlasterror(NO_ERROR);
	return(handle);					/* return handle */
}

getfullpath(filename,fullname);

if(findfirst(fullname,&dirent) == -1) {
	unlock_mutex(&vfs_mutex);
	return(-1);
}

/* check if file can be opened */

if((dirent.attribs & _DIR) == _DIR) {
	unlock_mutex(&vfs_mutex);

	setlasterror(ACCESS_DENIED);			/* can't open directory */
	return(-1);
}

if(openfiles == NULL) {					/* first in chain */
	handle=3;
	openfiles=kernelalloc(sizeof(FILERECORD));					/* add new entry */

	if(openfiles == NULL) {
		unlock_mutex(&vfs_mutex);
		setlasterror(NO_MEM);				/* out of memory */
		return(-1);
	}

		next=openfiles;
	}
	else
	{
		next=openfiles;
		last=next;
		handle=3;

		while(next != NULL) {					/* FIND END */
			last=next;
			next=next->next;
			handle++;
	}

		handle++;						/* new handle */	


		last->next=kernelalloc(sizeof(FILERECORD));					/* add new entry */
		next=last->next;

		if(next == NULL) {
			unlock_mutex(&vfs_mutex);

			setlasterror(NO_MEM);				/* out of memory */
			return(-1);
		}
	}

getfullpath(filename,fullname);
	
memset(next,0,sizeof(FILERECORD));
memcpy(next,&dirent,sizeof(FILERECORD));  /* copy data */

// strcpy(next->filename,fullname);	/* copy full filename */

next->currentpos=0;			/* set position to start of file */
next->currentblock=getstartblock(fullname);		/* get start block */
next->access=access;			/* access mode */
next->flags=FILE_REGULAR;		/* file information flags */
next->handle=handle;			/* file handle */
next->owner_process=getpid();		/* owner process */
next->next=NULL;
	
setlasterror(NO_ERROR);
unlock_mutex(&vfs_mutex);

return(handle);					/* return handle */
} 

/*
 * Delete file
 *
 * In:  name	File to delete
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t delete(char *name) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];

getfullpath(name,fullname);

splitname(name,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	setlasterror(GENERAL_FAILURE);
	return(-1);
}

getblockdevice(splitbuf.drive,&blockdevice);

if(update_block_device(splitbuf.drive,&blockdevice) == -1) return(-1);		/* get block device */

if(fs.delete == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.delete(name));
}

/*
 * Rename file
 *
 * In:  oldname	File to rename
	newname	New filename
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t rename(char *oldname,char *newname) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];

getfullpath(oldname,fullname);

splitname(oldname,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	setlasterror(GENERAL_FAILURE);
	return(-1);
}

getblockdevice(splitbuf.drive,&blockdevice);

if(update_block_device(splitbuf.drive,&blockdevice) == -1) return(-1);		/* get block device */

if(fs.rename == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.rename(oldname,newname));
}

/*
 * Create file
 *
 * In:  name	File to create
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t create(char *name) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;
char *fullname[MAX_PATH];

getfullpath(name,fullname);

splitname(name,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	setlasterror(GENERAL_FAILURE);
	return(-1);
}

getblockdevice(splitbuf.drive,&blockdevice);
if(update_block_device(splitbuf.drive,&blockdevice) == -1) return(-1);		/* get block device */

if(fs.create == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

if(fs.create(name) == -1) return(-1);

return(open(name,_O_RDWR));
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

getfullpath(name,fullname);

splitname(name,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	setlasterror(GENERAL_FAILURE);
	return(-1);
}

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

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	setlasterror(GENERAL_FAILURE);
	return(-1);
}

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

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	setlasterror(GENERAL_FAILURE);
	return(-1);
}

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

unlock_mutex(&vfs_mutex);

/*
 * if reading from character device, call device handler and return
 */

if(next->flags == FILE_CHAR_DEVICE) {		/* device i/o */		

	if(next->charioread(addr,size) == -1) {
		unlock_mutex(&vfs_mutex);
		return(-1);
	}

	unlock_mutex(&vfs_mutex);

	setlasterror(NO_ERROR);
	return(NO_ERROR);
}

splitname(next->filename,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	unlock_mutex(&vfs_mutex);

	setlasterror(GENERAL_FAILURE);
	return(-1);
}

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
	addr	Buffer to read to
	size	Number of bytes to read
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t write(size_t handle,void *addr,size_t size) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
FILERECORD *next;
SPLITBUF splitbuf;

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

if(next->flags == FILE_CHAR_DEVICE) {		/* device i/o */		

	if(next->chariowrite(addr,size) == -1) {
		unlock_mutex(&vfs_mutex);

		return(-1);
	}

	unlock_mutex(&vfs_mutex);

	setlasterror(NO_ERROR);
	return(NO_ERROR);
}

splitname(next->filename,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	unlock_mutex(&vfs_mutex);

	setlasterror(GENERAL_FAILURE);
	return(-1);
}

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
	
lock_mutex(&vfs_mutex);

next=openfiles;

while(next != NULL) {					/* find filename in struct */

	if(next->handle == handle) {

		if(next->owner_process != getpid()) {		/* if file is owned by process */
			unlock_mutex(&vfs_mutex);

			setlasterror(ACCESS_DENIED);
			return(-1);
		}

		last->next=next->next;
		kernelfree(next);

		unlock_mutex(&vfs_mutex);

		setlasterror(NO_ERROR);  
		return(NO_ERROR);				/* no error */
	}


	last=next;

	next=next->next;
	count++;
}

unlock_mutex(&vfs_mutex);

setlasterror(INVALID_HANDLE);
return(-1);
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

while(next != NULL) {					/* find filename in struct */

	if(next->owner_process == getpid()) {		/* if file is owned by process */
		last->next=next->next;
		kernelfree(next);
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
 * In:  handle	File handle
 *	pid	Process ID
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t dup_internal(size_t handle,size_t pid) {
FILERECORD *source;
FILERECORD *dest;
FILERECORD *last;
size_t count;

lock_mutex(&vfs_mutex);

source=openfiles;
	
while(source != NULL) {					/* find handle in struct */
	if((source->handle == handle) && (source->owner_process == pid)) break;

	source=source->next;
	count++;
}

if(source == NULL) {
	unlock_mutex(&vfs_mutex);

	setlasterror(INVALID_HANDLE);
	return(-1);
}

/* find end */
dest=openfiles;
	
while(dest != NULL) {
	last=dest;
	dest=dest->next;
}

last->next=kernelalloc(sizeof(FILERECORD));					/* add new entry */
	
if(last->next == NULL) {
	unlock_mutex(&vfs_mutex);
	setlasterror(NO_MEM);				/* out of memory */
	return(-1);
}

last=last->next;		/* point to struct */	

memcpy(last,source,sizeof(FILERECORD));  /* copy data */
	
last->owner_process=getpid();
last->next=NULL;

dest=openfiles;
	
while(dest != NULL) {
	dest=dest->next;
}

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
dup_internal(handle,getpid());
}

/*
 * Duplicate handle and overwrite
 *
 * In:  oldhandle	Existing file handle
	newhandle	File handle to overwrite
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t dup2(size_t oldhandle,size_t newhandle) {
FILERECORD *oldnext;
FILERECORD *newnext;
FILERECORD *last;
size_t savenext;

lock_mutex(&vfs_mutex);

oldnext=openfiles;
	
while(oldnext != NULL) {					/* find handle in struct */
	if(oldnext->handle == oldhandle) break;
	oldnext=oldnext->next;
}

if(oldnext == NULL) {
	unlock_mutex(&vfs_mutex);

	setlasterror(INVALID_HANDLE);
	return(-1);
}

newnext=openfiles;
	
while(newnext != NULL) {					/* find handle in struct */
	if(newnext->handle == newhandle) break;

	newnext=newnext->next;
}

if(newnext == NULL) {
	unlock_mutex(&vfs_mutex);

	setlasterror(INVALID_HANDLE);
	return(-1);
}	

savenext=oldnext->next;		/* save next pointer */

memcpy(newnext,oldnext,sizeof(FILERECORD));  /* copy data */

newnext->next=savenext;
newnext->owner_process=getpid();

unlock_mutex(&vfs_mutex);

setlasterror(NO_ERROR);

return(newnext->handle);
}

size_t timescalled=0;

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

		if(strcmp(newfs->name,next->name) == 0) {		/* already loaded */
			unlock_mutex(&vfs_mutex);
			setlasterror(INVALID_DRIVER);
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

memcpy(next,newfs,sizeof(FILESYSTEM));
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
void *b=NULL;

lock_mutex(&vfs_mutex);

blockbuf=kernelalloc(VFS_MAX);
if(blockbuf == NULL) return(-1);

if(blockio(0,drive,0,blockbuf) == -1) return(-1);		/* read block */

/* check magic number */

next=filesystems;

while(next != NULL) {
	b=blockbuf;
	b += next->location;

	if(memcmp(b,next->magicnumber,next->size) == 0) {	/* if magic number matches */
		memcpy(buf,next,sizeof(FILESYSTEM));

		kernelfree(blockbuf);

		unlock_mutex(&vfs_mutex);

		setlasterror(NO_ERROR);

		return(NO_ERROR);
	}

	next=next->next;
}

unlock_mutex(&vfs_mutex);

setlasterror(INVALID_DRIVER);
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

	if(c == '\\') {
		if(getpid() == -1) {
			c=boot_info->drive+'A';

			b=buf;
			*b++=c;
			*b++=':';

			strcat(b,filename);
			return(NO_ERROR);
		} 

		b=buf;

		c=*cwd;
		*b++=c;
		*b++=':';

		strcat(b,filename);

		goto over;
	} 

/* is already full path, just exit */

if(d == ':' && e == '\\') {               /* full path */
	strcpy(buf,filename);                     /* copy path */
}


if(d == ':' && e != '\\') {               /* full path */
/* drive and filename only */
	memcpy(buf,filename,2); 				/* get drive */
	strcat(buf,filename+3);                       /* get filename */
}

/* filename only */
if(d != ':' && e != '\\') {               /* full path */
	b=cwd;
	b=b+(strlen(cwd))-1;

	c=*b;

	if(c == '\\') {
		ksprintf(buf,"%s%s",cwd,filename);
	}
	else
	{
		ksprintf(buf,"%s\\%s",cwd,filename);
	}

}

over:
tc=tokenize_line(cwd,token,"\\");		/* tokenize line */
dottc=tokenize_line(filename,dottoken,"\\");		/* tokenize line */


for(countx=0;countx<dottc;countx++) {
	if(strcmp(dottoken[countx],"..") == 0 ||  strcmp(dottoken[countx],".") == 0) {

		for(count=0;count<dottc;count++) {

			if(strcmp(dottoken[count],"..") == 0 || strcmp(dottoken[countx],".") == 0) {
				tc--;
			}
			else
			{
				strcpy(token[tc],dottoken[count]);
					tc++;
			}
		}


		strcpy(buf,token[0]);
		strcat(buf,"\\");

		for(count=1;count<tc;count++) {
			strcat(buf,token[count]);

			if(count != tc-1) strcat(buf,"\\");
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


size_t openfiles_init(size_t a,size_t b) {
FILERECORD *next;

filesystems=NULL;

openfiles=kernelalloc(sizeof(FILERECORD));			/* stdin */
if(openfiles == NULL) {	/*can't allocate */
	kprintf_direct("kernel: can't allocate memory for stdin\n");
	return(-1);
}

strcpy(openfiles->filename,"stdin");
openfiles->handle=0;
openfiles->charioread=NULL;			/* for now */
openfiles->chariowrite=NULL;
openfiles->flags=FILE_CHAR_DEVICE;

openfiles->next=kernelalloc(sizeof(FILERECORD));			/* stdin */
if(openfiles->next == NULL) {	/*can't allocate */
	kprintf_direct("kernel: can't allocate memory for stdout\n");
	return(-1);
}

next=openfiles->next;

strcpy(next->filename,"stdout");
next->handle=1;
next->charioread=NULL;			/* for now */
next->chariowrite=NULL;
next->flags=FILE_CHAR_DEVICE;

next->next=kernelalloc(sizeof(FILERECORD));			/* stdin */
if(next->next == NULL) {	/*can't allocate */
	kprintf_direct("kernel: can't allocate memory for stderr\n");
	return(-1);
}

next=next->next;

strcpy(next->filename,"stderr");
next->handle=2;
next->charioread=NULL;			/* for now */
next->chariowrite=NULL;
next->next=NULL;
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

		if(type == _READ) {
			next->charioread=cptr;
		}
		else if(type == _WRITE) {
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

splitname(next->filename,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	unlock_mutex(&vfs_mutex);

	setlasterror(GENERAL_FAILURE);
	return(-1);
}

unlock_mutex(&vfs_mutex);

return(fs.seek(handle,pos,whence));
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

size_t setfiletimedate(char *filename,TIMEBUF *createtime,TIMEBUF *lastmodtime) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;

splitname(filename,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	setlasterror(GENERAL_FAILURE);
	return(-1);
}


return(fs.setfiletd(filename,createtime,lastmodtime));
}

/*
 * Get start block of file
 *
 * In:  name	Filename
 *
 * Returns: -1 on error, start block on success
 *
 */

size_t getstartblock(char *name) {
FILESYSTEM fs;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;

splitname(name,&splitbuf);				/* split name */

if(detect_filesystem(splitbuf.drive,&fs) == -1) {
	setlasterror(GENERAL_FAILURE);
	return(-1);
}

if(fs.getstartblock == NULL) {			/* not implemented */
	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}

return(fs.getstartblock(name));
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
size_t foundhandle=FALSE;

lock_mutex(&vfs_mutex);

next=openfiles;

while(next != NULL) {
/* update own handle */

	if((next->handle == handle) && (next->owner_process == getpid())) {		/* found handle */

		memcpy(next,buf,sizeof(FILERECORD));		/* copy data */
		foundhandle=TRUE;
	}

/* update other open file handles for the file */

	if((strcmp(next->filename,buf->filename) == 0)) {	/* found filename */

		next->timebuf.seconds=buf->timebuf.seconds;  	/* copy file time and date */
		next->timebuf.minutes=buf->timebuf.minutes;
		next->timebuf.hours=buf->timebuf.hours;  
		next->timebuf.day=buf->timebuf.day;  
		next->timebuf.month=buf->timebuf.month;
		next->timebuf.year=buf->timebuf.year;  

		next->filesize=buf->filesize;			/* copy file size */
		foundhandle=TRUE;
	}

	next=next->next;
}

lock_mutex(&vfs_mutex);

if(foundhandle == FALSE) return(-1);		/* handle not found */

return(0);
}

/*
 * Get single part of path from filename
 *
 * In:  filename	Filename to get part of filename from
	buf		Buffer to hold filename part
 *
 * Returns: -1 on error, 0 on success
 *
 * Returns a single part of the filename each time it is called
 */

size_t get_filename_token(char *filename,void *buf) {
char *f;
char *sf;
size_t count=0;

if(old == NULL)  {					/* start of filename */
	old=filename;
	f=filename;
}

f=old;

sf=f;

while(*f != '\\') {

	count++;					/* at end of token */
	
	if(*f++ == 0) { 
		count++;
		memcpy(buf,sf++,count);
		return(-1);				/* at end of filename */
	}

}

count++;

memcpy(buf,sf++,count-1);
old=f+1;

return(0);
}

/*
 * Get number of parts in filename seperated by \\
 *
 * In:  handle	File handle
	addr	Buffer to read to
	size	Number of bytes to read
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t get_filename_token_count(char *filename) {
char *f;
size_t count;
	
count=0;

f=filename;

while(*f != 0) {
	if(*f++ == '\\') count++;					/* at end of token */
}
	
count++;

return(count);						/* return number of tokens */
}


size_t change_file_owner_pid(size_t handle,size_t pid) {
FILERECORD *next;

lock_mutex(&vfs_mutex);

next=openfiles;

while(next != NULL) {
	if(next->handle == handle) {		/* found handle */
		next->owner_process=pid;		/* set owner */
		unlock_mutex(&vfs_mutex);

		return(0);
	}


	next=next->next;
}

unlock_mutex(&vfs_mutex);
return(-1);
}

