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
#include "exfat.h"
#include "debug.h"
#include "string.h"
#include "time.h"

/* 
 * ExFAT Filesystem
 *
 */

char *path_tokens[MAX_PATH][MAX_PATH];

/*
 * Initialize ExFAT filesystem module
 *
 * In:  initstr	Initialisation string
 *
 * Returns: 0 on success or -1 on failure
 *
 */
size_t exfat_init(char *initstr) {
FILESYSTEM exfatfilesystem;

MAGIC magicdata[] = { { "EXFAT   ",8,3 }, 
                    };

exfatfilesystem.magic_count=1;

memcpy(&exfatfilesystem.magicbytes,&magicdata,exfatfilesystem.magic_count*sizeof(MAGIC));

strncpy(exfatfilesystem.name,"ExFAT fileystem module",MAX_PATH);
exfatfilesystem.findfirst=&exfat_findfirst;
exfatfilesystem.findnext=&exfat_findnext;
exfatfilesystem.read=&exfat_read;
exfatfilesystem.write=&exfat_write;
exfatfilesystem.rename=&exfat_rename;
exfatfilesystem.unlink=&exfat_unlink;
exfatfilesystem.mkdir=&exfat_mkdir;
exfatfilesystem.rmdir=&exfat_rmdir;
exfatfilesystem.create=&exfat_create;
exfatfilesystem.chmod=&exfat_chmod;
exfatfilesystem.touch=&exfat_touch;

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
size_t exfat_findfirst(char *filename,FILERECORD *buf) {
return(exfat_find(FALSE,filename,buf));
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
size_t exfat_findnext(char *filename,FILERECORD *buf) {
return(exfat_find(TRUE,filename,buf));
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
BLOCKDEVICE blockdevice;
char *blockbuf;
char *blockptr;
SPLITBUF *splitbuf;
ExFATBootSector *superblock_info;
uint64_t findblock;
ExFATFileEntry FileEntry;
ExFATFileInfoEntry FileInfoEntry;
ExFATFilenameEntry FilenameEntry;
size_t count;
char *fullpath[MAX_PATH];
char *FilenamePtr=buf->filename;

splitbuf=kernelalloc(sizeof(SPLITBUF));			/* allocate split buffer */
if(splitbuf == NULL) return(-1);

if(find_type == FALSE) {
	getfullpath(filename,fullpath);			/* get full filename */
	touppercase(fullpath,fullpath);				/* convert to uppercase */

	splitname(fullpath,splitbuf);				/* split filename */

	buf->findentry=0;

	buf->findlastblock=exfat_get_start_block(splitbuf->drive,splitbuf->dirname); /* get start block of directory file is in */
	if(buf->findlastblock == -1) {
		kernelfree(splitbuf);
		return(-1);
	}

	strncpy(buf->searchfilename,fullpath,MAX_PATH); 
}
else
{
	splitname(buf->searchfilename,splitbuf);	/* split filename */
}

if(strlen(splitbuf->filename) == 0) strncpy(splitbuf->filename,"*",MAX_PATH);	/* if no filename, search for all files */

if(exfat_detect_change(splitbuf->drive) == -1) {
	kernelfree(splitbuf);
	return(-1);
}

if(getblockdevice(splitbuf->drive,&blockdevice) == -1) {
	kernelfree(splitbuf);
	return(-1);
} 

superblock_info=blockdevice.superblock;				/* point to superblock data */

blockbuf=kernelalloc(1 << superblock_info->BytesPerBlockShift);			/* allocate block buffer */ 
if(blockbuf == NULL) {
	kernelfree(splitbuf);
	return(-1);
}
	
findblock=(superblock_info->ClusterHeapOffset+buf->findlastblock) - 2;	/* get block number relative to cluster heap */

/* search through directory until filename found */

while(1) {
	blockptr=blockbuf+(buf->findentry*EXFAT_ENTRY_SIZE);	/* point to next file */

	if(blockio(DEVICE_READ,splitbuf->drive,((uint64_t) findblock),blockbuf) == -1) { /* read directory block */
		kernelfree(blockbuf);
		kernelfree(splitbuf);
		return(-1);
	}

	while(buf->findentry < (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE) {
		buf->findentry++;

		if((uint8_t) *blockptr ==  EXFAT_ENTRY_FILE) {				/* file entry */
			memcpy(&FileEntry,blockptr,EXFAT_ENTRY_SIZE);

			FilenameEntry.entrytype &= 0x7F;		/* mark as deleted */

			buf->attribs=FileEntry.attributes;

			buf->create_time_date.hours=(FileEntry.create_time_date & 0xf800) >> 11;
			buf->create_time_date.minutes=(FileEntry.create_time_date  & 0x7e0) >> 5;
			buf->create_time_date.seconds=(FileEntry.create_time_date & 0x1f);
			buf->create_time_date.year=((FileEntry.create_time_date & 0xfe000000) >> 15)+1980;
			buf->create_time_date.month=(FileEntry.create_time_date & 0x1e00000) >> 20;
			buf->create_time_date.day=(FileEntry.create_time_date & 0x1f0000) >> 24;
	
			buf->last_written_time_date.hours=(FileEntry.last_written_time_date & 0xf800) >> 11;
			buf->last_written_time_date.minutes=(FileEntry.last_written_time_date  & 0x7e0) >> 5;
			buf->last_written_time_date.seconds=(FileEntry.last_written_time_date & 0x1f);
			buf->last_written_time_date.year=((FileEntry.create_time_date & 0xfe000000) >> 15)+1980;
			buf->last_written_time_date.month=(FileEntry.create_time_date & 0x1e00000) >> 20;
			buf->last_written_time_date.day=(FileEntry.create_time_date & 0x1f0000) >> 24;

			buf->last_accessed_time_date.hours=(FileEntry.last_accessed_time_date & 0xf800) >> 11;
			buf->last_accessed_time_date.minutes=(FileEntry.last_accessed_time_date  & 0x7e0) >> 5;
			buf->last_accessed_time_date.seconds=(FileEntry.last_accessed_time_date & 0x1f);
			buf->last_accessed_time_date.year=((FileEntry.create_time_date & 0xfe000000) >> 15)+1980;
			buf->last_accessed_time_date.month=(FileEntry.create_time_date & 0x1e00000) >> 20;
			buf->last_accessed_time_date.day=(FileEntry.create_time_date & 0x1f0000) >> 24;

			if(buf->attribs & EXFAT_ATTRIB_DIRECTORY) {
				buf->flags=FILE_DIRECTORY;
			}
			else
			{
				buf->flags=FILE_REGULAR;
			}

		}
		else if((uint8_t) *blockptr ==  EXFAT_ENTRY_INFO) {				/* file information */
			memcpy(&FileInfoEntry,blockptr,EXFAT_ENTRY_SIZE);

			buf->filesize=FileInfoEntry.filesize;
			buf->startblock=FileInfoEntry.startblock;

			if(FileInfoEntry.flags & EXFAT_NO_FAT_CHAIN) buf->flags |= EXFAT_FILE_CONTIGUOUS;	/* file is continguous */
		}
		else if((uint8_t) *blockptr ==  EXFAT_ENTRY_FILENAME) {				/* filename */
			/* get filename */

			for(count=0;count < FileEntry.entrycount;count++) {
				memcpy(&FilenameEntry,blockptr,EXFAT_ENTRY_SIZE);

				AppendUnicodeNameFromFilenameEntry(&FilenameEntry,FilenamePtr);		/* get filename entry */
				FilenamePtr += EXFAT_ENTRY_SIZE;
			}

		}
		else if((uint8_t) *blockptr == 0) {						/* at end of directory */
			kernelfree(blockbuf);
			kernelfree(splitbuf);
		
			setlasterror(END_OF_DIRECTORY);
			return(-1); 
		}

		if(wildcard(splitbuf->filename,buf->filename) == 0) { 				/* if file found */     
			setlasterror(NO_ERROR);
			return(0);
		}

		blockptr += EXFAT_ENTRY_SIZE;	
	}
	
	if(buf->findentry >= (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE) {		/* at end of block */
		buf->findlastblock=exfat_get_next_block(splitbuf->drive,buf->findlastblock);	/* get next block */		
		buf->findentry=0;
		findblock=buf->findlastblock-2;

		if(buf->findlastblock == (uint64_t) 0xFFFFFFFFFFFFFFFF) {		/* at end of directory */
			kernelfree(blockbuf);
			kernelfree(splitbuf);

			setlasterror(END_OF_DIRECTORY);
			return(-1);
		}
	}

}

kernelfree(blockbuf);
kernelfree(splitbuf);

setlasterror(END_OF_DIRECTORY);
return(-1);
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
char *ioaddr;
char *iobuf;
size_t bytesdone=0;
BLOCKDEVICE blockdevice;
FILERECORD read_file_record;
size_t count;
uint64_t blockno;
size_t blockoffset;
size_t nextblock;
ExFATBootSector *superblock_info;
char *blockbuf;
size_t read_size;
size_t pos_rounded_down;
size_t pos_mod;
size_t pos_blocksize;

if(gethandle(handle,&read_file_record) == -1) {		/* get file information */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

if(exfat_detect_change(read_file_record.drive) == -1) return(-1);

if(getblockdevice(read_file_record.drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;		/* point to superblock */

/* If current position has changed using seek(), find block number from FAT */

if((read_file_record.flags & FILE_POS_MOVED_BY_SEEK) && (read_file_record.currentblock != read_file_record.previousblock) \
   && ((read_file_record.currentblock != (read_file_record.previousblock+1)))) {
	if((read_file_record.flags & EXFAT_NO_FAT_CHAIN) == 0) {	/* file is not continguous */
		/* If seeking to previous block, start from the start block.
		   The FAT is a singly-linked list so it must seek for the beginning */

		if(read_file_record.currentpos < read_file_record.previouspos) {
			read_file_record.currentblock=read_file_record.startblock;

			if((read_file_record.currentpos >= read_file_record.filesize) || \
			   (read_file_record.currentblock == 0) || \
			   (read_file_record.currentblock == 0xffffffffffffff)) {
				setlasterror(INPUT_PAST_END);
				return(-1);
			}

			read_file_record.currentpos=0;
		}	
	
		if(size > (1 << superblock_info->BytesPerBlockShift)) {
			count=size / (1 << superblock_info->BytesPerBlockShift);	
			count=round_down(count,1 << superblock_info->BytesPerBlockShift);	/* get number of whole blocks */
		}
		else
		{
			count=1;
		}

		while(count > 0) {
			read_file_record.currentblock=exfat_get_next_block(read_file_record.drive,read_file_record.currentblock);
			count--;
			read_file_record.currentpos +=  count;
		}
	}

	/* partial block */
	if( (size - (1 << superblock_info->BytesPerBlockShift)) > 0) read_file_record.currentpos +=  (size - (1 << superblock_info->BytesPerBlockShift));
}

read_file_record.previousblock=read_file_record.currentblock;		/* update previous block */

iobuf=kernelalloc(1 << superblock_info->BytesPerBlockShift);
if(iobuf == NULL) return(-1);

/* reading past end of file */ 

if((read_file_record.currentpos >= read_file_record.filesize) || \
   (read_file_record.currentblock == 0) || 
   (read_file_record.currentblock == (uint64_t) 0xFFFFFFFFFFFFFFFF)) {

	setlasterror(INPUT_PAST_END);
	return(-1);
}
	
/* until all data read */

read_size=size;

while(read_size > 0) {
	blockno=((read_file_record.currentblock-2) * (1 << superblock_info->BytesPerBlockShift)) + superblock_info->ClusterHeapOffset;
	blockoffset=read_file_record.currentpos % (1 << superblock_info->BytesPerBlockShift);	/* distance inside the block */
	count=(1 << superblock_info->BytesPerBlockShift)-blockoffset;
		
	if(blockio(DEVICE_READ,read_file_record.drive,blockno,iobuf) == -1) {		/* read block */
		kernelfree(iobuf);
		return(-1);
	}

	/* adjust copy size */

	if(size < (1 << superblock_info->BytesPerBlockShift)) {
		count=size;
	}
	else
	{
		if(read_size < (1 << superblock_info->BytesPerBlockShift)) {
			count=read_size;
		}
		else
		{
			count=(1 << superblock_info->BytesPerBlockShift);
		}
	}

	if((blockoffset+count) > (1 << superblock_info->BytesPerBlockShift)) count=(1 << superblock_info->BytesPerBlockShift)-(blockoffset+count);

	memcpy(addr,iobuf+blockoffset,count);			/* copy data to buffer */

	bytesdone += count;
	addr += count;      /* point to next address */

	/* if reading data that crosses a block boundary, find next block to read remainder of data */

	if((read_file_record.currentpos+count) >= round_up(read_file_record.currentpos,(1 << superblock_info->BytesPerBlockShift))) {
		if(read_file_record.flags & EXFAT_NO_FAT_CHAIN) {
			read_file_record.currentblock++;
		}
		else
		{		
	 		read_file_record.currentblock=exfat_get_next_block(read_file_record.drive,read_file_record.currentblock);
		}
	
		if(read_file_record.currentblock >= 0xffffffffffffff) break;
	}

	/* increment read size */

	read_size -= count; 
	read_file_record.currentpos += count;
	read_file_record.filesize += count;
	read_file_record.flags &= FILE_POS_MOVED_BY_SEEK;

	if(updatehandle(handle,&read_file_record) == -1) {		/* update file information */
		setlasterror(INVALID_HANDLE);
		return(-1);
	}

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

size_t exfat_write(size_t handle,void *addr,size_t size) {
uint64_t block;
char *ioaddr;
char *iobuf;
size_t bytesdone;
BLOCKDEVICE blockdevice;
TIME time;
FILERECORD write_file_record;
size_t count;
uint64_t blockno;
size_t blockoffset;
size_t last;
size_t newblock;
ExFATBootSector *superblock_info;
char *blockbuf;
uint64_t lastblock;
uint64_t sblock;
size_t write_size;
size_t is_first_written_block=FALSE;
SPLITBUF split;
size_t returnvalue;

get_time_date(&time);				/* get time and date */

if(gethandle(handle,&write_file_record) == -1) {	/* invalid handle */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

if(exfat_detect_change(write_file_record.drive) == -1) return(-1);

if(getblockdevice(write_file_record.drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;		/* point to superblock */

/* If current position has changed using seek(), find block number from FAT */

if((write_file_record.flags & FILE_POS_MOVED_BY_SEEK)) {
	if((write_file_record.flags & EXFAT_NO_FAT_CHAIN) == 0) {	/* file is not continguous */
		for(count=0;count<(write_file_record.currentpos/((1 << superblock_info->BytesPerBlockShift)));count++) {
			write_file_record.currentblock=fat_get_next_block(write_file_record.drive,write_file_record.currentblock);
		}
	}
}

blockno=((write_file_record.currentblock-2)*(1 << superblock_info->BytesPerBlockShift)) + superblock_info->ClusterHeapOffset;

/* writing past end of file, add new blocks to FAT chain only needs to be done once per write past end */ 

if((write_file_record.currentpos >= write_file_record.filesize) || \
   (write_file_record.currentblock == 0) || \
   (write_file_record.currentblock == 0xffffffffffffffff)) {

	/* update FAT and/or bitmap */

	count=(size / ((1 << superblock_info->BytesPerBlockShift)));		/* number of blocks in FAT chain */
	if(size % ((1 << superblock_info->BytesPerBlockShift))) count++;	/* round up */

	/* need to add another block to FAT chain */

	if((write_file_record.startblock == 0) || (write_file_record.currentpos % ((1 << superblock_info->BytesPerBlockShift)) == 0)) {
		lastblock=write_file_record.currentblock;			/* save last block in FAT chain */
		write_file_record.currentblock=newblock;			/* new current block */
	
		do {
			if(write_file_record.flags & EXFAT_NO_FAT_CHAIN) {	/* file is continguous */
				newblock=exfat_get_bitmap_entry(write_file_record.drive,write_file_record.previousblock+1);
				if(newblock == -1) {
					kernelfree(iobuf);
					return(-1);
				}
				else if(newblock) {	/* file can be extended in bitmap */
		 			newblock=exfat_find_free_bitmap_entry(write_file_record.drive);
					if(newblock == -1) {
						kernelfree(newblock);
						return(-1);
					}
				}
				else if(newblock == 0) {	/* file can't be extended in bitmap */
					exfat_convert_bitmap_to_fat_chain(write_file_record.drive,write_file_record.startblock);	/* convert to FAT chain */

					/* cleared EXFAT_NO_FAT_CHAIN flag */
				}
			}

			/* fall through to this code if EXFAT_NO_FAT_CHAIN flag was cleared */

			if((write_file_record.flags & EXFAT_NO_FAT_CHAIN) == 0) {	/* file is not continguous */
				/* add block to FAT chain */

				newblock=exfat_find_free_bitmap_entries(write_file_record.drive,1);
	
				if(exfat_update_fat(write_file_record.drive,lastblock,newblock) == -1) {
					kernelfree(iobuf);
					return(-1);
				}
	
				lastblock=newblock;
				newblock=exfat_find_free_bitmap_entries(write_file_record.drive,1);
			}


			if(write_file_record.startblock == 0) {				/* first block in chain */
				sblock=newblock;

				write_file_record.startblock=newblock;	
				is_first_written_block=TRUE;				/* is first block written to empty file */
			}

			count--;
		} while(count != 0);

		if(exfat_update_fat(write_file_record.drive,newblock,0xffffffffffffffff) == -1) { /* write end of FAT chain */
			kernelfree(iobuf);
			return(-1);
		}
	}
}

iobuf=kernelalloc((1 << superblock_info->BytesPerBlockShift));
if(iobuf == NULL) {
	kernelfree(iobuf);
	return(-1);
}
	
/* until all data written */

write_size=size;
blockno=((write_file_record.currentblock-2)*(1 << superblock_info->BytesPerBlockShift))+superblock_info->ClusterHeapOffset;

while(write_size > 0) {
	blockoffset=write_file_record.currentpos % ((1 << superblock_info->BytesPerBlockShift));	/* distance inside the block */
	count=((1 << superblock_info->BytesPerBlockShift))-blockoffset;

	if(blockio(DEVICE_READ,write_file_record.drive,blockno,iobuf) == -1) {		/* read block */
		kernelfree(iobuf);
		return(-1);
	}

	/* adjust copy size */

	if((blockoffset+size) > ((1 << superblock_info->BytesPerBlockShift))) {	/* write crosses data */
		count=((1 << superblock_info->BytesPerBlockShift))-size;	/* adjust size */
		/* the remainder of the data will be written on the next cycle */
	}
	else
	{
		count=size;
	}

	memcpy(iobuf+blockoffset,addr,count);			/* copy data to buffer */

	if(blockio(DEVICE_WRITE,write_file_record.drive,blockno,iobuf) == -1) {		/* write block */
		kernelfree(iobuf);
		return(-1);
	}

	addr += count;      /* point to next address */

	/* increment write size */
	write_size -= count; 
	write_file_record.currentpos += count;
	write_file_record.filesize += count;

	/* if writing data that crosses a block boundary, find next block to write remainder of data */

	if((write_file_record.currentpos+count) >= round_up(write_file_record.currentpos,((1 << superblock_info->BytesPerBlockShift)))) {
	 	write_file_record.currentblock=fat_get_next_block(write_file_record.drive,write_file_record.currentblock);
	}

}

updatehandle(handle,&write_file_record);			/* update file handle data */

exfat_update_file_entry(write_file_record.drive,&write_file_record);	/* update file entry */

setlasterror(NO_ERROR);
return(bytesdone);						/* return number of bytes */
}


/*
 * Rename file
 *
 * In:  oldname		File to rename
	newname		New filename
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t exfat_rename(char *filename,char *newname) {
SPLITBUF splitbuf_old;
SPLITBUF splitbuf_new;
char *blockbuf;
FILERECORD buf;
FILERECORD newbuf;
char *blockptr;
char *fullpath[MAX_PATH];
char *newfullpath[MAX_PATH];
size_t FileEntryCount=0;
size_t BlockUpdated=FALSE;
ExFATFileEntry FileEntry;
ExFATFileInfoEntry FileInfoEntry;
ExFATFilenameEntry FilenameEntry;
ExFATBootSector *superblock_info;
size_t dirblock;
size_t direntrycount;
char *FilenamePtr;
size_t nextdirblock;

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

FilenamePtr=splitbuf_old.filename;

if(findfirst(newfullpath,&newbuf) == 0) {		/* if new filename exists */
	setlasterror(FILE_EXISTS);
	return(-1);
}

blockbuf=kernelalloc((1 << superblock_info->BytesPerBlockShift));			/* allocate block buffer */
if(blockbuf == NULL) {
	setlasterror(NO_MEM);
	return(-1);
}
			
dirblock=(superblock_info->ClusterHeapOffset + buf.findlastblock)-2;	/* get block number relative to cluster heap */
direntrycount=0;

memset(buf.filename,0,MAX_PATH);

while(1) {
	BlockUpdated=FALSE;

	blockptr=blockbuf+(dirblock*EXFAT_ENTRY_SIZE);	/* point to next file */

	if(blockio(DEVICE_READ,splitbuf_old.drive,((uint64_t) dirblock),blockbuf) == -1) { /* read directory block */
		kernelfree(blockbuf);
		return(-1);
	}

	if((uint8_t) *blockptr ==  EXFAT_ENTRY_FILE) {				/* file entry */
		memcpy(&FileEntry,blockptr,EXFAT_ENTRY_SIZE);
	}
	else if((uint8_t) *blockptr ==  EXFAT_ENTRY_INFO) {				/* file information */
		memcpy(&FileInfoEntry,blockptr,EXFAT_ENTRY_SIZE);
	}
	else if((uint8_t) *blockptr ==  EXFAT_ENTRY_FILENAME) {				/* filename */
		if(FileEntryCount > (strlen(filename) / EXFAT_NAME_SEGMENT_LENGTH)) {	/* delete extra entries at end */
			memcpy(&FilenameEntry,blockptr,EXFAT_ENTRY_SIZE);

			FilenameEntry.entrytype &= 0x7F;		/* mark as deleted */
			memcpy(blockptr,&FilenameEntry,EXFAT_ENTRY_SIZE);
		}
		else
		{
			memcpy(&FilenameEntry,blockptr,EXFAT_ENTRY_SIZE);

			AppendFilenameEntryFromUnicodeName(FilenamePtr,&FilenameEntry);		/* put filename entry */
			memcpy(blockptr,&FilenameEntry,EXFAT_ENTRY_SIZE);

			FilenamePtr += EXFAT_NAME_SEGMENT_LENGTH;
		}

		BlockUpdated=TRUE;		
		blockptr += EXFAT_ENTRY_SIZE;	
	}
	
	if(BlockUpdated == TRUE) {		/* write block if it was updated */
		if(dirblock == (buf.findlastblock-2)) SetExFATDirtyFlag();		/* set dirty flag for first write */

		if(blockio(DEVICE_WRITE,splitbuf_old.drive,((uint64_t) dirblock),blockbuf) == -1) { /* write directory block */
			kernelfree(blockbuf);
			return(-1);
		}
	}

	if(direntrycount >= (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE) {		/* at end of block */
		nextdirblock=exfat_get_next_block(splitbuf_old.drive,dirblock);	/* get next block */		

		buf.findlastblock=(superblock_info->ClusterHeapOffset + nextdirblock)-2;

		if(nextdirblock == (uint64_t) 0xFFFFFFFFFFFFFFFF) {		/* at end of directory */
			kernelfree(blockbuf);
			setlasterror(END_OF_DIRECTORY);
			return(-1);
		}
	}

}
	
kernelfree(blockbuf);

setlasterror(NO_ERROR);  
return(NO_ERROR);				/* no error */
}

/*
 * Delete file
 *
 * In:  name	File to delete
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t exfat_unlink(char *filename) {
SPLITBUF splitbuf;
char *blockbuf;
FILERECORD buf;
FILERECORD newbuf;
char *blockptr;
char *fullpath[MAX_PATH];
size_t FileEntryCount=0;
size_t BlockUpdated=FALSE;
ExFATFileEntry FileEntry;
ExFATFileInfoEntry FileInfoEntry;
ExFATFilenameEntry FilenameEntry;
ExFATBootSector *superblock_info;
size_t dirblock;
size_t direntrycount;
size_t unlink_block;
size_t nextdirblock;
size_t previous_block;

getfullpath(filename,fullpath);			/* get full path of filename */

splitname(fullpath,&splitbuf);		/* split name */

if(findfirst(fullpath,&buf) == -1) {		/* get file entry */
	setlasterror(FILE_NOT_FOUND);
	return(-1);
}

blockbuf=kernelalloc((1 << superblock_info->BytesPerBlockShift));			/* allocate block buffer */
if(blockbuf == NULL) {
	setlasterror(NO_MEM);
	return(-1);
}
			
dirblock=(superblock_info->ClusterHeapOffset+buf.findlastblock)-2;	/* get block number relative to cluster heap */

memset(buf.filename,0,MAX_PATH);

/* search through directory until filename found */

while(1) {
	BlockUpdated=FALSE;

	blockptr=blockbuf+(dirblock*EXFAT_ENTRY_SIZE);	/* point to next file */

	if(blockio(DEVICE_READ,splitbuf.drive,((uint64_t) dirblock),blockbuf) == -1) { /* read directory block */
		kernelfree(blockbuf);
		return(-1);
	}

	for(direntrycount=0;direntrycount < (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE;direntrycount++) {
	
		if((uint8_t) *blockptr ==  EXFAT_ENTRY_FILE) {				/* file entry */
			memcpy(&FileEntry,blockptr,EXFAT_ENTRY_SIZE);
			FileEntry.entrytype &= 0x7F;		/* mark as deleted */
			memcpy(blockptr,&FileEntry,EXFAT_ENTRY_SIZE);
		}
		else if((uint8_t) *blockptr ==  EXFAT_ENTRY_INFO) {				/* file information */
			memcpy(&FileInfoEntry,blockptr,EXFAT_ENTRY_SIZE);
			FileEntry.entrytype &= 0x7F;		/* mark as deleted */
			memcpy(blockptr,&FileEntry,EXFAT_ENTRY_SIZE);
		}
		else if((uint8_t) *blockptr ==  EXFAT_ENTRY_FILENAME) {				/* filename */
			memcpy(&FilenameEntry,blockptr,EXFAT_ENTRY_SIZE);
			FilenameEntry.entrytype &= 0x7F;		/* mark as deleted */
			memcpy(blockptr,&FilenameEntry,EXFAT_ENTRY_SIZE);
		
			BlockUpdated=TRUE;
					
			if(FileEntryCount >= FileEntry.entrycount) break;		/* at end */
		}
		
		blockptr += EXFAT_ENTRY_SIZE;	
	}
	
	if(BlockUpdated == TRUE) {		/* write block if it was updated */
		if(dirblock == (buf.findlastblock-2)) SetExFATDirtyFlag();		/* set dirty flag for first write */

		if(blockio(DEVICE_WRITE,splitbuf.drive,((uint64_t) dirblock),blockbuf) == -1) { /* write directory block */
			kernelfree(blockbuf);
			return(-1);
		}
	}

	if(FileEntryCount >= FileEntry.entrycount) break;		/* at end */

	if(direntrycount >= (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE) {		/* at end of block */
		nextdirblock=exfat_get_next_block(splitbuf.drive,nextdirblock);	/* get next block */		

		if(nextdirblock == (uint64_t) 0xFFFFFFFFFFFFFFFF) {		/* at end of directory */
			kernelfree(blockbuf);

			setlasterror(END_OF_DIRECTORY);
			return(-1);
		}

		dirblock=(superblock_info->ClusterHeapOffset+nextdirblock)-2;	/* get block number relative to cluster heap */
	}

}

if(BlockUpdated == TRUE) {
/* remove FAT and entries */

	unlink_block=exfat_get_start_block(splitbuf.drive,filename);		/* get file's start block */

	do {
		/* remove FAT entry and bitmap entry */
		previous_block=unlink_block;

		if(exfat_update_fat(splitbuf.drive,(uint64_t) unlink_block,0) == -1) return(-1);

		exfat_clear_bitmap_entry(splitbuf.drive,previous_block);		/* remove entry from bitmap */
 		unlink_block=exfat_get_next_block(splitbuf.drive,previous_block);	/* get next block */
	} while(unlink_block != (uint64_t) 0xFFFFFFFFFFFFFFFF);
}

kernelfree(blockbuf);

setlasterror(NO_ERROR);  
return(NO_ERROR);				/* no error */
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
 * Create entry internal function
 *
 * In:  entrytype	Type of entry (0=file,1=directory)
	name		File or directory to create
 *
 * Returns: -1 on error, 0 on success
 *
 */
/* 
 * Create file or directory internal function
 *
 * In:  entrytype	Type of entry (0=file,1=directory)
	filename	File or directory to create
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t exfat_create_int(size_t type,char *filename) {
char *fullpath[MAX_PATH];
char *blockbuf;
char *blockptr;
SPLITBUF splitbuf;
ExFATBootSector *superblock_info;
size_t entry_count=0;
uint64_t rb;
BLOCKDEVICE blockdevice;
uint8_t entrytype;
TIME timedate;
uint32_t time_int;
char *SourceFilenamePtr;
char *DestFilenamePtr;
size_t BytesPerBlock;
ExFATFileEntry FileEntry;
ExFATFileInfoEntry FileInfoEntry;
ExFATFilenameEntry FilenameEntry;
FILERECORD filecheck;
size_t NumberOfFilenameEntries;
size_t readblock;
size_t count;

get_time_date(&timedate);				/* get time and date */

/* convert time and date to FAT format */
time_int=timedate.seconds | (timedate.minutes << 5) | (timedate.hours << 11) | (timedate.day << 16) | (timedate.month << 21) | ((timedate.year - 1980) << 11);

getfullpath(filename,fullpath);

if(findfirst(fullpath,&filecheck) == 0) {			/* file exists */
	setlasterror(FILE_EXISTS);
	return(-1);
}

splitname(fullpath,&splitbuf);

if(exfat_detect_change(splitbuf.drive) == -1) return(-1);
if(getblockdevice(splitbuf.drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;				/* point to superblock data*/

BytesPerBlock=1 << superblock_info->BytesPerBlockShift;	/* number of bytes per block */

blockbuf=kernelalloc(BytesPerBlock);			/* allocate block buffer */ 
if(blockbuf == NULL) return(-1);

rb=exfat_get_start_block(splitbuf.drive,splitbuf.dirname);		/* get start of directory where new file will be created */
if(rb == -1) {
	kernelfree(blockbuf);
	return(-1);
}

/* search through directory until enough free entries found */

NumberOfFilenameEntries=strlen(splitbuf.filename) / EXFAT_NAME_SEGMENT_LENGTH;	/* number of entries to create */
if(strlen(splitbuf.filename) % EXFAT_NAME_SEGMENT_LENGTH) NumberOfFilenameEntries++;	/* round up */

NumberOfFilenameEntries += 2;		/* and file and FileInfoEntry entries */

readblock=superblock_info->ClusterHeapOffset+rb;		/* get block number relative to cluster heap */

do {
	if(blockio(DEVICE_READ,splitbuf.drive,(uint64_t) readblock,blockbuf) == -1) { /* read directory block */
		kernelfree(blockbuf);
		return(-1);
	}

	/* loop through the entries in the block */

	blockptr=blockbuf;

	for(entry_count=0;entry_count < (BytesPerBlock / EXFAT_ENTRY_SIZE);entry_count++) {
		entrytype=*blockptr;
	
		if( ((type & 0x80) && (entry_count >= NumberOfFilenameEntries)) || (type == 0)) {		/* found free entry */		
			/* create file entry */
			FileEntry.entrytype=EXFAT_ENTRY_FILE;				/* entry type */
			FileEntry.entrycount=NumberOfFilenameEntries;				/* number of entries */

			if(type == _DIR) FileEntry.attributes=EXFAT_ATTRIB_DIRECTORY;	/* set directory bit if creating a directory */

			FileEntry.create_time_date=time_int; 			/* set create time and date */
			FileEntry.last_written_time_date=time_int;		/* set last modified time and date */
			FileEntry.last_accessed_time_date=time_int;		/* set last accessed time and date */

			/* create file information entry */
 			FileInfoEntry.entrytype=EXFAT_ENTRY_INFO;
			FileInfoEntry.flags=0;
			FileInfoEntry.filenamelength=strlen(splitbuf.filename);	/* filename length */
			FileInfoEntry.filenamehash=hashfilename(splitbuf.filename);	/* hash of filename */
			FileInfoEntry.validfilesize=0;

			if(type == _DIR) {			/* create subdirectory's block */
				FileInfoEntry.startblock=exfat_find_free_bitmap_entries(splitbuf.drive,1);	/* find free block to use */
				if(FileInfoEntry.startblock == -1) {		/* can't find block */
					kernelfree(blockbuf);
					return(-1);
				}
			}

			FileInfoEntry.filesize=0;

			/* create filename entries */

			NumberOfFilenameEntries=strlen(filename) / EXFAT_NAME_SEGMENT_LENGTH;		/* get number of filename segments */
			if(NumberOfFilenameEntries=strlen(filename) % EXFAT_NAME_SEGMENT_LENGTH) NumberOfFilenameEntries++;	/* round up */

			SourceFilenamePtr=splitbuf.filename;
			DestFilenamePtr=blockptr;

			/* write out blocks */

			for(count=0;count < NumberOfFilenameEntries;count++) {

				if(count == 0) {
					memcpy(blockptr,&FileEntry,EXFAT_ENTRY_SIZE);
				}
				else if(count == 1) {
					memcpy(blockptr,&FileInfoEntry,EXFAT_ENTRY_SIZE);
				}
				else
				{
					FilenameEntry.entrytype=EXFAT_ENTRY_FILENAME;
					FilenameEntry.entrycount=count;

					AppendFilenameEntryFromName(SourceFilenamePtr,DestFilenamePtr);		/* get filename entry */

					SourceFilenamePtr += EXFAT_NAME_SEGMENT_LENGTH;
					DestFilenamePtr += EXFAT_NAME_SEGMENT_LENGTH;
				}

				if((count == NumberOfFilenameEntries) || (blockptr == (blockptr + BytesPerBlock))) {
					if(blockio(DEVICE_WRITE,splitbuf.drive,(uint64_t) readblock,blockbuf) == -1) { /* write directory block */
						kernelfree(blockbuf);
						return(-1);
					}
				}
				
				if((count != NumberOfFilenameEntries) && (blockptr == (blockptr + BytesPerBlock))) {
					readblock=exfat_find_free_bitmap_entries(splitbuf.drive,1);	/* find free block to use */
					if(readblock == -1) {		/* can't find block */
						kernelfree(blockbuf);
						return(-1);		
					}

					memset(blockbuf,0,BytesPerBlock);		/* clear buffer */
					memset(&FileEntry,0,sizeof(ExFATFileEntry));
					memset(&FilenameEntry,0,sizeof(ExFATFilenameEntry));

					blockptr=blockbuf;
					DestFilenamePtr=blockbuf;
				}
			}

			kernelfree(blockbuf);
			return(0);			
		}
		
		entry_count++;
		blockptr += EXFAT_ENTRY_SIZE;
	}


	if(entry_count >= (BytesPerBlock / EXFAT_ENTRY_SIZE)) {		/* at end of block */
		readblock=fat_get_next_block(splitbuf.drive,readblock);			/* get next block */		
		if(readblock == 0xffffffffffffffff) break;				/* at end of directory */

		entry_count=0;
	}

} while(readblock != 0xffffffffffffffff);

kernelfree(blockbuf);

setlasterror(END_OF_DIRECTORY);
return(-1);
}

/*
 * Remove directory
 *
 * In:  name	Directory to remove
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t exfat_rmdir(char *dirname) {
}

/*
 * Set file attributes
 *
 * In:  filename	Filename
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t exfat_chmod(char *filename,size_t attributes) {
SPLITBUF splitbuf;
char *blockbuf;
FILERECORD buf;
FILERECORD newbuf;
char *blockptr;
char *fullpath[MAX_PATH];
size_t FileEntryCount=0;
size_t BlockUpdated=FALSE;
ExFATFileEntry FileEntry;
ExFATBootSector *superblock_info;
BLOCKDEVICE blockdevice;
size_t dirblock;
size_t direntrycount;
size_t nextdirblock;

getfullpath(filename,fullpath);			/* get full path of filename */

splitname(fullpath,&splitbuf);		/* split name */

if(findfirst(fullpath,&buf) == -1) {		/* get file entry */
	setlasterror(FILE_NOT_FOUND);
	return(-1);
}

if(exfat_detect_change(splitbuf.drive) == -1) return(-1);
if(getblockdevice(splitbuf.drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;				/* point to superblock data*/

blockbuf=kernelalloc(1 << superblock_info->BytesPerBlockShift);			/* allocate block buffer */
if(blockbuf == NULL) {
	setlasterror(NO_MEM);
	return(-1);
}
			
dirblock=superblock_info->ClusterHeapOffset+buf.findlastblock;	/* get block number relative to cluster heap */
direntrycount=0;

/* search through directory until filename found */

while(1) {
	BlockUpdated=FALSE;

	blockptr=blockbuf+(dirblock*EXFAT_ENTRY_SIZE);	/* point to next file */

	if(blockio(DEVICE_READ,splitbuf.drive,((uint64_t) dirblock),blockbuf) == -1) { /* read directory block */
		kernelfree(blockbuf);
		return(-1);
	}

	while(direntrycount < (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE) {
	
		if((uint8_t) *blockptr ==  EXFAT_ENTRY_FILE) {				/* file entry */
			memcpy(&FileEntry,blockptr,EXFAT_ENTRY_SIZE);

			FileEntry.attributes=(uint32_t) attributes;			/* set attributes */
			memcpy(blockptr,&FileEntry,EXFAT_ENTRY_SIZE);

			break;
		}
		
		blockptr += EXFAT_ENTRY_SIZE;	
	}
	
	if(FileEntryCount >= FileEntry.entrycount) break;		/* at end */

	if(direntrycount >= (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE) {		/* at end of block */
		nextdirblock=exfat_get_next_block(splitbuf.drive,dirblock);	/* get next block */		
		if(nextdirblock == (uint64_t) 0xFFFFFFFFFFFFFFFF) {		/* at end of directory */
			kernelfree(blockbuf);
			setlasterror(END_OF_DIRECTORY);
			return(-1);
		}

		dirblock=(superblock_info->ClusterHeapOffset+nextdirblock) - 2;	/* get block number relative to cluster heap */	/* get block number relative to cluster heap */
	}

}

if(BlockUpdated == TRUE) {		/* write block if it was updated */
	if(dirblock == (buf.findlastblock-2)) SetExFATDirtyFlag();		/* set dirty flag for first write */

	if(blockio(DEVICE_WRITE,splitbuf.drive,((uint64_t) dirblock),blockbuf) == -1) { /* write0 directory block */
		kernelfree(blockbuf);
		return(-1);
	}
}

kernelfree(blockbuf);

setlasterror(NO_ERROR);  
return(NO_ERROR);				/* no error */
}

/*
 * Set file time and date
 *
 * In:  filename	Filename
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t exfat_touch(char *filename,TIME *create_time_date,TIME *last_written_time_date,TIME *last_accessed_time_date) {
SPLITBUF splitbuf;
char *blockbuf;
FILERECORD buf;
FILERECORD newbuf;
char *blockptr;
char *fullpath[MAX_PATH];
size_t FileEntryCount=0;
size_t BlockUpdated=FALSE;
ExFATFileEntry FileEntry;
ExFATBootSector *superblock_info;
BLOCKDEVICE blockdevice;
size_t dirblock;
size_t direntrycount;
size_t nextdirblock;
getfullpath(filename,fullpath);			/* get full path of filename */

splitname(fullpath,&splitbuf);		/* split name */

if(findfirst(fullpath,&buf) == -1) {		/* get file entry */
	setlasterror(FILE_NOT_FOUND);
	return(-1);
}

if(exfat_detect_change(splitbuf.drive) == -1) return(-1);
if(getblockdevice(splitbuf.drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;				/* point to superblock data*/

blockbuf=kernelalloc(1 << superblock_info->BytesPerBlockShift);			/* allocate block buffer */
if(blockbuf == NULL) {
	setlasterror(NO_MEM);
	return(-1);
}
			
dirblock=superblock_info->ClusterHeapOffset+buf.findlastblock;	/* get block number relative to cluster heap */
direntrycount=0;

/* search through directory until filename found */

while(1) {
	BlockUpdated=FALSE;

	blockptr=blockbuf+(dirblock*EXFAT_ENTRY_SIZE);	/* point to next file */

	if(blockio(DEVICE_READ,splitbuf.drive,((uint64_t) dirblock),blockbuf) == -1) { /* read directory block */
		kernelfree(blockbuf);
		return(-1);
	}

	while(direntrycount < (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE) {
	
		if((uint8_t) *blockptr ==  EXFAT_ENTRY_FILE) {				/* file entry */
			memcpy(&FileEntry,blockptr,EXFAT_ENTRY_SIZE);

			/* set create time and date */
			FileEntry.create_time_date=create_time_date->seconds | 
				  (create_time_date->minutes << 5) |
				  (create_time_date->hours << 11) |
			   	  (create_time_date->day << 16) |
				  (create_time_date->month << 21) |
				  ((create_time_date->year-1980) << 11);

			/* set last written time and date */
			FileEntry.last_written_time_date=last_written_time_date->seconds |
				  (last_written_time_date->minutes << 5) |
				  (last_written_time_date->hours << 11) |
				  (last_written_time_date->day << 16) |
				  (last_written_time_date->month << 21) |
				  ((last_written_time_date->year-1980) << 11);

			/* set last accessed time and date */

			FileEntry.last_accessed_time_date=last_accessed_time_date->seconds |
				  (last_accessed_time_date->minutes << 5) |
				  (last_accessed_time_date->hours << 11) |
				  (last_accessed_time_date->day << 16) |
				  (last_accessed_time_date->month << 21) |
				  ((last_accessed_time_date->year-1980) << 11);

			memcpy(blockptr,&FileEntry,EXFAT_ENTRY_SIZE);

			break;
		}
		
		blockptr += EXFAT_ENTRY_SIZE;	
	}
	
	if(FileEntryCount >= FileEntry.entrycount) break;		/* at end */

	if(direntrycount >= (1 << superblock_info->BytesPerBlockShift)  / EXFAT_ENTRY_SIZE) {		/* at end of block */
		nextdirblock=exfat_get_next_block(splitbuf.drive,dirblock);	/* get next block */		
		if(nextdirblock == (uint64_t) 0xFFFFFFFFFFFFFFFF) {		/* at end of directory */
			kernelfree(blockbuf);

			setlasterror(END_OF_DIRECTORY);
			return(-1);
		}

		dirblock=(superblock_info->ClusterHeapOffset+nextdirblock) - 2;	/* get block number relative to cluster heap */	/* get block number relative to cluster heap */
	}

}

if(BlockUpdated == TRUE) {		/* write block if it was updated */
	if(dirblock == (buf.findlastblock-2)) SetExFATDirtyFlag();		/* set dirty flag for first write */

	if(blockio(DEVICE_WRITE,splitbuf.drive,((uint64_t) dirblock),blockbuf) == -1) { /* write directory block */
		kernelfree(blockbuf);
		return(-1);
	}
}
kernelfree(blockbuf);

setlasterror(NO_ERROR);  
return(NO_ERROR);				/* no error */
}

/*
 * Get ExFAT bitmap entry from root directory
 *
 * In:	drive	drive
	entry	Buffer to hold ExFAT bitmap entry
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t exfat_get_bitmap_information(size_t drive,ExFATBitmapEntry *entry) {
BLOCKDEVICE blockdevice;
uint64_t block;
ExFATBootSector *superblock_info;
void *blockbuf;
uint8_t *blockptr;
size_t dirblock;
size_t direntrycount;
size_t nextdirblock;

if(exfat_detect_change(drive) == -1) return(-1);
if(getblockdevice(drive,&blockdevice) == -1) return(-1);

blockbuf=kernelalloc(1 << superblock_info->BytesPerBlockShift);			/* allocate block buffer */ 
if(blockbuf == NULL) return(-1);

superblock_info=blockdevice.superblock;				/* point to superblock data */
block=superblock_info->RootDirectoryStart;			/* get root directory start block */

while(1) {
	blockptr=blockbuf+(dirblock*EXFAT_ENTRY_SIZE);	/* point to next file */

	if(blockio(DEVICE_READ,drive,((uint64_t) dirblock),blockbuf) == -1) { /* read directory block */
		kernelfree(blockbuf);
		return(-1);
	}

	for(direntrycount=0;direntrycount < (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE;direntrycount++) {
	
		if((uint8_t) *blockptr ==  EXFAT_BITMAP_ENTRY) {		/* bitmap entry */
			memcpy(entry,blockptr,EXFAT_ENTRY_SIZE);

			kernelfree(blockbuf);
			setlasterror(NO_ERROR);
			return(0);
		}
		
		blockptr += EXFAT_ENTRY_SIZE;	
	}

	if(direntrycount >= (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE) {		/* at end of block */
		nextdirblock=exfat_get_next_block(drive,dirblock);	/* get next block */		
		if(dirblock == (uint64_t) 0xFFFFFFFFFFFFFFFF) break;		/* at end of directory */

		dirblock=(superblock_info->ClusterHeapOffset+nextdirblock)-2;	/* get block number relative to cluster heap */
	}

}

kernelfree(blockbuf);
			
setlasterror(END_OF_DIRECTORY);
return(-1);
}

/*
 * Find free bitmap entries
 *
 * In:  drive			drive
 *	NumberOfEntries		Number of entries to find
 *
 * Returns: -1 on error, 0 on success
 *
 */
uint64_t exfat_find_free_bitmap_entries(size_t drive,size_t NumberOfEntries) {
ExFATBitmapEntry bitmap;
size_t count;
void *blockbuf;
size_t *blockptr;
ExFATBootSector *superblock_info;
size_t freefound=0;
size_t lastfreeblockfound;
size_t blocknumber=0;
BLOCKDEVICE blockdevice;
size_t bitmask;

if(exfat_get_bitmap_information(drive,&bitmap) == 0) return(-1);		/* get bitmap information */

if(getblockdevice(drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;			/* point to superblock information */

blockbuf=kernelalloc(1 << superblock_info->BytesPerBlockShift);			/* allocate block buffer */ 
if(blockbuf == NULL) return(-1);

/* Scan bitmap for free entries */

lastfreeblockfound=0;

for(count=bitmap.startblock;count < bitmap.startblock+(bitmap.length / (1 << superblock_info->BytesPerBlockShift));count++) {
	if(blockio(DEVICE_READ,drive,((uint64_t) count),blockbuf) == -1) { /* read bitmap block */
		kernelfree(blockbuf);
		return(-1);
	}

	for(bitmask=1 << (sizeof(size_t)*8)-1;bitmask > 0;bitmask >> 2) {
		if( ((size_t) *blockptr & bitmask) == 0) {		/* found free block */
			if(++freefound == NumberOfEntries) {		/* found enough */
				kernelfree(blockbuf);

				setlasterror(NO_ERROR);
				return(lastfreeblockfound);
			}
			
			lastfreeblockfound=blocknumber;
			freefound=0;			/* reset counter */
		}

		blocknumber++;
	}
}

setlasterror(END_OF_DIRECTORY);
return(-1);
}

/*
 * Update bitmap
 *
 * In:	drive		Drive
	entry		Bitmap entry to update
	SetOrClear	Set (1) or clear (0) bit
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t exfat_update_bitmap_entry(size_t drive,size_t entry,size_t SetOrClear) {
ExFATBitmapEntry bitmap;
size_t count;
void *blockbuf;
size_t *blockptr;
ExFATBootSector *superblock_info;
size_t blocknumber;
size_t bitnumber;
BLOCKDEVICE blockdevice;

if(exfat_get_bitmap_information(drive,&bitmap) == 0) return(-1);		/* get bitmap information */

if(getblockdevice(drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;			/* point to superblock information */

blockbuf=kernelalloc(1 << superblock_info->BytesPerBlockShift);			/* allocate block buffer */ 
if(blockbuf == NULL) return(-1);

blocknumber=entry / (1 << superblock_info->BytesPerBlockShift);		/* get block number */
bitnumber=entry % (1 << superblock_info->BytesPerBlockShift);		/* get bit number */

if(blockio(DEVICE_READ,drive,((uint64_t) blocknumber),blockbuf) == -1) { /* read bitmap block */
	kernelfree(blockbuf);
	return(-1);
}

blockptr=blockbuf+(bitnumber*(1 << superblock_info->BytesPerBlockShift));

*blockptr ^= SetOrClear;

if(blockio(DEVICE_WRITE,drive,((uint64_t) blocknumber),blockbuf) == -1) { /* write bitmap block */
	kernelfree(blockbuf);
	return(-1);
}

setlasterror(NO_ERROR);
return(0);
}

/*
 * Get bitmap entry
 *
 * In:	drive		Drive
	entry		Entry number
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t exfat_get_bitmap_entry(size_t drive,size_t entry) {
ExFATBitmapEntry bitmap;
size_t count;
void *blockbuf;
size_t *blockptr;
ExFATBootSector *superblock_info;
size_t blocknumber;
size_t bitnumber;
size_t bitmask;
BLOCKDEVICE blockdevice;

if(exfat_get_bitmap_information(drive,&bitmap) == 0) return(-1);		/* get bitmap information */

if(getblockdevice(drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;			/* point to superblock information */

blockbuf=kernelalloc(1 << superblock_info->BytesPerBlockShift);			/* allocate block buffer */ 
if(blockbuf == NULL) return(-1);

blocknumber=entry / (1 << superblock_info->BytesPerBlockShift);		/* get block number */
bitnumber=entry % (1 << superblock_info->BytesPerBlockShift);		/* get bit number */

if(blockio(DEVICE_READ,drive,((uint64_t) blocknumber),blockbuf) == -1) { /* read bitmap block */
	kernelfree(blockbuf);
	return(-1);
}

blockptr=blockbuf+(bitnumber*(1 << superblock_info->BytesPerBlockShift));
bitmask=~(bitnumber * (1 << superblock_info->BytesPerBlockShift));

setlasterror(NO_ERROR);
return((size_t) *blockptr & bitmask);
}

/*
 * Convert bitmap to FAT entries
 *
 * In:	drive		Drive
	startentry	Bitmap entry to start at
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t exfat_convert_bitmap_to_fat_chain(size_t drive,size_t startentry) {
ExFATBitmapEntry bitmap;
size_t count;
BLOCKDEVICE blockdevice;
size_t fatblock=0;
ExFATBootSector *superblock_info;
size_t entry;
size_t previous_fat_block=-1;
uint64_t *fatptr;
size_t endblock;
char *blockbuf;
size_t fatentry;
char *iobuf;

if(exfat_get_bitmap_information(drive,&bitmap) == 0) return(-1);		/* get bitmap information */

if(getblockdevice(drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;			/* point to superblock information */

blockbuf=kernelalloc(1 << superblock_info->BytesPerBlockShift);			/* allocate block buffer */ 
if(blockbuf == NULL) return(-1);

/* Convert bitmap entries to FAT entries */

entry=startentry;
endblock=bitmap.startblock+(bitmap.length / (1 << superblock_info->BytesPerBlockShift));

for(count=bitmap.startblock;count < endblock+1;count++) {
	fatblock=superblock_info->FATOffset+(entry / (1 << superblock_info->BytesPerBlockShift));	/* FAT block */
	fatentry=entry % (1 << superblock_info->BytesPerBlockShift);		/* FAT entry */

	if(fatblock != previous_fat_block) {		/* need to read FAT block */
		if(blockio(DEVICE_READ,blockdevice.drive,fatblock,iobuf) == -1) {		/* read block */
			kernelfree(iobuf);
			return(-1);
		}
	}

	fatptr=blockbuf+fatentry;		/* point to FAT entry */

	if(count == endblock) {			/* last entry */
		*fatptr=(uint64_t) 0xFFFFFFFFFFFFFFFF;
	}
	else
	{
		*fatptr=(uint64_t) count+1;		/* next entry in chain */
	}
}

setlasterror(NO_ERROR);
return(0);
}

/*
 * Get start block of file or directory
 *
 * In:  name	File or directory name
 *
 * Returns: -1 on error, start block on success
 *
 */
size_t exfat_get_start_block(size_t drive,char *name) {
uint64_t rb;
size_t count;
SPLITBUF splitbuf;
char *fullpath[MAX_PATH];
size_t tc;
BLOCKDEVICE blockdevice;
char *blockbuf;
char *blockptr;
size_t tokencount;
ExFATBootSector *superblock_info;
size_t TokenFound=FALSE;
char *filename[MAX_PATH];
size_t NumberOfEntries=-1;
ExFATFileEntry FileEntry;
ExFATFileInfoEntry FileInfoEntry;
ExFATFilenameEntry FilenameEntry;
size_t nextblock;
char *FilenamePtr;

getfullpath(name,fullpath);		/* get full path */	
splitname(fullpath,&splitbuf);

if(exfat_detect_change(drive) == -1) return(-1);

if(getblockdevice(drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;		/* point to data */

blockbuf=kernelalloc(1 << superblock_info->BytesPerBlockShift);			/* allocate block buffer */
if(blockbuf == NULL) return(-1);

/* Read first block of directory to buffer
 *
 *  For each token in path:
 *
 *   Find start block of token
 *   Until block = 0xffffffffffffffffff
 *  	Read block
 *   	Search entries in block for name and return block number from FAT entry if found
 *   	Get next block in directory from FAT using exfat_get_next_block()
 *
 *  Return -1 if at end of directory and token not found
 *
 * Return block number if at end of name  
 */

tc=tokenize_line(fullpath,path_tokens,"\\");			/* tokenize filename */

rb=(superblock_info->ClusterHeapOffset + superblock_info->RootDirectoryStart) - 2;	/* get block number relative to cluster heap */

if(strncmp(splitbuf.dirname,"\\",MAX_PATH) == 0) {						           /* root directory */
	if(strlen(splitbuf.filename) == 0) { 
		kernelfree(blockbuf);
		return(rb);
	}
}

for(tokencount=1;tokencount < tc;tokencount++) {
	memset(filename,0,MAX_PATH);

	FilenamePtr=splitbuf.filename;
	
	do {
		if(blockio(DEVICE_READ,splitbuf.drive,((uint64_t) rb),blockbuf) == -1) { /* read directory block */
			kernelfree(blockbuf);
			return(-1);
		}

		/* search through each directory block */

		for(count=0;count < (1 << superblock_info->BytesPerBlockShift) / EXFAT_ENTRY_SIZE;count++) {
			if((uint8_t) *blockptr ==  EXFAT_ENTRY_INFO) {				/* file information */
				memcpy(&FileInfoEntry,blockptr,EXFAT_ENTRY_SIZE);
				NumberOfEntries=FileEntry.entrycount;
			}
			else if((uint8_t) *blockptr ==  EXFAT_ENTRY_FILENAME) {				/* filename */
				memcpy(&FilenameEntry,blockptr,EXFAT_ENTRY_SIZE);
				AppendUnicodeNameFromFilenameEntry(FilenamePtr,&FilenameEntry.name);		/* get filename entry */
			
				FilenamePtr += EXFAT_NAME_SEGMENT_LENGTH;

				NumberOfEntries--;
				tokencount++;		/* point to next token */
			}
			
	
			if(NumberOfEntries-- == 0) {
				if(wildcard(splitbuf.filename,filename) == 0) { 	/* if entry matches */  			    			
					if(tokencount == (tc - 1)) {				/* last token in path */
						setlasterror(NO_ERROR);
					  	kernelfree(blockbuf);
						return(rb);				/* at end, return block */
					}

					TokenFound=TRUE;
					rb=FileInfoEntry.startblock;
					break;

				}
			}

			if((uint8_t) *blockptr == 0) {						/* at end of directory */
				kernelfree(blockbuf);
			
				setlasterror(PATH_NOT_FOUND);
				return(-1);
			}

		}

		if(TokenFound == FALSE) nextblock=exfat_get_next_block(drive,rb);	/* at end of block and not found token */
			
	} while(rb != 0xffffffffffffffff);
}

kernelfree(blockbuf);

setlasterror(PATH_NOT_FOUND);
return(-1);
}

/*
 * Get next block in ExFAT chain
 *
 * In:  drive	Drive
	block	Block number
 *
 * Returns: -1 on error, block number on success
 *
 */


size_t exfat_get_next_block(size_t drive,uint64_t block) {
uint8_t *buf;
uint64_t blockno;
uint64_t entry;
size_t entryno;
size_t entry_offset;
BLOCKDEVICE blockdevice;
ExFATBootSector *superblock_info;

if(exfat_detect_change(drive) == -1) return(-1);

if(getblockdevice(drive,&blockdevice) == -1) return(-1);

buf=kernelalloc(1 << superblock_info->BytesPerBlockShift);
if(buf == NULL) return(-1);

superblock_info=blockdevice.superblock;		/* point to superblock_info */

entryno=block*8;				

blockno=superblock_info->FATOffset+(entry / (1 << superblock_info->BytesPerBlockShift));	/* FAT block */
entry_offset=entry % (1 << superblock_info->BytesPerBlockShift);		/* FAT entry */

if(blockio(DEVICE_READ,drive,blockno,buf) == -1) {			/* read block */
	kernelfree(buf);
	return(-1);
}

entry=*(uint64_t*)&buf[entry_offset];		/* get entry */

kernelfree(buf);
return(entry);
}

/*
 * Update FAT entry
 *
 * In:  drive		Drive
	block		Block number of FAT entry
	blockentry	New FAT entry
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t exfat_update_fat(size_t drive,uint64_t block,uint64_t blockentry) {
uint64_t *buf;
uint64_t blockno;
size_t count;
size_t entryno;
size_t entry_offset;
ExFATBootSector *superblock_info;
BLOCKDEVICE blockdevice;

if(exfat_detect_change(drive) == -1) return(-1);

if(getblockdevice(drive,&blockdevice) == -1) return(-1);

superblock_info=blockdevice.superblock;		/* point to superblock_info */

entryno=block*8;				
blockno=superblock_info->FATOffset+(entryno / (1 << superblock_info->BytesPerBlockShift));	/* FAT block */
entry_offset=entryno % (1 << superblock_info->BytesPerBlockShift);		/* FAT entry */

buf=kernelalloc(1 << superblock_info->BytesPerBlockShift);
if(buf == NULL) return(-1);

if(blockio(DEVICE_READ,drive,blockno,buf) == -1) {	 	/* read block */
	kernelfree(buf);
	return(-1);
}

buf[entry_offset]=(uint64_t) blockentry;	/* update FAT */

/* write FATs */

for(count=0;count < superblock_info->NumberOfFATs;count++) {
	if(blockio(DEVICE_WRITE,drive,blockno,buf) == -1) return(-1);

	blockno +=superblock_info->FATLength;		/* point to next FAT */
}

setlasterror(NO_ERROR);
return(NO_ERROR);
}

/*
 * Detect exFAT drive change
 *
 * In:  drive	Drive
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t exfat_detect_change(size_t drive) {
char *bootbuf;
BLOCKDEVICE blockdevice;

getblockdevice(drive,&blockdevice);		/* get device */ 

bootbuf=kernelalloc(MAX_BLOCK_SIZE);				/* allocate buffer */
if(bootbuf == NULL) return(-1); 


if(blockio(DEVICE_READ,drive,(uint64_t) 0,bootbuf) == -1) {				/* get bios Parameter Block */
	kernelfree(bootbuf);
	return(-1); 
}

if(blockdevice.superblock == NULL) {
	blockdevice.superblock=kernelalloc(sizeof(ExFATBootSector));	/* allocate buffer */
	if(blockdevice.superblock == NULL) {
		kernelfree(bootbuf);
		return(-1);
	}
}

memcpy(blockdevice.superblock,bootbuf,sizeof(ExFATBootSector));		/* update superblock_info */
update_block_device(drive,&blockdevice);			/* update block device information */

return(0);
}

