/* CCP Version 0.0.1
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
#include "errors.h"
#include "vfs.h"
#include "filesystem.h"

#define MODULE_INIT example_filesystem_init

/*
 * Initialize example filesystem module
 *
 * In:  initstr	Initialisation string
 *
 * Returns: 0 on success or -1 on failure
 *
 */
size_t example_filesystem_init(char *initstr) {
FILESYSTEM example_filesystem;

/* Filesystem identification magic number */

MAGIC magicdata[] = { 
	{ "EXAMPLE",		/* filesystem identification */
		8,		/* length */
		3		/* location in boot block */
	},
};

example_filesystem_filesytem.magic_count=1;		/* number of magicdata entries */

memcpy(&example_filesystem.magicbytes,&magicdata,example_filesystemfilesystem.magic_count*sizeof(MAGIC));	/* copy magicdata entries */

strncpy(example_filesystemfilesystem.name,"example_filesystem",MAX_PATH);	/* filesystem name */

example_filesystemfilesystem.findfirst=&example_filesystem_findfirst;		/* findfirst() handler */
example_filesystemfilesystem.findnext=&example_filesystem_findnext;		/* findnext() handler */
example_filesystemfilesystem.read=&example_filesystem_read;			/* read() handler */
example_filesystemfilesystem.write=&example_filesystem_write;			/* write() handler */
example_filesystemfilesystem.rename=&example_filesystem_rename;			/* rename() handler */
example_filesystemfilesystem.delete=&example_filesystem_unlink;			/* unlink() handler */
example_filesystemfilesystem.mkdir=&example_filesystem_mkdir;			/* mkdir() handler */
example_filesystemfilesystem.rmdir=&example_filesystem_rmdir;			/* rmdir() handler */
example_filesystemfilesystem.create=&example_filesystem_create;			/* create() handler */
example_filesystemfilesystem.chmod=&example_filesystem_chmod;			/* chmod() handler */
example_filesystemfilesystem.touch=&example_filesystem_touch;		/* touch() handler */

return(register_filesystem(&example_filesystem));		/* register filesystem */
}

/*
 * Find first file in directory
 *
 * In:  name	Filename or wildcard of file to find
		buffer	Buffer to hold information about files
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t example_filesystem_findfirst(char *filename,FILERECORD *buf) {

setlasterror(NO_ERROR);
return(0);
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
size_t example_filesystem_findnext(char *filename,FILERECORD *buf) {

setlasterror(NO_ERROR);
return(0);
}

/*
 * Internal find function
 *
 * In:  type	Type of find, 0=first file, 1=next file
	name	Filename or wildcard of file to find
	buf	Buffer to hold information about files
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t example_filesystem_find(size_t find_type,char *filename,FILERECORD *buf) {

setlasterror(NO_ERROR);
return(0);
}

/*
 * Read from file
 *
 * In:  handle	File handle
	addr	Buffer to read to
	size	Number of bytes to read
 *
 * Returns: -1 on error, number of bytes read on success
 *
 */

size_t example_filesystem_read(size_t handle,void *addr,size_t size) {

setlasterror(NO_ERROR);
return(0);
}

/*
 * Write to file
 *
 * In:  handle	File handle
	addr	Buffer to write from
	size	Number of bytes to write
 *
 * Returns: -1 on error or number of bytes written on success
 *
 */

size_t example_filesystem_write(size_t handle,void *addr,size_t size) {

setlasterror(NO_ERROR);
return(0);
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

size_t example_filesystem_rename(char *filename,char *newname) {

setlasterror(NO_ERROR);
return(0);
}

/*
 * Delete file
 *
 * In:  name	File to delete
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t example_filesystem_delete(char *filename) {

setlasterror(NO_ERROR);
return(0);
}

/*
 * Create file
 *
 * In:  dirname	File to create
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t example_filesystem_create(char *filename) {

setlasterror(NO_ERROR);
return(0);
}

/*
 * Create directory
 *
 * In:  name	Directory to create
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t example_filesystem_mkdir(char *dirname) {

setlasterror(NO_ERROR);
return(0);
}

/*
 * Remove directory
 *
 * In:  dirname	Directory to remove
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t example_filesystem_rmdir(char *dirname) {

setlasterror(NO_ERROR);
return(0);
}

/*
 * Set file attributes
 *
 * In:  filename	Filename
 *	attributes	attributes
 * Returns: -1 on error, 0 on success
 *
 */
size_t example_filesystem_chmod(char *filename,size_t attributes) {

setlasterror(NO_ERROR);
return(0);
}

/*
 * Set file time and date
 *
 * In:  filename	Filename
 *	filetime	
 * Returns: -1 on error, 0 on success
 *
 * Stub function that calls internal create function
 */
size_t example_filesystem_touch(char *filename,TIME *create_time_date,TIME *last_modified_time_date,TIME *last_accessed_time_date) {

setlasterror(NO_ERROR);
return(0);
}

