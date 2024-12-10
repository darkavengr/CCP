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
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "memorymanager.h"
#include "fat.h"
#include "debug.h"

/* 
 * ExFAT Filesystem
 *
 */

/*
 * Initialize ExFAT filesystem module
 *
 * In:  initstr	Initialisation string
 *
 * Returns: 0 on success or -1 on failure
 *
 */
size_t exexfat_init(char *initstr) {
FILESYSTEM exfatfilesystem;

strncpy(exfatfilesystem.name,"ExFAT",MAX_PATH);
strncpy(exfatfilesystem.magicnumber,"EXFAT   ",MAX_PATH);
exfatfilesystem.size=8;
exfatfilesystem.location=0x3;
exfatfilesystem.findfirst=&exfat_findfirst;
exfatfilesystem.findnext=&exfat_findnext;
exfatfilesystem.read=&exfat_read;
exfatfilesystem.write=&exfat_write;
exfatfilesystem.rename=&exfat_rename;
exfatfilesystem.delete=&exfat_delete;
exfatfilesystem.mkdir=&exfat_mkdir;
exfatfilesystem.rmdir=&exfat_rmdir;
exfatfilesystem.create=&exfat_create;
exfatfilesystem.chmod=&exfat_chmod;
exfatfilesystem.setfiletd=&exfat_set_file_time_date;

return(register_filesystem(&exfatfilesystem));
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
size_t fat_findfirst(char *filename,FILERECORD *buf) {
	return(fat_find(FALSE,filename,buf));
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
size_t fat_findnext(char *filename,FILERECORD *buf) {
	return(fat_find(TRUE,filename,buf));
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
size_t exfat_find(size_t find_type,char *filename,FILERECORD *buf) {

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

size_t exfat_read(size_t handle,void *addr,size_t size) {
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

size_t exfat_write(size_t handle,void *addr,size_t size) {
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

size_t exfat_rename(char *filename,char *newname) {
}

/*
 * Delete file
 *
 * In:  name	File to delete
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t exfat_delete(char *filename) {
}

/*
 * Create directory
 *
 * In:  name	Directory to create
 *
 * Returns: -1 on error, 0 on success
 *
 * Stub function that calls internal create function
 */
size_t fat_exmkdir(char *dirname) {
return(fat_excreate_int(_DIR,dirname));
}

/*
 * Create file
 *
 * In:  dirname	File to create
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_excreate(char *filename) {
return(fat_excreate_int(_FILE,filename));
}

/*
 * Create fat entry internal function
 *
 * In:  entrytype	Type of entry (0=file,1=directory)
	name		File or directory to create
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t exfat_create_int(size_t type,char *filename) {
}

/*
 * Create directory
 *
 * In:  name	Directory to create
 *
 * Returns: -1 on error, 0 on success
 *
 * Stub function that calls internal create function
 */
size_t exfat_mkdir(char *dirname) {
return(exfat_create_int(_DIR,dirname));
}

/*
 * Create file
 *
 * In:  dirname	File to create
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t exfat_create(char *filename) {
return(exfat_create_int(_FILE,filename));
}

/*
 * Create fat entry internal function
 *
 * In:  entrytype	Type of entry (0=file,1=directory)
	name		File or directory to create
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t fat_create_int(size_t type,char *filename) {
}

