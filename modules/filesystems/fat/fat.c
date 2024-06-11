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

/* 
 * FAT12/FAT16/FAT32 Filesystem for CCP
 *
 */

#include <stdint.h>
#include "../../../header/errors.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "../../../filemanager/vfs.h"
#include "../../../memorymanager/memorymanager.h"
#include "fat.h"
#include "../../../header/debug.h"

#define MODULE_INIT fat_init

extern old;

size_t b;

size_t fat_findfirst(char *filename,FILERECORD *buf);							// TESTED
size_t fat_findnext(char *filename,FILERECORD *buf);							// TESTED
size_t fat_find(size_t find_type,char *filename,FILERECORD *buf);					// TESTED
size_t fat_rename(char *filename,char *newname);							// TESTED - MORE
size_t fat_rmdir(char *dirname);									// TESTED
size_t fat_mkdir(char *dirname);									// TESTED
size_t fat_create(char *filename);									// TESTED - MORE						
size_t fat_unlink(char *filename);									// TESTED
size_t fat_read(size_t handle,void *addr,size_t size);							// TESTED
size_t fat_write(size_t handle,void *addr,size_t size);							// TESTED
size_t fat_chmod(char *filename,size_t attribs);							// TESTED
size_t fat_set_file_time_date(char *filename,TIME *create_time_date,TIME *last_modified_time_date,TIME *last_accessed_time_date); // TESTED
size_t fat_find_free_block(size_t drive);								// TESTED
size_t fat_get_start_block(char *name);									// TESTED
size_t fat_get_next_block(size_t drive,uint64_t block);							// TESTED
size_t fat_update_fat(size_t drive,uint64_t block,uint16_t block_high_word,uint16_t block_low_word);	// TESTED - MORE
size_t fat_detect_change(size_t drive);									// TESTED
size_t fat_convert_filename(char *filename,char *outname);						// TESTED
size_t fat_read_long_filename(size_t drive,uint64_t block,size_t entryno,FILERECORD *n);		// TESTED
size_t fat_update_long_filename(size_t type,size_t drive,uint64_t block,size_t entryno,FILERECORD *new);// TESTED
size_t fat_create_short_name(char *filename,char *out);							// TESTED
uint8_t fat_create_long_filename_checksum(char *filename);						// TESTED
size_t fat_unlink_long_filename(char *filename,uint64_t block,size_t entry);				// TESTED
size_t fat_create_long_filename(size_t type,FILERECORD *newname,uint64_t block,size_t entry);		// TESTED
void fat_entry_to_filename(char *filename,char *out);							// TESTED
size_t fat_init(char *i);										// TESTED
size_t fat_create_int(size_t entrytype,char *filename);							// TESTED
size_t fat_create_entry(size_t type,size_t drive,uint64_t rb,size_t entryno,size_t datastart,char *filename,char *blockbuf); // TESTED
size_t fat_is_long_filename(char *filename);								// TESTED
size_t create_subdirectory_entries(char *dirname,size_t datastart,size_t startblock,TIME *time,TIME *date,size_t fattype,size_t drive); // TESTED

/*
 * Initialize FAT filesystem module
 *
 * In:  initstr	Initialisation string
 *
 * Returns: 0 on success or -1 on failure
 *
 */
size_t fat_init(char *initstr) {
FILESYSTEM fatfilesystem;

strncpy(fatfilesystem.name,"FAT",MAX_PATH);
strncpy(fatfilesystem.magicnumber,"FAT",MAX_PATH);
fatfilesystem.size=3;
fatfilesystem.location=0x36;
fatfilesystem.findfirst=&fat_findfirst;
fatfilesystem.findnext=&fat_findnext;
fatfilesystem.read=&fat_read;
fatfilesystem.write=&fat_write;
fatfilesystem.rename=&fat_rename;
fatfilesystem.unlink=&fat_unlink;
fatfilesystem.mkdir=&fat_mkdir;
fatfilesystem.rmdir=&fat_rmdir;
fatfilesystem.create=&fat_create;
fatfilesystem.chmod=&fat_chmod;
fatfilesystem.touch=&fat_set_file_time_date;

return(register_filesystem(&fatfilesystem));
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
 * Internal FAT find function
 *
 * In:  type	Type of find, 0=first file, 1=next file
	name	Filename or wildcard of file to find
	buf	Buffer to hold information about files
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t fat_find(size_t find_type,char *filename,FILERECORD *buf) {
size_t lfncount;
size_t fattype;
size_t rootdir;
size_t start_of_data_area;
char *fp[MAX_PATH];
char *file[11];
BLOCKDEVICE blockdevice;
DIRENT dirent;
char *blockbuf;
char *blockptr;
SPLITBUF *splitbuf;
FILERECORD *lfnbuf;
BPB *bpb;

lfnbuf=kernelalloc(sizeof(FILERECORD));	/* allocate split buffer */
if(lfnbuf == NULL) return(-1);

splitbuf=kernelalloc(sizeof(SPLITBUF));			/* allocate split buffer */
if(splitbuf == NULL) {
	kernelfree(lfnbuf);
	return(-1);
}

if(find_type == FALSE) {
	getfullpath(filename,fp);			/* get full filename */
	touppercase(fp);				/* convert to uppercase */

	splitname(fp,splitbuf);				/* split filename */

	buf->findentry=0;
	buf->findlastblock=fat_get_start_block(splitbuf->dirname);
	if(buf->findlastblock == -1) return(-1);

	strncpy(buf->searchfilename,fp,MAX_PATH); 
}
else
{
	splitname(buf->searchfilename,splitbuf);	/* split filename */

}

if(strlen(splitbuf->filename) == 0) strncpy(splitbuf->filename,"*",MAX_PATH);

fattype=fat_detect_change(splitbuf->drive);
if(fattype == -1) {
	kernelfree(splitbuf);
	kernelfree(lfnbuf);
	setlasterror(INVALID_DISK);
	return(-1);
}

if(getblockdevice(splitbuf->drive,&blockdevice) == -1) {
	kernelfree(splitbuf);
	kernelfree(lfnbuf);
	setlasterror(INVALID_DISK);
	return(-1);
} 

bpb=blockdevice.superblock;

if(strncmp(splitbuf->dirname,"\\",MAX_PATH) != 0) { /* not root */
	if(fattype == 12 || fattype == 16) {
		rootdir=((bpb->rootdirentries*FAT_ENTRY_SIZE)+(bpb->sectorsperblock-1))/bpb->sectorsize;
		start_of_data_area=rootdir+(bpb->reservedsectors+(bpb->numberoffats*bpb->sectorsperfat))-2; /* get start of data area */
	}
	else
	{
		start_of_data_area=bpb->fat32rootdirstart+(bpb->reservedsectors+(bpb->numberoffats*bpb->fat32sectorsperfat))-2;
	}
	
}
else
{
	start_of_data_area=0;
}

	

bpb=blockdevice.superblock;				/* point to superblock data*/

blockbuf=kernelalloc(bpb->sectorsperblock*bpb->sectorsize);			/* allocate block buffer */ 
if(blockbuf == NULL) {
	kernelfree(splitbuf);
	kernelfree(lfnbuf);
	return(-1);
}

/*
 * If it's the root directory, it is located at a fixed position for fat12 and fat16.
   All other directories are located in the data area. */

if(strncmp(splitbuf->dirname,"\\",MAX_PATH) != 0) {		 /* not root */
	if(fattype == 12 || fattype == 16) {
		rootdir=((bpb->rootdirentries*FAT_ENTRY_SIZE)+(bpb->sectorsperblock-1))/bpb->sectorsize;
		start_of_data_area=rootdir+(bpb->reservedsectors+(bpb->numberoffats*bpb->sectorsperfat))-2; /* get start of data area */
	}
	else
	{
		start_of_data_area=bpb->fat32rootdirstart+(bpb->reservedsectors+(bpb->numberoffats*bpb->fat32sectorsperfat))-2;
	}
	
}
else
{
	start_of_data_area=0;
}

/* search thorough directory until filename found */

while(1) {
	blockptr=blockbuf+(buf->findentry*FAT_ENTRY_SIZE);	/* point to next file */

	if(blockio(_READ,splitbuf->drive,(uint64_t) buf->findlastblock+start_of_data_area,blockbuf) == -1) { /* read directory block */
		kernelfree(blockbuf);
		kernelfree(lfnbuf);
		kernelfree(splitbuf);
		return(-1);
	}

	while(buf->findentry < ((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE)) {
		buf->findentry++;

		memcpy(&dirent,blockptr,FAT_ENTRY_SIZE);				/* copy entry */

		if(*blockptr == 0) {						/* at end of directory */
			kernelfree(blockbuf);
			kernelfree(lfnbuf);
			kernelfree(splitbuf);
		
			setlasterror(END_OF_DIRECTORY);
			return(-1);
		}

		if((unsigned char) *blockptr == 0xE5) {				/* ignore deleted files */
			blockptr += FAT_ENTRY_SIZE;
			continue;
		}

		fat_entry_to_filename(dirent.filename,file);			/* convert filename */
	
		if(dirent.attribs == 0x0f) {			/* long filename */
			lfncount=fat_read_long_filename(splitbuf->drive,buf->findlastblock,buf->findentry-1,lfnbuf);
	
			blockptr += (lfncount*FAT_ENTRY_SIZE);	

			if(wildcard(splitbuf->filename,lfnbuf->filename) == 0) { 	/* if file found */      	
				strncpy(buf->filename,lfnbuf->filename,MAX_PATH); 	/* copy file information from long filename data returned */
				buf->attribs=lfnbuf->attribs;

				buf->create_time_date.day=lfnbuf->create_time_date.day;
				buf->create_time_date.month=lfnbuf->create_time_date.month;
				buf->create_time_date.year=lfnbuf->create_time_date.year;
				buf->create_time_date.hours=lfnbuf->create_time_date.hours;
				buf->create_time_date.minutes=lfnbuf->create_time_date.minutes;
				buf->create_time_date.seconds=lfnbuf->create_time_date.seconds;
				
				buf->last_written_time_date.day=lfnbuf->last_written_time_date.day;
				buf->last_written_time_date.month=lfnbuf->last_written_time_date.month;
				buf->last_written_time_date.year=lfnbuf->last_written_time_date.year;
				buf->last_written_time_date.hours=lfnbuf->last_written_time_date.hours;
				buf->last_written_time_date.minutes=lfnbuf->last_written_time_date.minutes;
				buf->last_written_time_date.seconds=lfnbuf->last_written_time_date.seconds;

				buf->last_accessed_time_date.day=lfnbuf->last_accessed_time_date.day;
				buf->last_accessed_time_date.month=lfnbuf->last_accessed_time_date.month;
				buf->last_accessed_time_date.year=lfnbuf->last_accessed_time_date.year;
				buf->last_accessed_time_date.hours=lfnbuf->last_accessed_time_date.hours;
				buf->last_accessed_time_date.minutes=lfnbuf->last_accessed_time_date.minutes;
				buf->last_accessed_time_date.seconds=lfnbuf->last_accessed_time_date.seconds;

				buf->filesize=lfnbuf->filesize;
				buf->startblock=lfnbuf->startblock;

				/* extra information */
				buf->currentblock=buf->startblock;

				buf->drive=splitbuf->drive;
				buf->dirent=buf->findentry-1;
				buf->findentry=buf->dirent;
				buf->access=0;
				buf->currentpos=0;

				if(buf->attribs & FAT_ATTRIB_DIRECTORY) {
					buf->flags=FILE_DIRECTORY;
				}
				else
				{
					buf->flags=FILE_REGULAR;
				}

				kernelfree(blockbuf);
				kernelfree(splitbuf);

				setlasterror(NO_ERROR);
				return(0);
			 }	



		}
		else
		{		
			if(wildcard(splitbuf->filename,file) == 0) { 				/* if file found */     
				memset(buf->filename,0,MAX_PATH);

				/* copy file information from the FAT entry */
				strncpy(buf->filename,file,MAX_PATH);			/* convert filename */

				buf->attribs=dirent.attribs;
				buf->filesize=dirent.filesize;
				buf->startblock=(dirent.block_high_word << 16)+(dirent.block_low_word);
				buf->drive=splitbuf->drive;
				buf->dirent=buf->findentry-1;
				buf->access=0;
				buf->currentpos=0;
				
				buf->create_time_date.hours=(dirent.createtime & 0xf800) >> 11;
				buf->create_time_date.minutes=(dirent.createtime  & 0x7e0) >> 5;
				buf->create_time_date.seconds=(dirent.createtime & 0x1f);
				buf->create_time_date.year=((dirent.createdate & 0xfe00) >> 9)+1980;
				buf->create_time_date.month=(dirent.createdate & 0x1e0) >> 5;
				buf->create_time_date.day=(dirent.createdate & 0x1f);
	
				buf->last_written_time_date.hours=(dirent.last_modified_time & 0xf800) >> 11;
				buf->last_written_time_date.minutes=(dirent.last_modified_time  & 0x7e0) >> 5;
				buf->last_written_time_date.seconds=(dirent.last_modified_time & 0x1f);
				buf->last_written_time_date.year=((dirent.last_modified_date & 0xfe00) >> 9)+1980;
				buf->last_written_time_date.month=(dirent.last_modified_date & 0x1e0) >> 5;
				buf->last_written_time_date.day=(dirent.last_modified_date & 0x1f);
	
				buf->last_accessed_time_date.hours=(dirent.last_modified_time & 0xf800) >> 11;
				buf->last_accessed_time_date.minutes=(dirent.last_modified_time  & 0x7e0) >> 5;
				buf->last_accessed_time_date.seconds=(dirent.last_modified_time & 0x1f);
				buf->last_accessed_time_date.year=((dirent.last_modified_date & 0xfe00) >> 9)+1980;
				buf->last_accessed_time_date.month=(dirent.last_modified_date & 0x1e0) >> 5;
				buf->last_accessed_time_date.day=(dirent.last_modified_date & 0x1f);

				buf->drive=splitbuf->drive;		/* additional information */

				if(buf->attribs & FAT_ATTRIB_DIRECTORY) {	/* get file type */
					buf->flags=FILE_DIRECTORY;
				}
				else
				{
					buf->flags=FILE_REGULAR;
				}

				kernelfree(blockbuf);
				kernelfree(splitbuf);
	
				setlasterror(NO_ERROR);
				return(0);			
			}

			blockptr=blockptr+FAT_ENTRY_SIZE;	
		}
	}

	if(buf->findentry >= ((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE)) {		/* at end of block */
		buf->findlastblock=fat_get_next_block(splitbuf->drive,buf->findlastblock);	/* get next block */		
		buf->findentry=0;

		if((fattype == 12 && buf->findlastblock >= 0xff8) || (fattype == 16 && buf->findlastblock >= 0xfff8) || (fattype == 32 && buf->findlastblock >= 0xfffffff8)) {
			kernelfree(blockbuf);
			kernelfree(splitbuf);

			setlasterror(END_OF_DIRECTORY);
			return(-1);
		}
	}

}

kernelfree(blockbuf);
kernelfree(splitbuf);
kernelfree(lfnbuf);

setlasterror(END_OF_DIRECTORY);
return(-1);
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

size_t fat_rename(char *filename,char *newname) {
SPLITBUF splitbuf_old;
SPLITBUF splitbuf_new;
char *blockbuf;
FILERECORD buf;
FILERECORD newbuf;
char *blockptr;
char *fullpath[MAX_PATH];
char *newfullpath[MAX_PATH];
size_t type;

getfullpath(filename,fullpath);			/* get full path of filename */
getfullpath(newname,newfullpath);		/* get full path of new filename */

splitname(fullpath,&splitbuf_old);		/* split name */
splitname(newfullpath,&splitbuf_new);		/* split name */

if(splitbuf_old.drive != splitbuf_new.drive) {	/* can't rename across drives */
	setlasterror(RENAME_ACR_DRIVE);
	return(-1);
}

if(findfirst(fullpath,&buf) == -1) {		/* get file entry */
	setlasterror(FILE_NOT_FOUND);
	return(-1);
}

if(findfirst(newfullpath,&newbuf) == 0) {		/* if new filename exists */
	setlasterror(FILE_EXISTS);
	return(-1);
}

if(fat_is_long_filename(splitbuf_new.filename)) {	/* long filename */
	strncpy(buf.filename,splitbuf_new.filename,MAX_PATH);

	if(newbuf.attribs & FAT_ATTRIB_DIRECTORY) {		/* renaming directory */
		type=_DIR;
	}
	else
	{
		type=_FILE;
	}

	return(fat_update_long_filename(type,splitbuf_old.drive,(uint64_t) buf.findlastblock,buf.dirent,&buf));
}

blockbuf=kernelalloc(MAX_BLOCK_SIZE);			/* allocate block buffers */
if(blockbuf == NULL) {
	setlasterror(NO_MEM);
	return(-1);
}
			
if(blockio(_READ,splitbuf_old.drive,(uint64_t) buf.findlastblock,blockbuf) == -1) return(-1); /* read block */

blockptr=blockbuf+(buf.dirent*FAT_ENTRY_SIZE);			/* point to entry */

fat_convert_filename(splitbuf_new.filename,blockptr);			/* create new directory entry */

if(blockio(_WRITE,splitbuf_old.drive,buf.findlastblock,blockbuf) == -1) return(-1); /* write block */

setlasterror(NO_ERROR);  
return(NO_ERROR);				/* no error */
}



/*
 * Remove directory
 *
 * In:  name	Directory to remove
 *
 * Returns: -1 on error, 0 on success
 *
 * Stub function to call fat_unlink 
 *
 */

size_t fat_rmdir(char *dirname) {
return(fat_unlink(dirname));
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
size_t fat_mkdir(char *dirname) {
return(fat_create_int(_DIR,dirname));
}

/*
 * Create file
 *
 * In:  dirname	File to create
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_create(char *filename) {
return(fat_create_int(_FILE,filename));
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
size_t fattype;
size_t rootdir;
size_t start_of_data_area;
char *fullpath[MAX_PATH];
char *blockbuf;
char *blockptr;
SPLITBUF splitbuf;
BPB *bpb;
FILERECORD filecheck;
size_t entry_count=0;
uint64_t rb;
BLOCKDEVICE blockdevice;
size_t readblock;
size_t previousblock;

getfullpath(filename,fullpath);

DEBUG_PRINT_STRING(fullpath);
	
if(findfirst(fullpath,&filecheck) == 0) {			/* file exists */
	setlasterror(FILE_EXISTS);
	return(-1);
}

splitname(fullpath,&splitbuf);

fattype=fat_detect_change(splitbuf.drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
}

if(getblockdevice(splitbuf.drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

bpb=blockdevice.superblock;				/* point to superblock data*/

if((fattype == 12) || (fattype == 16)) {
	rootdir=((bpb->rootdirentries*FAT_ENTRY_SIZE)+(bpb->sectorsperblock-1))/bpb->sectorsize;
	start_of_data_area=rootdir+(bpb->reservedsectors+(bpb->numberoffats*bpb->sectorsperfat))-2; /* get start of data area */
}
else
{
	start_of_data_area=bpb->fat32rootdirstart+(bpb->reservedsectors+(bpb->numberoffats*bpb->fat32sectorsperfat))-2;	
}

blockbuf=kernelalloc(bpb->sectorsperblock*bpb->sectorsize);			/* allocate block buffer */ 
if(blockbuf == NULL) return(-1);

rb=fat_get_start_block(splitbuf.dirname);		/* get start of directory where new file will be created */
if(rb == -1) return(-1);

/* search thorough directory until free entry found */

while(1) {

	if(strncmp(splitbuf.dirname,"\\",MAX_PATH) == 0) {	/* root */		
		readblock=rb;
	}
	else
	{
		readblock=rb+start_of_data_area;			/* not root */
	}

	if(blockio(_READ,splitbuf.drive,(uint64_t) readblock,blockbuf) == -1) { /* read directory block */
		kernelfree(blockbuf);
		return(-1);
	}

	/* loop through the entries in the block */

	blockptr=blockbuf;

	entry_count=0;

	while(entry_count < ((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE)) {
				
		if(((unsigned char) *blockptr == 0xE5) || ((unsigned char) *blockptr == 0)) {		/* found free entry */	
			return(fat_create_entry(type,splitbuf.drive,(uint64_t) rb,entry_count,start_of_data_area,splitbuf.filename,blockbuf));			
		}
		
		entry_count++;
		blockptr=blockptr+FAT_ENTRY_SIZE;
	}


	if(entry_count >= ((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE)) {		/* at end of block */
		previousblock=rb;			/* save previous block */

		rb=fat_get_next_block(splitbuf.drive,rb);			/* get next block */		

		entry_count=0;

		/* at end of directory */

		if((fattype == 12 && rb >= 0xff8) || (fattype == 16 && rb >= 0xfff8) || (fattype == 32 && rb >= 0xfffffff8)) {
			/* The root directory is fixed in size for fat12 and fat16 */

			if((strncmp(splitbuf.dirname,"\\",MAX_PATH) == 0) && (fattype == 12 || fattype == 16)) { 
				kernelfree(blockbuf);

				setlasterror(DIRECTORY_FULL);
				return(-1);
			}
		
			rb=fat_find_free_block(splitbuf.drive);		/* find free block for directory entry */
			if(rb == -1) return(-1);

			if(fat_update_fat(splitbuf.drive,previousblock,(rb >> 8),(rb & 0xffff)) == -1) return(-1);	/* update FAT */

			return(fat_create_entry(type,splitbuf.drive,rb,(uint64_t) 0,start_of_data_area,splitbuf.filename,blockbuf));			
		}
	}
    
}

kernelfree(blockbuf);

setlasterror(END_OF_DIRECTORY);
return(-1);
}

/*
 * Create fat entry internal function
 *
 * In:  type		Type of entry (0=file,1=directory)
	drive		Drive
	rb		FAT directory block
	entryno		FAT entry number within directory block
	datastart	Data area start block
	filename	File or directory to create
	blockbuf	Pointer to buffer holding directory entries
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t fat_create_entry(size_t type,size_t drive,uint64_t rb,size_t entryno,size_t datastart,char *filename,char *blockbuf) {
FILERECORD lfn_filerecord;
uint16_t time;
uint16_t date;
TIME TIME;
size_t count;
char *fullpath[MAX_PATH];
SPLITBUF splitbuf;
char *blockptr;
DIRENT dirent;
size_t startblock;
size_t fattype;
size_t writeblock;

getfullpath(filename,fullpath);
splitname(fullpath,&splitbuf);

gettime(&TIME);				/* get time and date */

time=(TIME.hours << 11) | (TIME.minutes << 5) | TIME.seconds; /* convert time and date to FAT format */
date=((TIME.year-1980) << 9) | (TIME.month << 5) | TIME.day;

if((type == _FILE) || (type == _DIR)) {				/* create file or directory */

	if(fat_is_long_filename(filename) == TRUE) {		/* create long filename */
		memset(&lfn_filerecord,0,sizeof(FILERECORD));

		strncpy(lfn_filerecord.filename,&splitbuf.filename,MAX_PATH);
		memcpy(&lfn_filerecord.create_time_date,&TIME,sizeof(TIME));
		memcpy(&lfn_filerecord.last_written_time_date,&TIME,sizeof(TIME));
		memcpy(&lfn_filerecord.last_accessed_time_date,&TIME,sizeof(TIME));

		lfn_filerecord.startblock=-1;

		return(fat_create_long_filename(type,&lfn_filerecord,(uint64_t) rb,entryno));

	}

	/* get fat entry */

	fat_convert_filename(splitbuf.filename,dirent.filename);		/* filename */

	if(type == _FILE) {
		dirent.attribs=0;		/* file attributes */
	}
	else {					/* if creating directory, set directory attribute */
		dirent.attribs=FAT_ATTRIB_DIRECTORY;
	}

	dirent.createtime=time;			/* create time */
	dirent.createdate=date;			/* create date */
	dirent.last_modified_time=time;		/* last modified time */
	dirent.last_modified_date=date;		/* last modified date */
	dirent.filesize=0;			/* file size */

	
	/* get start block */

	if(type == _FILE) {			/* files have a zero cluster number if they are empty */
		startblock=0;
	}
	else					/* get block for subdirectory entries */
	{
		startblock=fat_find_free_block(splitbuf.drive);		/* find free block */
		if(startblock == -1) return(-1);
	}

	if((fattype == 12) || (fattype == 16)) {
		dirent.block_low_word=(startblock & 0xffff);
	} 
	else
	{
		dirent.block_high_word=(startblock >> 16);
		dirent.block_low_word=(startblock & 0xffff);
	}
				
	blockptr=blockbuf+(entryno*FAT_ENTRY_SIZE);	/* point to fat entry */

	memcpy(blockptr,&dirent,FAT_ENTRY_SIZE);			/* copy directory entry */

	if(strncmp(splitbuf.dirname,"\\",MAX_PATH) == 0) {	/* root */		
		writeblock=rb;
	}
	else
	{
		writeblock=rb+datastart;			/* not root */
	}

	if(blockio(_WRITE,splitbuf.drive,(uint64_t) writeblock,blockbuf) == -1) return(-1); 		
}

/* fall through */
 
if(type == _DIR) return(create_subdirectory_entries(splitbuf.dirname,datastart,startblock,time,date,fattype,splitbuf.drive));	/* create subdirectory entries (. and ..) */

return(0);
}

/*
 * Create subdirectory entries (. and ..)
 * Internal use only
 *
 * In: dirname		Name of directory
       datastart	Start block of FAT data area
       startblock	Start block of subdirectory
 *
 * Returns: 0 on success or -1 on error.
 *
 */
size_t create_subdirectory_entries(char *dirname,size_t datastart,size_t startblock,TIME *time,TIME *date,size_t fattype,size_t drive) {
char *blockdirbuf;
char *blockdirptr;
size_t parentdir_startblock;
char *dirblockbuf;
char *dirblockptr;
size_t count;
DIRENT dirent;
BPB *bpb;
BLOCKDEVICE blockdevice;
size_t rootdir;
size_t start_of_data_area;

parentdir_startblock=fat_get_start_block(dirname);		/* get start block of parent directory */
		
dirblockbuf=kernelalloc(MAX_BLOCK_SIZE);	/* allocate buffer for subdirectory block */
if(dirblockbuf == NULL) return(-1);

if(getblockdevice(drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

bpb=blockdevice.superblock;				/* point to superblock data*/

/* get start of data area */
if(fattype == 12 || fattype == 16) {
	rootdir=((bpb->rootdirentries*FAT_ENTRY_SIZE)+(bpb->sectorsperblock-1))/bpb->sectorsize;
	start_of_data_area=rootdir+(bpb->reservedsectors+(bpb->numberoffats*bpb->sectorsperfat))-2; /* get start of data area */
}
else
{
	start_of_data_area=bpb->fat32rootdirstart+(bpb->reservedsectors+(bpb->numberoffats*bpb->fat32sectorsperfat))-2;
}

/* create first two entries in subdirectory ( . and ..) */

dirblockptr=dirblockbuf;

for(count=0;count<2;count++) {
	/* padded FAT entry filename */

	if(count == 0) {
		strncpy(dirent.filename,".          ",MAX_PATH);	/* this directory */
	}
	else
	{
		strncpy(dirent.filename,"..         ",MAX_PATH);	/* parent directory */
	}

	dirent.attribs=FAT_ATTRIB_DIRECTORY;
	dirent.reserved=NULL;
	dirent.create_time_fine_resolution=0;
	dirent.createtime=time;
	dirent.createdate=date;
	dirent.last_modified_time=dirent.createtime;
	dirent.last_modified_date=dirent.createdate;
	dirent.last_accessed_date=dirent.createdate;
	dirent.filesize=0;

	if(fattype == 32) { 
		if(count == 0) {					/* . */
			dirent.block_high_word=(startblock >> 16);			/* start block */
			dirent.block_low_word=(startblock & 0xffff);
		}
		else							/* .. */
		{
			dirent.block_high_word=(parentdir_startblock >> 16);			/* start block */
			dirent.block_low_word=(parentdir_startblock & 0xffff);
		}
	}

	if(fattype == 12 || fattype == 16) {
		dirent.block_high_word=0;

		if(count == 0) {					/* . */
			dirent.block_low_word=(startblock & 0xffff);
		}
		else							/* .. */
		{
			dirent.block_low_word=(parentdir_startblock & 0xffff);
		}	
	}

	memcpy(dirblockptr,&dirent,FAT_ENTRY_SIZE);		/* copy directory entry */

	dirblockptr += FAT_ENTRY_SIZE;
}
	
if(blockio(_WRITE,drive,(uint64_t) (startblock+datastart),dirblockbuf) == -1) {	/* write block */
	kernelfree(dirblockbuf);
	return(-1);
}

kernelfree(dirblockbuf);
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

size_t fat_unlink(char *filename) {
SPLITBUF splitbuf;
char *blockbuf;
FILERECORD buf;
FILERECORD subdirbuf;
char *blockptr;
char *delete_dir_path[MAX_PATH];

splitname(filename,&splitbuf);				/* split name */

if(findfirst(filename,&buf) == -1) {		/* get file entry */
	setlasterror(FILE_NOT_FOUND);
	return(-1);
}

if(buf.attribs & FAT_ATTRIB_DIRECTORY) {	/* if deleting directory, check if empty */
	ksnprintf(delete_dir_path,"%s\\*",subdirbuf.filename,MAX_PATH);
	
	if(findfirst(delete_dir_path,&subdirbuf) == 0) {		/* directory not empty */
		do {
			DEBUG_PRINT_HEX(subdirbuf.filename);
			if( (strncmp(subdirbuf.filename,".",MAX_PATH) != 0) && (strncmp(subdirbuf.filename,"..",MAX_PATH) != 0)) { /* non empty directorys have files other than */
				setlasterror(DIRECTORY_NOT_EMPTY);				 /* . and .. */
				return(-1);
			}
												   
		} while(findnext(delete_dir_path,&buf) != 0);
	}
}

if(fat_is_long_filename(splitbuf.filename)) {	/* long filename */
	return(fat_unlink_long_filename(filename,buf.findlastblock,buf.dirent));
}

blockbuf=kernelalloc(MAX_BLOCK_SIZE);			/* allocate block buffers */
if(blockbuf == NULL) {
	setlasterror(NO_MEM);
	return(-1);
}
			
if(blockio(_READ,splitbuf.drive,(uint64_t) buf.findlastblock,blockbuf) == -1) return(-1); /* read block */

blockptr=blockbuf+(buf.dirent*FAT_ENTRY_SIZE);			/* point to entry */

*blockptr=0xE5;						/* mark file as deleted */

if(blockio(_WRITE,splitbuf.drive,(uint64_t)buf.findlastblock,blockbuf) == -1) return(-1); /* write block */

setlasterror(NO_ERROR);  
return(NO_ERROR);				/* no error */
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

size_t fat_read(size_t handle,void *addr,size_t size) {
char *ioaddr;
char *iobuf;
size_t bytesdone;
size_t fattype;
BLOCKDEVICE blockdevice;
FILERECORD read_file_record;
size_t count;
uint64_t blockno;
size_t blockoffset;
size_t datastart;
size_t fatsize;
size_t rootdirsize;
size_t nextblock;
BPB *bpb;
char *blockbuf;
size_t read_size;
size_t pos_rounded_down;
size_t pos_rounded_up;

if(gethandle(handle,&read_file_record) == -1) {		/* bad handle */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

fattype=fat_detect_change(read_file_record.drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(read_file_record.drive,&blockdevice) == -1) {
	setlasterror(INVALID_HANDLE);
	return(NULL);
}

bpb=blockdevice.superblock;		/* point to superblock */

/* If current position has changed using seek(), find block number from FAT */

if((read_file_record.flags & FILE_POS_MOVED_BY_SEEK)) {
	for(count=0;count<(read_file_record.currentpos/(bpb->sectorsperblock*bpb->sectorsize));count++) {

		read_file_record.currentblock=fat_get_next_block(read_file_record.drive,read_file_record.currentblock);
	}
}

iobuf=kernelalloc(bpb->sectorsperblock*bpb->sectorsize);
if(iobuf == NULL) return(-1);

/* reading past end of file */ 

if((read_file_record.currentpos >= read_file_record.filesize) || \
   (read_file_record.currentblock == 0) || \
   (fattype == 12 && read_file_record.currentblock >= 0xff8) || \
   (fattype == 16 && read_file_record.currentblock >= 0xfff8) || \
   (fattype == 32 && read_file_record.currentblock >= 0xfffffff8)) {

	setlasterror(INPUT_PAST_END);
	return(-1);
}
	
/* until all data read */

read_size=size;

if(fattype == 12 || fattype == 16) {
	fatsize=(bpb->sectorsperfat*bpb->numberoffats);
	rootdirsize=((bpb->rootdirentries*FAT_ENTRY_SIZE)+bpb->sectorsperblock-1)/bpb->sectorsize;
	datastart=bpb->reservedsectors+fatsize+rootdirsize;
	blockno=((read_file_record.currentblock-2)*bpb->sectorsperblock)+datastart;
}

if(fattype == 32) {
	fatsize=(bpb->fat32sectorsperfat*bpb->numberoffats);
	datastart=bpb->reservedsectors+fatsize+bpb->fat32rootdirstart;
	blockno=((read_file_record.currentblock-2)*bpb->sectorsperblock)+datastart;
}

while(read_size > 0) {
	blockno=((read_file_record.currentblock-2)*bpb->sectorsperblock)+datastart;
	blockoffset=read_file_record.currentpos % (bpb->sectorsperblock*bpb->sectorsize);	/* distance inside the block */
	count=(bpb->sectorsperblock*bpb->sectorsize)-blockoffset;
	
	if(blockio(_READ,read_file_record.drive,blockno,iobuf) == -1) return(-1);		/* read block */
	
	/* adjust copy size */

	if(size < (bpb->sectorsperblock*bpb->sectorsize)) {
		count=size;
	}
	else
	{
		count=(bpb->sectorsperblock*bpb->sectorsize);
	}

	if((blockoffset+count) > (bpb->sectorsperblock*bpb->sectorsize)) count=(bpb->sectorsperblock*bpb->sectorsize)-(blockoffset+count);

	bytesdone += count;

	memcpy(addr,iobuf+blockoffset,count);			/* copy data to buffer */
	addr += count;      /* point to next address */

	/* if reading data that crosses a block boundary, find next block to read remainder of data */

	pos_rounded_down=read_file_record.currentpos - (read_file_record.currentpos % (bpb->sectorsperblock*bpb->sectorsize));
	pos_rounded_up=read_file_record.currentpos + (read_file_record.currentpos % (bpb->sectorsperblock*bpb->sectorsize));

	if((pos_rounded_down+count) > pos_rounded_up) {
	 	read_file_record.currentblock=fat_get_next_block(read_file_record.drive,read_file_record.currentblock);

		if((fattype == 12 && read_file_record.currentblock >= 0xff8) || 
		   (fattype == 16 && read_file_record.currentblock >= 0xfff8) || \
		   (fattype == 32 && read_file_record.currentblock >= 0xfffffff8)) break;
	}

	/* increment read size */
	read_size -= count; 
	read_file_record.currentpos += count;
	read_file_record.filesize += count;
	read_file_record.flags |= FILE_POS_MOVED_BY_SEEK;
}

kernelfree(iobuf);

setlasterror(NO_ERROR);
return(bytesdone);						/* return number of bytes */
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

size_t fat_write(size_t handle,void *addr,size_t size) {
uint64_t block;
char *ioaddr;
char *iobuf;
size_t bytesdone;
size_t fattype;
BLOCKDEVICE blockdevice;
TIME TIME;
FILERECORD write_file_record;
size_t count;
uint64_t blockno;
size_t blockoffset;
size_t datastart;
size_t fatsize;
size_t rootdirsize;
size_t last;
size_t nextblock;
size_t newblock;
BPB *bpb;
char *blockbuf;
DIRENT dirent;
uint64_t lastblock;
uint64_t sblock;
size_t write_size;
size_t is_first_written_block=FALSE;
SPLITBUF split;

gettime(&TIME);				/* get time and date */

if(gethandle(handle,&write_file_record) == -1) {	/* bad handle */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

fattype=fat_detect_change(write_file_record.drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(write_file_record.drive,&blockdevice) == -1) {
	setlasterror(INVALID_HANDLE);
	return(NULL);
}


bpb=blockdevice.superblock;		/* point to superblock */

/* If current position has changed using seek(), find block number from FAT */

if((write_file_record.flags & FILE_POS_MOVED_BY_SEEK)) {
	for(count=0;count<(write_file_record.currentpos/(bpb->sectorsperblock*bpb->sectorsize));count++) {

		write_file_record.currentblock=fat_get_next_block(write_file_record.drive,write_file_record.currentblock);
	}
}

/* find block to read from */

if(fattype == 12 || fattype == 16) {
	fatsize=(bpb->sectorsperfat*bpb->numberoffats);

	rootdirsize=((bpb->rootdirentries*FAT_ENTRY_SIZE)+bpb->sectorsperblock-1)/bpb->sectorsize;
	datastart=bpb->reservedsectors+fatsize+rootdirsize;

	blockno=((write_file_record.currentblock-2)*bpb->sectorsperblock)+datastart;
}

if(fattype == 32) {
	fatsize=(bpb->fat32sectorsperfat*bpb->numberoffats);
	datastart=bpb->reservedsectors+fatsize+bpb->fat32rootdirstart;
	blockno=((write_file_record.currentblock-2)*bpb->sectorsperblock)+datastart;
}


iobuf=kernelalloc(bpb->sectorsperblock*bpb->sectorsize);

if(iobuf == NULL) {
	kernelfree(iobuf);
	return(-1);
}

/* writing past end of file, add new blocks to FAT chain only needs to be done once per write past end */ 

if((write_file_record.currentpos >= write_file_record.filesize) || \
   (write_file_record.currentblock == 0) || \
   (fattype == 12 && write_file_record.currentblock >= 0xff8) || \
   (fattype == 16 && write_file_record.currentblock >= 0xfff8) || \
   (fattype == 32 && write_file_record.currentblock >= 0xfffffff8)) {
	/* update FAT */

	count=(size / (bpb->sectorsperblock*bpb->sectorsize));		/* number of blocks in FAT chain */
	if(size % (bpb->sectorsperblock*bpb->sectorsize)) count++;	/* round up */

	newblock=fat_find_free_block(write_file_record.drive);
	
	if(write_file_record.startblock == 0) {				/* first block in chain */
		sblock=newblock;

		write_file_record.startblock=newblock;	
		is_first_written_block=TRUE;				/* is first block written to empty file */
	}	

	/* need to add another block to FAT chain */

	if((write_file_record.startblock == 0) || (write_file_record.currentpos % (bpb->sectorsperblock*bpb->sectorsize) == 0)) {
		lastblock=write_file_record.currentblock;			/* save last block in FAT chain */
		write_file_record.currentblock=newblock;			/* new current block */
	
		do {
			if(fat_update_fat(write_file_record.drive,lastblock,(newblock >> 16),(newblock & 0xffff)) == -1) return(-1);
	
			lastblock=newblock;

			newblock=fat_find_free_block(write_file_record.drive);

			count--;
			} while(count != 0);

			/* end of fat chain */

			if(fattype == 12) {
				fat_update_fat(write_file_record.drive,newblock,0,0xff8);
			}
			else if(fattype == 16) {
				fat_update_fat(write_file_record.drive,newblock,0,0xfff8);
			}
			else if(fattype == 32) {
				fat_update_fat(write_file_record.drive,newblock,0xffff,0xfff8);
			}
	
		}
	}
	
/* until all data written */

write_size=size;

if(fattype == 12 || fattype == 16) {
	fatsize=(bpb->sectorsperfat*bpb->numberoffats);
	rootdirsize=((bpb->rootdirentries*FAT_ENTRY_SIZE)+bpb->sectorsperblock-1)/bpb->sectorsize;
	datastart=bpb->reservedsectors+fatsize+rootdirsize;
	blockno=((write_file_record.currentblock-2)*bpb->sectorsperblock)+datastart;
}

if(fattype == 32) {
	fatsize=(bpb->fat32sectorsperfat*bpb->numberoffats);
	datastart=bpb->reservedsectors+fatsize+bpb->fat32rootdirstart;
	blockno=((write_file_record.currentblock-2)*bpb->sectorsperblock)+datastart;
}

while(write_size > 0) {
	DEBUG_PRINT_HEX(write_file_record.currentpos);	
	DEBUG_PRINT_HEX(blockoffset);

	blockoffset=write_file_record.currentpos % (bpb->sectorsperblock*bpb->sectorsize);	/* distance inside the block */
	count=(bpb->sectorsperblock*bpb->sectorsize)-blockoffset;
	
	if(blockio(_READ,write_file_record.drive,blockno,iobuf) == -1) return(-1);		/* read block */

	/* adjust copy size */

	if((blockoffset+size) > (bpb->sectorsperblock*bpb->sectorsize)) {	/* write crosses data */
		count=(bpb->sectorsperblock*bpb->sectorsize)-size;	/* adjust size */
		/* the remainder of the data will be written on the next cycle */
	}
	else
	{
		count=size;
	}

	memcpy(iobuf+blockoffset,addr,count);			/* copy data to buffer */

	if(blockio(_WRITE,write_file_record.drive,blockno,iobuf) == -1) return(-1);		/* write block */

	addr += count;      /* point to next address */

	/* increment write size */
	write_size -= count; 
	write_file_record.currentpos += count;
	write_file_record.filesize += count;

	/* if writing data that crosses a block boundary, find next block to write remainder of data */

	if((blockoffset+size) > (bpb->sectorsperblock*bpb->sectorsize)) {
	 	write_file_record.currentblock=fat_get_next_block(write_file_record.drive,write_file_record.currentblock);
	}

}

/* update file size, time and date */

updatehandle(handle,&write_file_record);			/* update file handle data */

splitname(write_file_record.filename,&split);

/* update file's directory entry if it's a long filename */
if(fat_is_long_filename(split.filename)) {			/* long filename */
	memcpy(&write_file_record.create_time_date,&TIME,sizeof(TIME));
	memcpy(&write_file_record.last_written_time_date,&TIME,sizeof(TIME));
	memcpy(&write_file_record.last_accessed_time_date,&TIME,sizeof(TIME));

	write_file_record.filesize += size;

	if(is_first_written_block == TRUE) write_file_record.startblock=sblock; /* is first block in empty file */

	return(fat_update_long_filename(_FILE,write_file_record.drive,(uint64_t) write_file_record.findlastblock,write_file_record.dirent,&write_file_record));
}

/* update file's directory entry if it's a 8.3 filename */

blockbuf=kernelalloc(MAX_BLOCK_SIZE);			/* allocate block buffers */
if(blockbuf == NULL) return(-1);

if(blockio(_READ,write_file_record.drive,(uint64_t) write_file_record.findlastblock,blockbuf) == -1) return(-1); /* read block */

memcpy(&dirent,blockbuf+(write_file_record.dirent*FAT_ENTRY_SIZE),FAT_ENTRY_SIZE);		/* get FAT entry */

/* Update last modified time and date and last accessed date */
dirent.last_modified_time=(TIME.hours << 11) | (TIME.minutes << 5) | TIME.seconds;
dirent.last_modified_date=((TIME.year-1980) << 9) | (TIME.month << 5) | TIME.day;
dirent.last_accessed_date=((TIME.year-1980) << 9) | (TIME.month << 5) | TIME.day;

dirent.filesize += size;

/* set start block if it's the first block to be written */

if(is_first_written_block == TRUE) {
	if(fattype == 12 || fattype == 16) dirent.block_low_word=(sblock & 0xffff);

	if(fattype == 32) {
		dirent.block_high_word=(sblock << 16);
		dirent.block_low_word=(sblock & 0xffff);
	}
}

memcpy(blockbuf+(write_file_record.dirent*FAT_ENTRY_SIZE),&dirent,FAT_ENTRY_SIZE);		/* update FAT entry */

if(blockio(_WRITE,write_file_record.drive,write_file_record.findlastblock,blockbuf) == -1) return(-1); /* write block */

kernelfree(blockbuf);

setlasterror(NO_ERROR);
return(bytesdone);						/* return number of bytes */
}

/*
 * Set file attributes
 *
 * In:  filename	Filename
	attribs		Attributes. See fat.h for attributes.
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_chmod(char *filename,size_t attribs) {
SPLITBUF splitbuf;
DIRENT dirent;
FILERECORD buf;
char *blockbuf;
size_t type;

if((attribs & FAT_ATTRIB_DIRECTORY)) {		/* can't set directory bit */
	setlasterror(ACCESS_DENIED);  
	return(-1);
}

if((attribs & FAT_ATTRIB_VOLUME_LABEL)) {	/* can't set volume label bit */
	setlasterror(ACCESS_DENIED);  
	return(-1);
}

splitname(filename,&splitbuf);				/* split name */

if(findfirst(filename,&buf) == -1) return(-1);		/* get file entry */

if(fat_is_long_filename(splitbuf.filename)) {			/* long filename */
	buf.attribs=attribs;

	if(buf.attribs & FAT_ATTRIB_DIRECTORY) {		/* renaming directory */
		type=_DIR;
	}
	else
	{
		type=_FILE;
	}

	return(fat_update_long_filename(type,buf.drive,(uint64_t) buf.findlastblock,buf.dirent,&buf));

}

blockbuf=kernelalloc(MAX_BLOCK_SIZE);			/* allocate block buffers */
if(blockbuf == NULL) return(-1);

if(blockio(_READ,splitbuf.drive,(uint64_t) buf.findlastblock,blockbuf) == -1) return(-1); /* read block */

memcpy(&dirent,blockbuf+(buf.dirent*FAT_ENTRY_SIZE),FAT_ENTRY_SIZE);		/* get FAT entry */

dirent.attribs=(uint8_t) attribs;			/* set attributes */

memcpy(blockbuf+(buf.dirent*FAT_ENTRY_SIZE),&dirent,FAT_ENTRY_SIZE);		/* update FAT entry */

if(blockio(_WRITE,splitbuf.drive,buf.findlastblock,blockbuf) == -1) return(-1); /* write block */

kernelfree(blockbuf);

setlasterror(NO_ERROR);  
return(NO_ERROR);				/* no error */
}

/*
 * Set file time and date
 *
 * In:  filename		File to set file and date of
	create_time_date	Struct with create time and date information
	last_modified_time_date	Struct with last modified time and date information
	last_accessed_time_date	Struct with last accessed time and date information

 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t fat_set_file_time_date(char *filename,TIME *create_time_date,TIME *last_modified_time_date,TIME *last_accessed_time_date) {
SPLITBUF splitbuf;
DIRENT dirent;
FILERECORD buf;
char *blockbuf;
size_t type;

splitname(filename,&splitbuf);				/* split name */

if(findfirst(filename,&buf) == -1) return(-1);		/* get file entry */

if(fat_is_long_filename(splitbuf.filename)) {			/* long filename */
	memcpy(&buf.create_time_date,create_time_date,sizeof(TIME));
	memcpy(&buf.last_written_time_date,last_modified_time_date,sizeof(TIME));
	memcpy(&buf.last_accessed_time_date,last_accessed_time_date,sizeof(TIME));

	if(buf.attribs & FAT_ATTRIB_DIRECTORY) {		/* renaming directory */
		type=_DIR;
	}
	else
	{
		type=_FILE;
	}

	return(fat_update_long_filename(type,buf.drive,(uint64_t) buf.findlastblock,buf.dirent,&buf));
}

blockbuf=kernelalloc(MAX_BLOCK_SIZE);			/* allocate block buffers */
if(blockbuf == NULL) return(-1);

if(blockio(_READ,splitbuf.drive,(uint64_t) buf.findlastblock,blockbuf) == -1) return(-1); /* read block */

memcpy(&dirent,blockbuf+(buf.dirent*FAT_ENTRY_SIZE),FAT_ENTRY_SIZE);		/* get FAT entry */

/* Update file time and date */
dirent.createtime=(create_time_date->hours << 11) | (create_time_date->minutes << 5) | create_time_date->seconds;
dirent.createdate=((create_time_date->year-1980) << 9) | (create_time_date->month << 5) | create_time_date->day;
dirent.last_modified_time=(last_modified_time_date->hours << 11) | (last_modified_time_date->minutes << 5) | last_modified_time_date->seconds;
dirent.last_modified_date=((last_modified_time_date->year-1980) << 9) | (last_modified_time_date->month << 5) | last_modified_time_date->day;
dirent.last_accessed_date=((last_accessed_time_date->year-1980) << 9) | (last_accessed_time_date->month << 5) | last_accessed_time_date->day;

memcpy(blockbuf+(buf.dirent*FAT_ENTRY_SIZE),&dirent,FAT_ENTRY_SIZE);		/* update FAT entry */

if(blockio(_WRITE,splitbuf.drive,buf.findlastblock,blockbuf) == -1) return(-1); /* write block */

kernelfree(blockbuf);

setlasterror(NO_ERROR);  
return(NO_ERROR);				/* no error */
}


/*
 * Find free block
 *
 * In:  drive		Drive
 *
 * Returns: -1 on error, block number on success
 *
 */

size_t fat_find_free_block(size_t drive) {
uint64_t rb;
uint64_t count;
size_t entry_offset;
uint16_t entry;
size_t entryno;
uint32_t *longptr;
size_t fattype;
uint16_t shortentry;
uint16_t *shortptr;
uint32_t longentry;
size_t fatsize;
uint32_t startblock;
BLOCKDEVICE blockdevice;
BPB *bpb;
uint8_t *buf;
uint64_t blockcount;
size_t entrycount;
size_t whichentry;

fattype=fat_detect_change(drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(drive,&blockdevice) == -1) {		/* get block device struct */
	setlasterror(INVALID_DISK);
	return(-1);
}		

bpb=blockdevice.superblock;		/* point to data */
	
buf=kernelalloc(bpb->sectorsperblock*bpb->sectorsize);
if(buf == NULL) return(-1);

blockcount=0;

/* until free block found */ 

entrycount=0;
whichentry=2;

if(fattype == 12) {				/* fat 12 */
	fatsize=bpb->sectorsperfat;
	rb=bpb->reservedsectors;

//	kprintf_direct("Find free block\n");

	while(rb < (fatsize*bpb->sectorsperblock)) {

		if(rb == bpb->reservedsectors) {			/* skip first three entries in first FAT sector */
			entrycount=3;
		}
		else
		{
			entrycount=0;
		}

		b=buf;

		for(count=rb;count<rb+2;count++) {
			if(blockio(_READ,drive,count,b) == -1) {			/* read block */ 
				kernelfree(buf);
				return(-1);
			}

			b += bpb->sectorsize;
		}
			
		rb += 2;	

		shortptr=buf;

		while(entrycount < (bpb->sectorsperblock*bpb->sectorsize)*2) {
			// get FAT entries

			entryno=(entrycount+(entrycount/2));
			entry_offset=(entryno % (bpb->sectorsperblock*bpb->sectorsize));	/* offset into fat */
			entry=*(uint16_t*)&buf[entry_offset];		/* get entry */

			if(entrycount & 1) {					/* block is odd */
				entry=entry >> 4;
			}
			else
			{
				entry=entry & 0xfff;

			}

			if(entry == 0) return(whichentry-2);		/* found free entry */

			whichentry++;
			entrycount++;

	      }
	}
}

if(fattype == 16) {				/* fat 16 */
	fatsize=bpb->sectorsperfat;
	rb=bpb->reservedsectors;

	for(count=rb;count<rb+(fatsize*bpb->sectorsperblock);count++) {  
		if(blockio(_READ,drive,count,buf) == -1) {			/* read block */ 
			kernelfree(buf);
			 return(-1);
		}

		for(entry_offset=0;entry_offset<(bpb->sectorsperblock*bpb->sectorsize);entry_offset++) {
			if(buf[entry_offset] == 0) {
				setlasterror(NO_ERROR);

				return(blockcount);
			}

			blockcount++;
		}
	}
}

if(fattype == 32) {
	for(count=bpb->reservedsectors;count<bpb->fat32sectorsperfat;count++) {  
		if(blockio(_READ,drive,count,buf) == -1) {			/* read block */ 
			kernelfree(buf);
			return(-1);
		}

		longptr=buf;

		for(entry_offset=0;entry_offset<(bpb->sectorsperblock*bpb->sectorsize);entry_offset++) {
					       
			if(longptr[entry_offset] == 0) {
				setlasterror(NO_ERROR);
				return(blockcount);
			}

			blockcount++;
		}

	}
}

kernelfree(buf);	
setlasterror(END_OF_DIRECTORY);

return(-1);
}

/*
 * Get start block of file
 *
 * In:  name	Filename
 *
 * Returns: -1 on error, start block on success
 *
 */

size_t fat_get_start_block(char *name) {
char *token_buffer[MAX_PATH];
uint64_t rb;
size_t count;
SPLITBUF *splitbuf;
char *fullpath[MAX_PATH];
size_t fattype;
size_t tc;
BLOCKDEVICE blockdevice;
char *b;
char *blockbuf;
char *blockptr;
char *file[11];
size_t c;
uint64_t start_of_data_area=0;
uint64_t rootdir_size=0;
DIRENT directory_entry;
FILERECORD *lfnbuf=NULL;
size_t lfncount;
BPB *bpb;
size_t datastart;
size_t fatsize;
size_t rootdirsize;

splitbuf=kernelalloc(sizeof(SPLITBUF));
if(splitbuf == NULL) return(-1);

lfnbuf=kernelalloc(sizeof(FILERECORD));	/* allocate split buffer */
if(lfnbuf == NULL) return(-1);

getfullpath(name,fullpath);		/* get full path */	

memset(splitbuf,0,sizeof(SPLITBUF));

splitname(fullpath,splitbuf);

fattype=fat_detect_change(splitbuf->drive);
if(fattype == -1) {
	kernelfree(splitbuf);
	kernelfree(lfnbuf);
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(splitbuf->drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(NULL);
}

bpb=blockdevice.superblock;		/* point to data */


blockbuf=kernelalloc(bpb->sectorsperblock*bpb->sectorsize);			/* allocate block buffer */
if(blockbuf == NULL) {
		kernelfree(lfnbuf);
		kernelfree(splitbuf);	
		return(-1);
}

/* read first block of directory to buffer
 *
 *  for each token in path:
 *
 *   find start block of token
 *   until block=0xfff (fat 12) or 0xffff (fat 16) or 0xfffffffff (fat 32) 
 *   read block
 *   search entries in block for name, return block number from fat entry if found
 *   get next block in directory from fat using fat_get_next_block
 *
 *  return ERROR if at end of directory and token not found
 *
 * return block number if at end of name  
 */

c=0;
	
old=NULL;

c=get_filename_token(fullpath,token_buffer);					/* skip first token*/
tc=get_filename_token_count(fullpath);
tc--;

count=1;

if(fattype == 12 || fattype == 16) rb=bpb->reservedsectors+(bpb->sectorsperfat*bpb->numberoffats);  /* get start of root directory */
if(fattype == 32) rb=bpb->fat32rootdirstart;  /* get start of root directory */

if(strncmp(splitbuf->dirname,"\\",MAX_PATH) == 0) {						           /* root directory */
	if(strlen(splitbuf->filename) == 0) { 
		kernelfree(lfnbuf);
		kernelfree(blockbuf);
		kernelfree(splitbuf);
		return(rb);
	}
}

if(fattype == 12 || fattype == 16) {
	fatsize=(bpb->sectorsperfat*bpb->numberoffats);
	rootdirsize=((bpb->rootdirentries*FAT_ENTRY_SIZE)+bpb->sectorsperblock-1)/bpb->sectorsize;
	datastart=bpb->reservedsectors+fatsize+rootdirsize;
}

if(fattype == 32) {
	fatsize=(bpb->fat32sectorsperfat*bpb->numberoffats);
	datastart=bpb->reservedsectors+fatsize+bpb->fat32rootdirstart;
}

while(c != -1) {
	memset(token_buffer,0,MAX_PATH);
	c=get_filename_token_count(fullpath);
	get_filename_token(fullpath,token_buffer);					/* tokenize filename */

	tc--;	

	do {
		if(blockio(_READ,splitbuf->drive,(uint64_t) rb+start_of_data_area,blockbuf) == -1) {			/* read block for entry */
			kernelfree(splitbuf);
			kernelfree(blockbuf);
		     	kernelfree(lfnbuf);
			return(-1);
		}

		blockptr=blockbuf;

		for(count=0;count<((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE);count++) {
			memcpy(&directory_entry,blockptr,FAT_ENTRY_SIZE);

			fat_entry_to_filename(directory_entry.filename,file);			/* convert filename */

			touppercase(token_buffer);			/* convert to uppercase */
		
			if(wildcard(token_buffer,file) == 0) { 		/* if short entry filename found */         
				if(fattype == 12 || fattype == 16) rb=directory_entry.block_low_word;				/* get next block */
				if(fattype == 32) rb=(uint64_t) ((directory_entry.block_high_word << 16)+directory_entry.block_low_word); 	/* get next block */

				if(tc == 0) {				/* last token in path */
					setlasterror(NO_ERROR);
		
					kernelfree(lfnbuf);
				  	kernelfree(blockbuf);
				  	kernelfree(splitbuf);
			
					return(rb);				/* at end, return block */
				 }

					rb += (datastart-2);			/* add start of data area */
					break;
			}

			if(*blockptr == 0) {				/* end of directory */     
				kernelfree(lfnbuf);
				kernelfree(splitbuf);
				kernelfree(blockbuf);

				setlasterror(PATH_NOT_FOUND);
				return(-1);
			}

			blockptr=blockptr+FAT_ENTRY_SIZE;					/* point to next entry */
		}

		rb=fat_get_next_block(splitbuf->drive,rb);

	} while((fattype == 12 && rb <= 0xff8) || (fattype == 16 && rb <= 0xfff8) || (fattype == 32 && rb <= 0xfffffff8));

}


kernelfree(lfnbuf);
kernelfree(blockbuf);
kernelfree(splitbuf);

setlasterror(PATH_NOT_FOUND);
return(-1);
}

/*
 * Get next block in FAT chain
 *
 * In:  drive	Drive
	block	Block number
 *
 * Returns: -1 on error, block number on success
 *
 */


size_t fat_get_next_block(size_t drive,uint64_t block) {
uint8_t *buf;
size_t fattype;
uint64_t blockno;
uint16_t entry;
uint32_t entry_dword;
size_t entryno;
uint32_t entry_offset;
BLOCKDEVICE blockdevice;
BPB *bpb;

fattype=fat_detect_change(drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
}

buf=kernelalloc(MAX_BLOCK_SIZE);
if(buf == NULL) return(-1);

bpb=blockdevice.superblock;		/* point to data */

if(fattype == 12) {					/* fat12 */
	entryno=(block+(block/2));
	blockno=(bpb->reservedsectors*bpb->sectorsperblock)+(entryno / (bpb->sectorsperblock*bpb->sectorsize));
	entry_offset=(entryno % (bpb->sectorsperblock*bpb->sectorsize));	/* offset into fat */

	if(blockio(_READ,drive,blockno,buf) == -1) { 	/* read block */
		kernelfree(buf);
		return(-1);
	}

	blockio(_READ,drive,blockno+1,buf+(bpb->sectorsperblock*bpb->sectorsize)); 	/* read block+1 */
	
	entry=*(uint16_t*)&buf[entry_offset];		/* get entry */

	if(block & 1) {					/* block is odd */
		entry=entry >> 4;
	}
	else
	{
		entry=entry & 0xfff;
	}
		
	kernelfree(buf);
	return(entry);
}


if(fattype == 16) {					/* fat16 */

	entryno=block*2;				
	blockno=(bpb->reservedsectors*bpb->sectorsperblock)+(entryno / (bpb->sectorsperblock*bpb->sectorsize));
	entry_offset=entryno % (bpb->sectorsperblock*bpb->sectorsize);	/* offset into fat */

	if(blockio(_READ,drive,blockno,buf) == -1) { 	/* read block */
		kernelfree(buf);
		return(-1);
	}

		entry=*(uint16_t*)&buf[entry_offset];		/* get entry */

		kernelfree(buf);
		return(entry);
	}

	if(fattype == 32) {					/* fat32 */

		entryno=block*4;				
		blockno=(bpb->reservedsectors*bpb->sectorsperblock)+(entryno / (bpb->sectorsperblock*bpb->sectorsize));
		entry_offset=entryno % (bpb->sectorsperblock*bpb->sectorsize);	/* offset into fat */

		if(blockio(_READ,drive,blockno,buf) == -1) {			/* read block */
			kernelfree(buf);
			return(-1);
		}

		entry_dword=*(buf+entry_offset);				/* get entry */

		entry_dword=entry_dword+(bpb->fat32sectorsperfat*bpb->numberoffats)+((bpb->rootdirentries*FAT_ENTRY_SIZE)/bpb->sectorsperblock)-bpb->reservedsectors;

		kernelfree(buf);
		return(entry_dword & 0x0FFFFFFF);
	}

}
/*
 * Update FAT entry
 *
 * In:  drive		Drive
	block		Block number of FAT entry
	block_high_word High word of new FAT entry
	block_low_word	Low word of new FAT entry
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_update_fat(size_t drive,uint64_t block,uint16_t block_high_word,uint16_t block_low_word) {
uint8_t buf[8196];
uint16_t *b;
size_t entriesperblock;
size_t fattype;
uint64_t blockno;
size_t count;
uint16_t entry;
uint16_t e;
size_t entryno;
uint32_t entry_offset;
BPB *bpb;
BLOCKDEVICE blockdevice;
uint8_t firstbyte;
uint8_t secondbyte;
uint8_t thirdbyte;
size_t fatstart;

fattype=fat_detect_change(drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(drive,&blockdevice) == -1) {
		setlasterror(INVALID_DISK);
		return(NULL);
}

bpb=blockdevice.superblock;		/* point to data */

if(fattype == 12) {					/* fat12 */
	entryno=block+(block/2);
	blockno=(uint64_t) (bpb->reservedsectors*bpb->sectorsperblock)+(entryno / (bpb->sectorsperblock*bpb->sectorsize));
	entry_offset=(entryno % (bpb->sectorsperblock*bpb->sectorsize));	/* offset into fat */

	if(blockio(_READ,drive,blockno,buf) == -1) return(-1); 	/* read block */
	blockio(_READ,drive,blockno+1,buf+(bpb->sectorsperblock*bpb->sectorsize)); 	/* read block	+1 */

	firstbyte=buf[entry_offset];
	secondbyte=buf[entry_offset+1];
	thirdbyte=buf[entry_offset+2];

	// 23 61 45

	if(block & 1) {					/* block is odd */
		secondbyte=(secondbyte & 0x0f)+(uint8_t) (block_low_word & 0xf0);
		thirdbyte=((block_low_word & 0xff80) >> 8);
	}
	else
	{
		firstbyte=(uint8_t) (block_low_word & 0xff);
		secondbyte=(secondbyte & 0x0f)+(uint8_t) ((block_low_word & 0xf00) >> 4);
	}

	buf[entry_offset]=firstbyte;
	buf[entry_offset+1]=secondbyte;
	buf[entry_offset+2]=thirdbyte;
}


if(fattype == 16) {					/* fat16 */
	entryno=block*2;				
	blockno=(bpb->reservedsectors*bpb->sectorsperblock)+(entryno / (bpb->sectorsperblock*bpb->sectorsize));
	entry_offset=entryno % (bpb->sectorsperblock*bpb->sectorsize);	/* offset into fat */

	if(blockio(_READ,drive,blockno,buf) == -1) return(-1); 	/* read block */
	blockio(_READ,drive,blockno+1,buf+(bpb->sectorsperblock*bpb->sectorsize)); 	/* read block	+1 */

	*(uint16_t*)&buf[entry_offset]=block_low_word;
}

if(fattype == 32) {					/* fat32 */
	entryno=block*4;				
	blockno=(bpb->reservedsectors*bpb->sectorsperblock)+(entryno / (bpb->sectorsperblock*bpb->sectorsize));
	entry_offset=entryno % (bpb->sectorsperblock*bpb->sectorsize);	/* offset into fat */

	if(blockio(_READ,drive,blockno,buf) == -1) return(-1); 	/* read block */
	blockio(_READ,drive,blockno+1,buf+(bpb->sectorsperblock*bpb->sectorsize)); 	/* read block	+1 */

	*(uint32_t*)&buf[entry_offset]=(block_high_word << 16)+block_low_word;
}

/* write fats */

for(count=0;count<bpb->numberoffats;count++) {
	if(blockio(_WRITE,drive,blockno,buf) == -1) return(-1);

	blockno=blockno+(bpb->sectorsperfat*bpb->sectorsperblock);
}

setlasterror(NO_ERROR);
return(NO_ERROR);
}

/*
 * Detect FAT drive change
 *
 * In:  drive	Drive
 *
 * Returns: -1 on error, FAT type on success
 *
 */


size_t fat_detect_change(size_t drive) {
size_t  count;
size_t fattype;
char *bootbuf;
BPB bpb;
BLOCKDEVICE blockdevice;

getblockdevice(drive,&blockdevice);		/* get device */ 

bootbuf=kernelalloc(MAX_BLOCK_SIZE);				/* allocate buffer */
if(bootbuf == NULL) return(-1); 
	
if(blockio(_READ,drive,(uint64_t) 0,bootbuf) == -1) {				/* get bios parameter block */
	kernelfree(bootbuf);
	return(-1); 
}

memcpy(&bpb,bootbuf,54);		/* copy bpb */

blockdevice.sectorsperblock=bpb.sectorsperblock;

if(blockdevice.numberofsectors/blockdevice.sectorsperblock < 4095) fattype=12;		/* get fat type */
if(blockdevice.numberofsectors/blockdevice.sectorsperblock >= 4095 && blockdevice.numberofsectors/bpb.sectorsperblock < 268435445) fattype=16;
if(blockdevice.numberofsectors/blockdevice.sectorsperblock/bpb.sectorsperblock >= 268435445) fattype=32;
if(fattype == 32) memcpy((void *) &bpb.fat32sectorsperfat, (void *) bootbuf+54,62-36);		/* fat32 */

kernelfree(bootbuf);

if(blockdevice.superblock == NULL) {
	blockdevice.superblock=kernelalloc(sizeof(BPB));	/* allocate buffer */
	if(blockdevice.superblock == NULL) {
		kernelfree(bootbuf);
		return(-1);
	}
}

memcpy(blockdevice.superblock,&bpb,sizeof(BPB));		/* update BPB */

update_block_device(drive,&blockdevice);			/* update block device information */

return(fattype);
}

/*
 * Convert filename (only filename) to FAT entry name
 * Internal use only
 *
 * In:  name	Filename
		outname Buffer to hold converted name
 *
 * Returns: nothing
 *
 */

size_t fat_convert_filename(char *filename,char *outname) {
char *s;
char *d;
size_t count;
char *temp[MAX_PATH];

strncpy(temp,filename,MAX_PATH);				/* copy filename */

s=temp;
d=outname;

memset(outname,' ',11);				/* fill with spaces */

for(count=0;count<strlen(filename);count++) {
	if(*s >= 'a' && *s <= 'z') *s -= 32;			/* convert to uppercase */

	s++;
}

s=temp;

for(count=0;count<strlen(filename);count++) { 
	*d=*s++;

	if(*d == '.') {					/* copy extension */
		*d=' ';
		memcpy((void *) outname+8,(void *) s,3);
	
		return;
	}

	d++;
}

return;
}


/*
 * Read long filename entry into of buffer starting from block, internal use only
 *
 * In:  drive	Drive
	block	Block number of long filename
	entryno FAT entry number of long filename
	n	FILERECORD to hold information about long filename
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_read_long_filename(size_t drive,uint64_t block,size_t entryno,FILERECORD *n) {
char *s;
size_t entrycount=0;
size_t ec;
char *d;
DIRENT dirent;
char *buf[MAX_PATH];
size_t fattype;
uint64_t blockcount;
size_t count;
char *blockbuf;
char *blockptr;
uint8_t l;
size_t lfncount=0;
char *b;
LFN_ENTRY *lfn;
BPB *bpb;
BLOCKDEVICE blockdevice;

blockcount=block;

fattype=fat_detect_change(drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(NULL);
}

bpb=blockdevice.superblock;		/* get bpb */

memset(buf,0,MAX_PATH);			/* clear buffer */
memset(n->filename,0,MAX_PATH);			/* clear buffer */

blockbuf=kernelalloc((bpb->sectorsperblock*bpb->sectorsize));
if(blockbuf == NULL) return(-1);

lfn=kernelalloc((bpb->sectorsperblock*bpb->sectorsize));
if(lfn == NULL) {
	kernelfree(blockbuf);
	return(-1);
}

blockptr=blockbuf+entryno*FAT_ENTRY_SIZE;		/* point to first entry */

d=buf;
d += MAX_PATH;

lfncount=0;

while((fattype == 12 && blockcount <= 0xff8) || (fattype == 16 && blockcount <= 0xfff8) || (fattype == 32 && blockcount <= 0xfffffff8)) {	/* until end of chain */

	if(blockio(_READ,drive,block,blockbuf) == -1) {	/* read block for entry */
		kernelfree(blockbuf);
		kernelfree(lfn);
		return(-1);
	}

	for(count=0;count<((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE)-(entryno*FAT_ENTRY_SIZE);count++) {
		n->dirent=count;
					
		memset(lfn,0,sizeof(LFN_ENTRY));
		memcpy(lfn,(void *) blockptr,FAT_ENTRY_SIZE);

		if(lfn->sequence == 0) {
			kernelfree(blockbuf);
			kernelfree(lfn);
			return(lfncount);		/* no more lfn */
		}
		
		blockptr=blockptr+FAT_ENTRY_SIZE;

	        if(lfn->sequence == 0xE5) continue;			/* skip deleted entries */
		
		entrycount=lfn->sequence;				/* get number of entries */
		if(entrycount >= 0x40) entrycount=entrycount-0x40;
		
		/* Copy filename from long filename entry in reverse order */

		if(strlen_unicode(lfn->lasttwo_chars,2) > 0) {  
			d=d-strlen_unicode(lfn->lasttwo_chars,2);

			s=lfn->lasttwo_chars;
			b=d;
	
			for(ec=0;ec<strlen_unicode(lfn->lasttwo_chars,2);ec++) {
				if((uint8_t) *s == 0xFF) break;

				*b++=*s;
				s=s+2;
			}   
		}

		if(strlen_unicode(lfn->nextsix_chars,6) > 0) {
			d=d-strlen_unicode(lfn->nextsix_chars,6);
			s=lfn->nextsix_chars;

			b=d;

			for(ec=0;ec<strlen_unicode(lfn->nextsix_chars,6);ec++) {			
				if((uint8_t) *s == 0xFF) break;

				*b++=*s;
				s=s+2;
			}  
		}

		if(strlen_unicode(lfn->firstfive_chars,5) > 0) {
			d=d-strlen_unicode(lfn->firstfive_chars,5);
			s=lfn->firstfive_chars;
					  
			b=d;

			for(ec=0;ec<strlen_unicode(lfn->firstfive_chars,5);ec++) {			
				if((uint8_t) *s == 0xFF) break;

				*b++=*s;
				s=s+2;	
			} 
		}

		if((lfn->sequence == 1) || (lfn->sequence == 0x41)) {		/* at end of filename copy filename */
			lfncount++;			/* skip short entry */

			strncpy(n->filename,d,MAX_PATH);			/* copy filename */

			/* at end, so will point to short entry */

			memcpy(&dirent,blockptr,FAT_ENTRY_SIZE);			/* get directory entry */

			/* fill FILERECORD structure with file information */
			n->attribs=dirent.attribs;

			n->create_time_date.hours=(dirent.createtime & 0xf800) >> 11;
			n->create_time_date.minutes=(dirent.createtime  & 0x7e0) >> 6;
			n->create_time_date.seconds=(dirent.createtime & 0x1f);
			n->create_time_date.year=((dirent.createdate & 0xfe00) >> 9)+1980;
			n->create_time_date.month=(dirent.createdate & 0x1e0) >> 5;
			n->create_time_date.day=(dirent.createdate & 0x1f);

			n->last_written_time_date.hours=(dirent.last_modified_time & 0xf800) >> 11;
			n->last_written_time_date.minutes=(dirent.last_modified_time  & 0x7e0) >> 6;
			n->last_written_time_date.seconds=(dirent.last_modified_time & 0x1f);
			n->last_written_time_date.year=((dirent.last_modified_date & 0xfe00) >> 9)+1980;
			n->last_written_time_date.month=(dirent.last_modified_date & 0x1e0) >> 5;
			n->last_written_time_date.day=(dirent.last_modified_date & 0x1f);

			n->last_accessed_time_date.year=((dirent.last_accessed_date & 0xfe00) >> 9)+1980;
			n->last_accessed_time_date.month=(dirent.last_accessed_date & 0x1e0) >> 5;
			n->last_accessed_time_date.day=(dirent.last_accessed_date & 0x1f);

			n->filesize=dirent.filesize;
			n->startblock=(dirent.block_high_word << 16)+dirent.block_low_word;

			n->findentry=entryno;
			n->findlastblock=block;

			kernelfree(blockbuf);
			kernelfree(lfn);

			return(lfncount+1);
		}

		lfncount++;
	}

	blockcount++;
}

kernelfree(blockbuf);
kernelfree(lfn);

return(-1);
}
		
/*
 * Update long filename, internal use only
 *
 * In:  drive	Drive
	block	Block number of long filename
	entryno FAT entry number of long filename
	n	FILERECORD to hold information about long filename
	newname	New name to use
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_update_long_filename(size_t type,size_t drive,uint64_t block,size_t entryno,FILERECORD *new) {
BLOCKDEVICE blockdevice;
char *buf[MAX_PATH];
size_t entry_count;
size_t first_free_entry;
size_t free_entry_count;
size_t fattype;
void *blockbuf;
char *fatname[11];
FILERECORD lfn;
char *blockptr;
SPLITBUF splitbuf;
char *fullpath[MAX_PATH];
size_t flen;
size_t firstblock;
size_t blockcount;
size_t freeblock;
BPB *bpb;

if(fat_read_long_filename(drive,(uint64_t) block,entryno,&lfn) == -1) return(-1);		/* read long filename */

/*
 * new long filename is shorter or same length as old long filename
 *
 */

if(strlen(new->filename) <= strlen(lfn.filename)) {
	if(fat_unlink_long_filename(lfn.filename,(uint64_t) block,entryno) == -1) return(-1);		/* delete filename and recreate it */
	if(fat_create_long_filename(type,new,(uint64_t) block,entryno) == -1) return(-1);

	return(0);
}
	
/* Otherwise new long filename is longer than old long filename */

/* attempt to create free entries in free directory entries, if none could be found, create entries at end of directory */

fattype=fat_detect_change(drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(NULL);
}

blockbuf=kernelalloc((bpb->sectorsperblock*bpb->sectorsize));		/* allocate block buffer */
if(blockbuf == NULL) return(-1);

flen=strlen(new->filename)/13;				/* get number of 13 byte sections */
if(strlen(new->filename) % 13 > 0) flen++;

bpb=blockdevice.superblock;		/* get bpb */

getfullpath(new->filename,fullpath);			/* get full path */
splitname(fullpath,&splitbuf);

firstblock=fat_get_start_block(splitbuf.dirname);

block=firstblock;					/* save block number */

while((fattype == 12 && block != 0xfff) || (fattype == 16 && block !=0xffff) || (fattype == 32 && block !=0xffffffff)) {	/* until end of chain */

	if(blockio(_READ,splitbuf.drive,block,blockbuf) == -1) {	/* read block for entry */
		kernelfree(blockbuf);
		return(-1);
	}

	free_entry_count=0;
	first_free_entry=0;

	blockptr=blockbuf;

	for(entry_count=0;entry_count<((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE);entry_count++) {

		DEBUG_PRINT_HEX(blockptr);

		if(((uint8_t) *blockptr == 0) || ((uint8_t)  *blockptr == 0xE5)) { /* deleted file or end of directory */
			free_entry_count++;

			if((uint8_t) *blockptr == 0) break;		/* at end of directory and dont' need so search for more entries */
		}
		else
		{
			first_free_entry=entry_count+1;
			free_entry_count=0;		/* entries must be contingous */
		}


		if(free_entry_count > flen) {			/* found free entries */
			fat_unlink_long_filename(new->filename,(uint64_t) block,entryno);	/* delete filename and recreate it */

			memcpy(&lfn,new,sizeof(FILERECORD));					/* copy file information */

			lfn.findlastblock=block;
			lfn.dirent=first_free_entry;
			
			fat_create_long_filename(type,&lfn,(uint64_t) block,first_free_entry);

			kernelfree(blockbuf);
			return(0);
		}
		
		blockptr=blockptr+FAT_ENTRY_SIZE;		/* point to next entry */
	}

	block=fat_get_next_block(splitbuf.drive,block);
	blockptr=blockbuf;
}

/* no free entries could be found, create entries at end of directory */

/* root directory is fixed size for fat12 and fat16 */

if(fattype == 12 || fattype == 16) {
	if(strncmp(splitbuf.dirname,"\\",MAX_PATH) == 0) {
		setlasterror(DIRECTORY_FULL);
		return(-1);
	}
}

block=firstblock;

while((fattype == 12 && block != 0xfff) || (fattype == 16 && block !=0xffff) || (fattype == 32 && block !=0xffffffff)) {	/* until end of chain */
	b=fat_get_next_block(splitbuf.drive,block);			/* get next block */
				
	if((fattype == 12 && b != 0xfff) || (fattype == 16 && b !=0xffff) || (fattype == 32 && b !=0xffffffff)) break;

	block=b;
}

if(flen*FAT_ENTRY_SIZE < (bpb->sectorsperblock*bpb->sectorsize)) {		/* one block */
	blockcount=1;
}
else
{
	blockcount=(flen*FAT_ENTRY_SIZE)/(bpb->sectorsperblock*bpb->sectorsize);	/* more than one block */
}

/* create free blocks at end of directory */

freeblock=fat_find_free_block(drive);			/* find free block */
if(freeblock == -1) {
	kernelfree(blockbuf);
	return(-1);
}
			
if(fat_update_fat(drive,block,(freeblock >> 8),(freeblock & 0xffff)) == -1) {
	kernelfree(blockbuf);
	return(-1);
}

while(blockcount > 0) {
	freeblock=fat_find_free_block(drive);			/* add remainder of blocks */

	if(freeblock == -1) {
		kernelfree(blockbuf);
		return(-1);
	}

	if(fat_update_fat(drive,block,(freeblock >> 8),(freeblock & 0xffff)) == -1) {
		kernelfree(blockbuf);
		return(-1);
	}

blockcount--;
}


fat_unlink_long_filename(lfn.filename,block,entryno);			/* delete filename and recreate it */

memcpy(&lfn,new,sizeof(FILERECORD));					/* copy file information */

fat_create_long_filename(type,&lfn,block,entryno);
			
kernelfree(blockbuf);

return(NO_ERROR);
}

/*
 * Create short FAT filename (FILENA~1.EXT) from long filename
 *
 * In:  filename	Filename to create short name from
	out		Buffer to hold short filename
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_create_short_name(char *filename,char *out) {
size_t  count;
char *s;
char *f;
char *v;
char *buf[11]; 
char *b;
char *e;
size_t longfilecount;
FILERECORD searchbuf;
	
/* translate to uppercase, removing any illegal characters */

for(longfilecount=1;longfilecount<9999;longfilecount++) {
	memset(out,0,11);				/* clear buffer */

	s=out;						/* copy first 6 chars */
	f=filename;

	count=0;

	itoa(longfilecount,buf);			/* get length of number */

	while(count < 8-strlen(buf)) {
		if((char) *f != 0x20) {
			*s++=*f;
			count++;
		}

		f++;
	}

	s--;
	*s++='~';

	b=s;

	itoa(longfilecount,b);

	f=filename;
	while(*f != 0) {

		if(*f++ == 0x2e) {
			f--;
			break;		/* find extension */
		}
	} 

	strncat(s,f,MAX_PATH);	/* append extension */

	s=out;

	for(count=0;count<strlen(out);count++) {
		if(*s > 'a' && *s < 'z') *s=*s-32;
		s++;
	}    

	if(findfirst(out,&searchbuf) == -1) return(NO_ERROR);			/* check if file already exists */
}

	return(-1);
}

uint8_t fat_create_long_filename_checksum(char *filename) {
uint8_t sum=0;
uint32_t count;

for (count=0;count<11;count++) {
	sum=(sum >> 1) + ((sum & 1) << 7);  /* rotate */
	sum += filename[count];                       /* add next name byte */
}

return(sum); 
}

/*
 * Delete long filename
 *
 * In:  filename	Filename to delete
	block		Block number of long filename
	entryno			FAT entry number of long filename
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_unlink_long_filename(char *filename,uint64_t block,size_t entry) {
size_t count;
char *blockptr;
char *blockbuf;
SPLITBUF splitbuf;
char *fullpath[MAX_PATH];
uint64_t blockcount;
size_t flen;
BLOCKDEVICE blockdevice;
BPB *bpb;
int fattype;
uint64_t block_write_count;

getfullpath(filename,fullpath);				/* get full path */
splitname(fullpath,&splitbuf);				/* split filename */

fattype=fat_detect_change(splitbuf.drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(splitbuf.drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(NULL);
}

bpb=blockdevice.superblock;	/* point to data */

flen=strlen(splitbuf.filename)/13;		/* get number of 13 byte sections */
if(strlen(splitbuf.filename) % 13) flen++;		/* get number of 13 byte sections */
flen++;			/* plus short entry */

/* allocate buffer */

if(flen*FAT_ENTRY_SIZE < (bpb->sectorsperblock*bpb->sectorsize)) {		/* one block */
	blockcount=1;
}
else
{
	blockcount=(flen*FAT_ENTRY_SIZE)/(bpb->sectorsperblock*bpb->sectorsize);	/* more than one block */
	if(blockcount % (bpb->sectorsperblock*bpb->sectorsize)) blockcount++;	/* round up */
}

blockbuf=kernelalloc((bpb->sectorsperblock*bpb->sectorsize)*blockcount);	/* allocate block buffer */
if(blockbuf == NULL) return(-1);

/* read in blocks */

blockptr=blockbuf;

for(count=0;count<blockcount;count++) {

	if(blockio(_READ,splitbuf.drive,block+count,blockptr) == -1) {	/* read block for entry */
		kernelfree(blockbuf);
		return(-1);
	}

	blockptr=blockptr+(bpb->sectorsperblock*bpb->sectorsize);
}

blockptr=blockbuf;
blockptr += (entry*FAT_ENTRY_SIZE);			/* point to entries */

for(count=0;count<flen;count++) {			/* delete entries */
	*blockptr=0xE5;
	blockptr=blockptr+FAT_ENTRY_SIZE;
}

/* write out blocks */

blockptr=blockbuf;

for(count=0;count<blockcount;count++) {

	if(blockio(_WRITE,splitbuf.drive,block+count,blockptr) == -1) {	/* write block for entry */
		kernelfree(blockbuf);
		return(-1);
	}

	blockptr=blockptr+(bpb->sectorsperblock*bpb->sectorsize);
}

kernelfree(blockbuf);

return(NO_ERROR);
}

/*
 * Create long filename
 *
 * In:  type		Type of entry to create (0=file, 1=directory)
	newname		FILERECORD of new filename
	block		Block number of long filename
	entry		FAT entry number of long filename
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_create_long_filename(size_t type,FILERECORD *newname,uint64_t block,size_t entry) {
size_t count;
char *fptr;
char *copyptr;
size_t flen;
char *s;
LFN_ENTRY lfn[255];
DIRENT dirent;
size_t ec;
char *blockptr;
char *blockbuf;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;
char *fullpath[MAX_PATH];
uint64_t blockcount;
char *file[11];
uint32_t seq;
BPB *bpb;
size_t fattype;
uint64_t block_count_loop;
TIME TIME;
size_t startblock;
gettime(&TIME);				/* get time and date */
FILERECORD exists;
size_t rootdir;
size_t start_of_data_area;

getfullpath(newname,fullpath);				/* get full path */
splitname(fullpath,&splitbuf);				/* split filename */

fattype=fat_detect_change(splitbuf.drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(splitbuf.drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(NULL);
}

bpb=blockdevice.superblock;				/* point to data */

if(fattype == 12 || fattype == 16) {
	rootdir=((bpb->rootdirentries*FAT_ENTRY_SIZE)+(bpb->sectorsperblock-1))/bpb->sectorsize;
	start_of_data_area=rootdir+(bpb->reservedsectors+(bpb->numberoffats*bpb->sectorsperfat))-2; /* get start of data area */
}
else
{
	start_of_data_area=bpb->fat32rootdirstart+(bpb->reservedsectors+(bpb->numberoffats*bpb->fat32sectorsperfat))-2;
}
	
memset(lfn,0,sizeof(LFN_ENTRY)*255);		/* clear struct */

/* copy filename to new lfn entries */

fat_create_short_name(splitbuf.filename,file);		/* get short filename */

/* create long filename in reverse order */


flen=strlen(splitbuf.filename)/13;			/* get number of 13 byte sections */
if(strlen(splitbuf.filename) % 13) flen++;		/* round up */

count=flen;
seq=1;							/* first sequence number */
fptr=splitbuf.filename;					/* point to name */

/* create short entry */

fat_convert_filename(file,dirent.filename);		/* convert to fat entry format */

dirent.attribs=newname->attribs;

if(type == _DIR) dirent.attribs |= FAT_ATTRIB_DIRECTORY;	/* set directory bit */

dirent.reserved=0;
dirent.create_time_fine_resolution=0;
dirent.createtime=(newname->create_time_date.hours << 11) | (newname->create_time_date.minutes << 6) | newname->create_time_date.seconds; /* convert time and date to FAT format */
dirent.createdate=((newname->create_time_date.year-1980) << 9) | (newname->create_time_date.month << 5) | newname->create_time_date.day;
dirent.last_modified_time=dirent.createtime;
dirent.last_modified_date=dirent.createdate;
dirent.filesize=newname->filesize;

/* get start block */

if(type == _FILE) {			/* files have a zero cluster number if they are empty */
	startblock=newname->startblock;
}
else					/* get block for subdirectory entries */
{
	startblock=fat_find_free_block(splitbuf.drive);		/* find free block */
	if(startblock == -1) return(-1);
}

if(findfirst(newname->filename,&exists) == -1) {	/* creating, not updating file or directory entries */
	if((fattype == 12) || (fattype == 16)) {
		dirent.block_high_word=0;
		dirent.block_low_word=(startblock & 0xffff);
	} 
	else
	{
		dirent.block_low_word=(startblock & 0xffff);
		dirent.block_high_word=(startblock >> 16);
	}
}

/* create long filename entries in reverse order */

while(1) {
	memset(lfn[count],0,sizeof(LFN_ENTRY));

	lfn[count].attributes=0x0f;					/* attributes must be 0x0f */
	lfn[count].sc_alwayszero=0;					/* must be zero */
	lfn[count].checksum=fat_create_long_filename_checksum(dirent.filename);		/* create checksum */
	lfn[count].typeindicator=0;

	lfn[count].sequence=seq;
	if(seq == flen) lfn[count].sequence |= 0x40;			/* last entry */

	s=lfn[count].firstfive_chars;

	for(ec=0;ec<5;ec++) {
		if(*fptr == 0) {
				lfn_pad_bytes(ec,5,fptr);		/* pad out filename with 0xFF bytes */
				lfn_pad_bytes(6,6,&lfn[count].nextsix_chars);
				lfn_pad_bytes(2,2,&lfn[count].lasttwo_chars);
				goto lfn_over;
		}

		*s=*fptr++;
		s=s+2;
	}

	s=lfn[count].nextsix_chars;

	for(ec=0;ec<6;ec++) {
		if(*fptr == 0) {
				lfn_pad_bytes(ec,6,fptr);		/* pad out filename with 0xFF bytes */
				lfn_pad_bytes(2,2,&lfn[count].lasttwo_chars);
				goto lfn_over;
		}

		*s=*fptr++;
		s=s+2;
	}

	s=lfn[count].lasttwo_chars;

	for(ec=0;ec<2;ec++) {
		if(*fptr == 0) {
			lfn_pad_bytes(ec,2,&lfn[count].lasttwo_chars);	/* pad out filename with 0xFF bytes */
			goto lfn_over;
		}

		*s=*fptr++;
		s=s+2;
	}

	count--;	
	seq++;
}

lfn_over:
blockcount=(flen*FAT_ENTRY_SIZE)/(bpb->sectorsperblock*bpb->sectorsize);	/* get number of blocks */

if(flen*FAT_ENTRY_SIZE < (bpb->sectorsperblock*bpb->sectorsize)) {		/* one block */
	blockcount=1;
}
else
{
	blockcount=(flen*FAT_ENTRY_SIZE)/(bpb->sectorsperblock*bpb->sectorsize);	/* more than one block */
	blockcount++;
}

blockbuf=kernelalloc((bpb->sectorsperblock*bpb->sectorsize)*blockcount);	/* allocate block buffer */
if(blockbuf == NULL) return(-1);

blockptr=blockbuf;

/* read in blocks */

for(block_count_loop=block;block_count_loop<(uint64_t) block+blockcount;block_count_loop++) {
	if(blockio(_READ,splitbuf.drive,(uint64_t) block_count_loop,blockptr) == -1) {	/* read block for entry */
		kernelfree(blockbuf);
		return(-1);
	}

	blockptr += (bpb->sectorsperblock*bpb->sectorsize);
}

blockptr=blockbuf+(entry*FAT_ENTRY_SIZE);

memcpy(blockptr,&lfn[1],(flen*FAT_ENTRY_SIZE));			/* copy long filename entries */
memcpy(blockptr+(((flen)*FAT_ENTRY_SIZE)),&dirent,FAT_ENTRY_SIZE);	/* copy short filename entry */

/* write out blocks */
blockptr=blockbuf;

for(block_count_loop=block;block_count_loop<(uint64_t)  block+blockcount;block_count_loop++) {
		if(blockio(_WRITE,splitbuf.drive,(uint64_t) block_count_loop,blockptr) == -1) {	/* read block for entry */
			kernelfree(blockbuf);
			return(-1);
		}

		blockptr=blockptr+(bpb->sectorsperblock*bpb->sectorsize);	/* point to next */
}

kernelfree(blockbuf);

/* create subdirectory entries (. and ..) */
return(create_subdirectory_entries(splitbuf.dirname,start_of_data_area,startblock,dirent.createtime,dirent.createdate,fattype,splitbuf.drive));
}

/*
 * Pad long filename with 0xFF bytes
 *
 * In:  numberofbytes	Number of bytes to pad.
	totalbytes	Total number of bytes in entry
	ptr		Pointer to entry
 *
 * Returns: Nothing
 *
 */

void lfn_pad_bytes(size_t numberofbytes,size_t totalbytes,char *ptr) {
size_t padcount;

if(numberofbytes == totalbytes) {
	padcount=totalbytes;
}
else
{
	if((totalbytes-numberofbytes) < 0) return;

	padcount=totalbytes-numberofbytes;		/* number of bytes to pad with */
}

while(padcount > 0) {
	*ptr++=0xFF;
	padcount--;
}

return;
}

/*
 * Convert FAT entry to filename
 *
 * In:  filename	FAT entry to convert
	out		Buffer to hold filename
 *
 * Returns: nothing
 *
 */	
void fat_entry_to_filename(char *filename,char *out) {
char *f;
char *o;
size_t  count;

char *b;

memset(out,0,11);

f=filename;
o=out;

for(count=0;count<8;count++) {
	if(*f == ' ') break;

	*o++=*f++;				/* get filename */
}

b=filename+8;				/* point to end */

if(*b != ' ') {			/* get extension */
	f=b;
	*o++='.';

	for(count=0;count<3;count++) {			/* copy extension */
		*o++=*f++;
	}
}

*o++=0;		/* put null at end */

return;
}

/*
 * Check if filename is a long or short filename
 *
 * Note: Filenames are considered long even if they are within the limits for 8.3 names
 * if they contain lowercase letters, restricted characters or . characters before the extension.
 *
 * In: Filename	Filename to check
 *
 * Returns TRUE or FALSE
 *
 */ 

size_t fat_is_long_filename(char *filename) {
char *ptr=filename;
char *long_chars="+,;=[] \t";
char *lp;
size_t char_count=0;

while(*ptr != 0) {
	if((*ptr >= 'a') && (*ptr <= 'z')) return(TRUE);	/* Is lowercase letter */
	
	lp=long_chars;

	while(*lp != 0) {					/* check for characters that are only allowed in long filenames */
		if(*ptr == *lp) return(TRUE);
		lp++;
	}

	if(*lp == '.') {					/* . characters are allowed in any place in long filenames */
		if(char_count < (strlen(filename)-3)) return(TRUE);
	}


	ptr++;
	char_count++;
}

if(strlen(filename) > 12) return(TRUE);			/* is long filename */

return(FALSE);
}
