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
 * FAT12/FAT16 FAT32 Filesystem for CCP
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

size_t fat_findfirst(char *filename,FILERECORD *buf);				// TESTED
size_t fat_findnext(char *filename,FILERECORD *buf);				// TESTED
size_t fat_find(size_t find_type,char *filename,FILERECORD *buf);		// TESTED
size_t fat_rename(char *filename,char *newname);				// TESTED
size_t fat_rmdir(char *dirname);
size_t fat_mkdir(char *dirname);						// TESTED
size_t fat_create(char *filename);						// TESTED
size_t fat_delete(char *filename);						// TESTED
size_t fat_read(size_t handle,void *addr,size_t size);			// TESTED
size_t fat_write(size_t handle,void *addr,size_t size);			// TESTED
size_t fat_chmod(char *filename,size_t attribs);
size_t fat_setfiletimedate(char *filename,TIMEBUF *createtime,TIMEBUF *lastmodtime);
size_t fat_findfreeblock(size_t drive);
size_t fat_getstartblock(char *name);
size_t fat_getnextblock(size_t drive,uint64_t block);
size_t updatefat(size_t drive,uint64_t block,uint16_t block_high_word,uint16_t block_low_word);
size_t fat_detectchange(size_t drive);
size_t fat_convertfilename(char *filename,char *outname);
size_t fat_readlfn(size_t drive,uint64_t block,size_t entryno,FILERECORD *n);
size_t fat_writelfn(size_t drive,uint64_t block,size_t entryno,char *n,char *newname);
size_t createshortname(char *filename,char *out);
uint8_t createlfnchecksum(char *filename);
size_t deletelfn(char *filename,uint64_t block,size_t entry);
size_t createlfn(FILERECORD *newname,uint64_t block,size_t entry);
size_t fat_seek(size_t handle,size_t pos,size_t whence);
void fatentrytofilename(char *filename,char *out);
size_t fat_init(char *i);							// TESTED
size_t fat_create_int(size_t entrytype,char *filename);


/*
 * Initialize FAT filesystem module
 *
 * In:  initstr	Initialisation string
 *
 * Returns: nothing
 *
 */
size_t fat_init(char *initstr) {
FILESYSTEM fatfilesystem;

strcpy(fatfilesystem.name,"FAT");
strcpy(fatfilesystem.magicnumber,"FAT");
fatfilesystem.size=3;
fatfilesystem.location=0x36;
fatfilesystem.findfirst=&fat_findfirst;
fatfilesystem.findnext=&fat_findnext;
fatfilesystem.read=&fat_read;
fatfilesystem.write=&fat_write;
fatfilesystem.rename=&fat_rename;
fatfilesystem.delete=&fat_delete;
fatfilesystem.mkdir=&fat_mkdir;
fatfilesystem.rmdir=&fat_rmdir;
fatfilesystem.create=&fat_create;
fatfilesystem.chmod=&fat_chmod;
fatfilesystem.setfiletd=&fat_setfiletimedate;
fatfilesystem.getstartblock=&fat_getstartblock;
fatfilesystem.seek=&fat_seek;

register_filesystem(&fatfilesystem);
}

/*
 * Find first or next file in directory
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

size_t fat_findnext(char *filename,FILERECORD *buf) {
	return(fat_find(TRUE,filename,buf));
}

/*
 * Internal FAT find function
 *
 * In:  type	Type of find, 0=first file, 1=next file
	name	Filename or wildcard of file to find
					   buffer	Buffer to hold information about files
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t fat_find(size_t find_type,char *filename,FILERECORD *buf) {
size_t count;
size_t lfncount;
size_t fattype;
size_t rootdir;
size_t start_of_data_area;
char *fp[MAX_PATH];
char *file[11];
BLOCKDEVICE blockdevice;
DIRENT dirent;
size_t b;
char *blockbuf;
char *blockptr;
unsigned char c;
SPLITBUF *splitbuf;
FILERECORD *lfnbuf;
BPB *bpb;

lfnbuf=kernelalloc(sizeof(FILERECORD));	/* allocate split buffer */
if(lfnbuf == NULL) return(-1);

splitbuf=kernelalloc(sizeof(SPLITBUF));	/* allocate split buffer */
if(splitbuf == NULL) {
	kernelfree(lfnbuf);
	return(-1);
}

if(find_type == FALSE) {
	getfullpath(filename,fp);			/* get full filename */
	touppercase(fp);				/* convert to uppercase */

	splitname(fp,splitbuf);					/* split filename */

	buf->findentry=0;
	
	buf->findlastblock=fat_getstartblock(splitbuf->dirname);
	if(buf->findlastblock == -1) return(-1);

	strcpy(buf->searchfilename,fp); 
}
else
{
	splitname(buf->searchfilename,splitbuf);					/* split filename */

}

if(strlen(splitbuf->filename) == 0) strcpy(splitbuf->filename,"*");

fattype=fat_detectchange(splitbuf->drive);
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

bpb=blockdevice.superblock;		/* point to data */

blockbuf=kernelalloc(bpb->sectorsperblock*bpb->sectorsize);			/* allocate block buffers */ 
if(blockbuf == NULL) {
	kernelfree(splitbuf);
	kernelfree(lfnbuf);
	return(-1);
}

/*
 * If it's the root directory, then it's in a fixed position for fat12 and fat16.
			All other directories are located in the data area */

if(strcmp(splitbuf->dirname,"\\") != 0) { /* not root */
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

blockptr=blockbuf+(buf->findentry*FAT_ENTRY_SIZE);	/* point to next file */

if(blockio(_READ,splitbuf->drive,(uint64_t) buf->findlastblock+start_of_data_area,blockbuf) == -1) { /* read directory block */
	kernelfree(blockbuf);
	kernelfree(lfnbuf);
	kernelfree(splitbuf);
	return(-1);
}

/* search thorough block until filename found */

while(buf->findentry < ((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE)+1) {
	buf->findentry++;

	memcpy(&dirent,blockptr,FAT_ENTRY_SIZE);				/* copy entry */

	c=*blockptr;

	if(c == 0) {						/* end of directory */
		kernelfree(blockbuf);
		kernelfree(splitbuf);
		kernelfree(lfnbuf);
		setlasterror(END_OF_DIR);
		return(-1);
	}
							/* get first character of filename */
	if(c == 0xE5) {
		continue;							/* deleted file */
	}

	fatentrytofilename(dirent.filename,file);			/* convert filename */

	if(dirent.attribs == 0x0f) {			/* long filename */
		lfncount=fat_readlfn(splitbuf->drive,buf->findlastblock,buf->findentry-1,lfnbuf);

		buf->findentry += lfncount;

		blockptr=blockptr+(lfncount*FAT_ENTRY_SIZE);			/* point to next */
				
		if(wildcard(splitbuf->filename,lfnbuf->filename) == 0) { 	/* if file found */      	
			strcpy(buf->filename,lfnbuf->filename);

			buf->attribs=dirent.attribs;

			buf->filesize=dirent.filesize;
			buf->startblock=(dirent.block_high_word << 16)+(dirent.block_low_word);
			buf->drive=splitbuf->drive;
			buf->dirent=(buf->findentry-lfncount);
			buf->access=0;
			buf->currentpos=0;

			buf->timebuf.hours=(dirent.last_modified_time & 0xf800) >> 11;
			buf->timebuf.minutes=(dirent.last_modified_time  & 0x7e0) >> 6;
			buf->timebuf.seconds=(dirent.last_modified_time & 0x1f);
			buf->timebuf.year=((dirent.last_modified_date & 0xfe00) >> 9)+1980;
			buf->timebuf.month=(dirent.last_modified_date & 0x1e0) >> 5;
			buf->timebuf.day=(dirent.last_modified_date & 0x1f);

			if(buf->attribs & FAT_ATTRIB_DIRECTORY) {
				buf->flags=FILE_DIRECTORY;
			}
			else
			{
				buf->flags=FILE_REGULAR;
			}

/* get next block */

			if(fattype == 12 || fattype == 16) {
				buf->currentblock=(bpb->sectorsperfat*bpb->numberoffats) \
				+ (buf->startblock) - bpb->reservedsectors;
				+ ((bpb->rootdirentries*FAT_ENTRY_SIZE) / (bpb->sectorsperblock*bpb->sectorsize));
			}

			if(fattype == 32) {
				buf->currentblock=(bpb->fat32sectorsperfat*bpb->numberoffats) \
				+ (bpb->fat32rootdirstart*bpb->sectorsperblock) \
				+ (buf->startblock) - bpb->reservedsectors;
			}


			kernelfree(blockbuf);
			kernelfree(splitbuf);
			kernelfree(lfnbuf);
		
			setlasterror(NO_ERROR);

			return(0);
		}

}

			if(wildcard(splitbuf->filename,file) == 0) { 				/* if file found */     
				fatentrytofilename(dirent.filename,buf->filename);			/* convert filename */

				buf->attribs=dirent.attribs;

				buf->filesize=dirent.filesize;

				buf->startblock=(dirent.block_high_word << 16)+(dirent.block_low_word);
				buf->drive=splitbuf->drive;
				buf->dirent=buf->findentry-1;
				buf->access=0;
				buf->currentpos=0;

				buf->timebuf.hours=(dirent.last_modified_time & 0xf800) >> 11;
				buf->timebuf.minutes=(dirent.last_modified_time  & 0x7e0) >> 6;
				buf->timebuf.seconds=(dirent.last_modified_time & 0x1f);
				buf->timebuf.year=((dirent.last_modified_date & 0xfe00) >> 9)+1980;
				buf->timebuf.month=(dirent.last_modified_date & 0x1e0) >> 5;
				buf->timebuf.day=(dirent.last_modified_date & 0x1f);
	
				buf->drive=splitbuf->drive;		/* additional information */

				if(buf->attribs & FAT_ATTRIB_DIRECTORY) {
					buf->flags=FILE_DIRECTORY;
				}
				else
				{
					buf->flags=FILE_REGULAR;
				}

				if(fattype == 12 || fattype == 16) {
					buf->currentblock=(bpb->sectorsperfat*bpb->numberoffats) \
					+ ((bpb->rootdirentries*FAT_ENTRY_SIZE) / (bpb->sectorsperblock*bpb->sectorsize)) \
					+ (buf->startblock) - bpb->reservedsectors;
				}

				if(fattype == 32) {
					buf->currentblock=(bpb->fat32sectorsperfat*bpb->numberoffats) \
					+ (bpb->fat32rootdirstart*bpb->sectorsperblock) \
					+ blockdevice.startblock - bpb->reservedsectors;
				}

				kernelfree(splitbuf);
				kernelfree(blockbuf);
				kernelfree(lfnbuf);

				setlasterror(NO_ERROR);
				return(0);
			}

			blockptr=blockptr+FAT_ENTRY_SIZE;

		}

if(buf->findentry >= ((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE)+1) {		/* at end of block */

	buf->findlastblock=fat_getnextblock(splitbuf->drive,buf->findlastblock);	/* get next block */
	buf->findentry=0;

	if((fattype == 12 && buf->findlastblock >= 0xff0) || (fattype == 16 && buf->findlastblock >= 0xfff0) || (fattype == 32 && buf->findlastblock >= 0xfffffff0)) {
			kernelfree(blockbuf);
			kernelfree(splitbuf);

			setlasterror(END_OF_DIR);
			return(-1);
	}
}

kernelfree(blockbuf);
kernelfree(splitbuf);
kernelfree(lfnbuf);

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
size_t fat_rename(char *filename,char *newname) {
char *fullname[MAX_PATH];
char *newfullname[MAX_PATH];
size_t count;
uint64_t rb;
size_t fattype;
size_t lfncount;
BLOCKDEVICE blockdevice;
char *blockbuf;
char *blockptr;
uint32_t found=FALSE;
SPLITBUF *splitbuf;
SPLITBUF *newsplitbuf;
char *file[11];
char *newfile[11];
DIRENT dirent;
FILERECORD *lfnbuf;
BPB *bpb;

lfnbuf=kernelalloc(sizeof(FILERECORD));	/* allocate split buffer */
if(lfnbuf == NULL) return(-1);


getfullpath(filename,fullname);                 /* get full path */  
getfullpath(newname,newfullname);                 /* get full path */  


splitbuf=kernelalloc(sizeof(SPLITBUF));
if(splitbuf == NULL) {				/* can't allocate */
	kernelfree(lfnbuf);

	setlasterror(NO_MEM);
	return(-1);
}

newsplitbuf=kernelalloc(sizeof(SPLITBUF));
if(newsplitbuf == NULL) {				/* can't allocate */
	setlasterror(NO_MEM);
	return(-1);
}
	
splitname(fullname,splitbuf);			/* split name */
splitname(newfullname,newsplitbuf);			/* split name */

if(splitbuf->drive != newsplitbuf->drive || strcmp(splitbuf->dirname,newsplitbuf->dirname) != 0) {
	setlasterror(RENAME_ACR_DRIVE);
	kernelfree(splitbuf);
	kernelfree(newsplitbuf);
	return(-1);
}

fattype=fat_detectchange(splitbuf->drive);
if(fattype == -1) {
	kernelfree(splitbuf);
	kernelfree(lfnbuf);

	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(splitbuf->drive,&blockdevice) == -1) {
	kernelfree(splitbuf);
	kernelfree(newsplitbuf);

	setlasterror(INVALID_DISK);
	return(-1);
}

bpb=blockdevice.superblock;		/* point to data */


fat_convertfilename(splitbuf->filename,file);			/* convert to FAT format */

blockbuf=kernelalloc(bpb->sectorsperblock*bpb->sectorsize);

if(blockbuf == NULL) {			/* can't allocate */
	kernelfree(splitbuf);
	kernelfree(newsplitbuf);

	setlasterror(NO_MEM);
	return(-1);
}


rb=fat_getstartblock(splitbuf->dirname);		/* get directory start */
if(rb == -1) {
		kernelfree(splitbuf);
		kernelfree(newsplitbuf);
		return(-1);
}


blockptr=blockbuf;

if(blockio(_READ,splitbuf->drive,rb,blockbuf) == -1) {
		kernelfree(splitbuf);
		kernelfree(newsplitbuf);
		return(-1);
}

/* search through directory until filename found, and rename */

while((fattype == 12 && rb < 0xff0) || (fattype == 16 && rb < 0xfff0) || (fattype == 32 && rb < 0xfffffff0)) {	/* until end */

/* search through block to find entry, and rename if found */
	for(count=0;count<(bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE;count++) {
		memcpy(&dirent,blockptr,FAT_ENTRY_SIZE);
		blockptr=blockptr+FAT_ENTRY_SIZE;
	
		if(*dirent.filename == 0) {					/* end of directory */
			setlasterror(END_OF_DIR);
			return(-1);
		}

		fatentrytofilename(dirent.filename,file);			/* convert filename */




		if(dirent.attribs == 0x0f) {			/* long filename */
			lfncount=fat_readlfn(splitbuf->drive,rb,count,lfnbuf);

			if(lfncount != -1 &&  wildcard(splitbuf->filename,lfnbuf->filename) == 0) { 	/* if file found */

				fat_writelfn(splitbuf->drive,rb,count,fullname,newfullname);

				kernelfree(blockbuf);
				kernelfree(splitbuf);
				kernelfree(lfnbuf);

				setlasterror(NO_ERROR);

				return(NO_ERROR);
			}

			blockptr=blockptr+(lfncount*FAT_ENTRY_SIZE);		/* point to entry after lfn */
		}


		if(wildcard(filename,file) == 0) {					   /* file found */  
				wildcard_rename(filename,newname,newfile);	/* process wildcards in filename */

				fat_convertfilename(newfile,dirent.filename);	/* new name */    
				memcpy(blockptr-FAT_ENTRY_SIZE,&dirent,FAT_ENTRY_SIZE);

				if(blockio(_WRITE,splitbuf->drive,rb,blockbuf) == -1) {		/* write block */
					kernelfree(splitbuf);
					 kernelfree(newsplitbuf);
					setlasterror(NO_ERROR);
					return(NO_ERROR);
				}
		}
	}

	blockptr=blockbuf;
	
	rb=fat_getnextblock(splitbuf->drive,rb);			/* get next block */
	if(rb == -1) {
		kernelfree(splitbuf);
		kernelfree(newsplitbuf);
	
		return(-1);
	}

	blockptr=blockbuf;
			
	if(blockio(_READ,splitbuf->drive,rb,blockbuf) == -1) {
			kernelfree(splitbuf);
			kernelfree(newsplitbuf);

			return(-1);
	}
}

kernelfree(splitbuf);
kernelfree(newsplitbuf); 

setlasterror(FILE_NOT_FOUND);
return(-1);
}

/*
 * Remove directory
 *
 * In:  name	Directory to remove
 *
 * Returns: -1 on error, 0 on success
 *
 * Stub function to call fat_delete */

size_t fat_rmdir(char *dirname) {
return(fat_delete(dirname));
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

size_t fat_create(char *dirname) {
return(fat_create_int(_FILE,dirname));
}

/*
 * Create fat entry internal function
 *
 * In:  entrytype	Type of entry (0=file,1=directory)
	name		File to create
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_create_int(size_t entrytype,char *filename) {
size_t drive;
size_t subdir;
size_t dirstart;
char *fullname[MAX_PATH];
uint64_t rb;
unsigned fattype;
size_t block;
size_t count;
size_t blockcount;
size_t entrycount;
BLOCKDEVICE blockdevice;
TIMEBUF timebuf;
SPLITBUF *splitbuf;
DIRENT dirent;
char *blockbuf;
char *blockptr;
uint16_t time;
uint16_t date;
size_t flen;
FILERECORD findbuf;
BPB *bpb;
uint32_t start_of_data_area;
uint32_t thisdir_start_of_data_area;
uint32_t rootdir;
char *cwd[MAX_PATH];
SPLITBUF cwdsplit;
unsigned char c;

splitbuf=kernelalloc(sizeof(SPLITBUF));
if(splitbuf == NULL) {				/* can't allocate */
	kernelfree(blockbuf);

	setlasterror(NO_MEM);
	return(-1);
}

blockptr=blockbuf;

splitname(filename,splitbuf);				/* split name */

fattype=fat_detectchange(splitbuf->drive);
if(fattype == -1) {
	kernelfree(blockbuf);
	kernelfree(splitbuf);
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(splitbuf->drive,&blockdevice) == -1) {
	kernelfree(blockbuf);
	kernelfree(splitbuf);
	setlasterror(INVALID_DISK);			/* drive doesn't exist */
	return(-1);
} 

bpb=blockdevice.superblock;		/* point to data */

blockbuf=kernelalloc((bpb->sectorsperblock*bpb->sectorsize));
if(blockbuf == NULL) {				/* can't allocate */
	kernelfree(blockbuf);

	setlasterror(NO_MEM);
	return(-1);
}

getfullpath(filename,fullname);			/* get full path */

if(findfirst(filename,&findbuf) != -1) {	/* check if file exists */
	kernelfree(blockbuf);
	kernelfree(splitbuf);

	setlasterror(FILE_EXISTS);
	return(-1);
}

blockptr=blockbuf;
rb=fat_getstartblock(splitbuf->dirname);		/* get directory start */
if(rb == -1) {
	kernelfree(splitbuf);
	return(-1);
}

if(blockio(_READ,splitbuf->drive,rb,blockbuf) == -1) { /* read block */
	kernelfree(blockbuf);
	kernelfree(splitbuf);

	return(-1);
}
	
gettime(&timebuf);				/* get time and date */

time=(timebuf.hours << 11)+(timebuf.minutes << 5)+timebuf.seconds;
date=(timebuf.year << 9)+(timebuf.month << 5)+timebuf.day;


entrycount=0;
flen=strlen(filename)/13;		/* get number of 13 byte sections */

if((strlen(filename) % 13) > 0) flen++;

/* search through directory until free entry found and create file  */

while((fattype == 12 && rb < 0xff0) || (fattype == 16 && rb < 0xfff0) || (fattype == 32 && rb < 0xfffffff0)) {	/* until end */

/* loop through block until free entry found */

	for(count=0;count<bpb->sectorsperblock*bpb->sectorsize;count++) {

		c=*blockptr;

		if(c == 0) goto makeentry;			/* at end of directory found enough */

		if(c == 0xE5) {		/* free entry */
			entrycount++;
		}
		else
		{
			entrycount=0;					/* entries must be contingous */
		}


		if(entrycount >= flen) goto makeentry;		/* make entry */

		blockptr=blockptr+FAT_ENTRY_SIZE;
	}

	if(strcmp(splitbuf->dirname,"\\") != 0) rb += start_of_data_area;		/* If not root directory, add start of data area */

	if(blockio(_READ,splitbuf->drive,rb,blockbuf) == -1) {	/* read block */
		kernelfree(blockbuf);
		kernelfree(splitbuf);

		return(-1);
	}

/* next block */ 

	rb=fat_getnextblock(splitbuf->drive,rb);				/* get next block */
	if(rb == -1) {						/* at end,find free block */

/* fat12 and fat16 are limited to 128 entries in the root directory */
	
		if(strcmp(splitbuf->dirname,"\\") == 0) {
			if(fattype == 12 || fattype == 16) {
				if(entrycount == bpb->rootdirentries) {
					setlasterror(DIRECTORY_FULL);

					kernelfree(splitbuf);
					kernelfree(blockbuf);
					return(-1);
				}
			}
		}

		rb=fat_findfreeblock(splitbuf->drive);
			if(rb == -1) {						/* at end,find free block */
				kernelfree(blockbuf);
				kernelfree(splitbuf);
				return(-1);
			}

	}

	blockptr=blockbuf;
} 
	
	makeentry:


	if(entrytype == _FILE) {
		if(strlen(filename) > 11) {		/* create long filename */
			memset(&findbuf,0,sizeof(dirent));

			strcpy(findbuf.filename,splitbuf->filename);
			memcpy(&findbuf.timebuf,&timebuf,sizeof(TIMEBUF));
			findbuf.startblock=-1;

			createlfn(&findbuf,rb,count);

			return(0);
	}

	fat_convertfilename(splitbuf->filename,dirent.filename);	/* get filename */
	dirent.attribs=0;
	dirent.createtime=time;
	dirent.createdate=date;
	dirent.last_modified_time=time;
	dirent.last_modified_date=date;

	dirent.filesize=0;

	if(fattype == 32) { 
		dirent.block_high_word=0xffff;
		dirent.block_low_word=0xffff;	/* entry */
	}
				
	if(fattype == 12) dirent.block_low_word=0xfff;  /* entry */
	if(fattype == 16) dirent.block_low_word=0xffff;

	memcpy(blockptr,&dirent,FAT_ENTRY_SIZE);
		
	if(blockio(_WRITE,splitbuf->drive,rb,blockbuf) == -1) {	/* write block */
		kernelfree(splitbuf);
		kernelfree(blockbuf);

		return(-1); 
	}
			
	return(0);
}

	if(entrytype == _DIR) {
		getcwd(cwd);
		splitname(cwd,&cwdsplit);

		if(fattype == 12 || fattype == 16) {
			rootdir=((bpb->rootdirentries*FAT_ENTRY_SIZE)+(bpb->sectorsperblock-1))/bpb->sectorsize;
			start_of_data_area=rootdir+(bpb->reservedsectors+(bpb->numberoffats*bpb->sectorsperfat))-2; /* get start of data area */
		}
		else
		{
			start_of_data_area=bpb->fat32rootdirstart+(bpb->reservedsectors+(bpb->numberoffats*bpb->fat32sectorsperfat))-2;
		}

		if(strcmp(cwdsplit.dirname,"\\") != 0) { /* not root */
			thisdir_start_of_data_area=start_of_data_area;
		}
		else
		{
			thisdir_start_of_data_area=0;
		}

		if(strlen(filename) > 11) {		/* create long filename */
			memset(&findbuf,0,sizeof(FILERECORD));

			strcpy(findbuf.filename,splitbuf->filename);
			memcpy(findbuf.timebuf,&timebuf,sizeof(TIMEBUF));

			return(createlfn(&findbuf,rb,count));
		}

		findbuf.startblock=-1;
		memset(&dirent,0,sizeof(DIRENT));

		fat_convertfilename(splitbuf->filename,dirent.filename);	/* get filename */

		dirent.createtime=time;
		dirent.createdate=date;
		dirent.last_modified_time=time;
		dirent.last_modified_date=date;
		dirent.attribs=FAT_ATTRIB_DIRECTORY;

		subdir=fat_findfreeblock(splitbuf->drive);		/* find free block */
		if(subdir == -1) {				/* no blocks */
			kernelfree(splitbuf);
			kernelfree(blockbuf);
			return(-1);
		}

	if(fattype == 32) { 
		dirent.block_high_word=(subdir >> 16);			/* start block */
		dirent.block_low_word=(uint16_t) (subdir  & 0xffff);
		updatefat(splitbuf->drive,subdir,0xffff,0xfff0);         
	}

	if(fattype == 12) {
		dirent.block_high_word=0;
		dirent.block_low_word=(uint16_t) subdir;

		updatefat(splitbuf->drive,subdir,0,0xfff0);
	}

	if(fattype == 16) {
		dirent.block_low_word=(uint16_t) subdir;
		updatefat(splitbuf->drive,subdir,0,0xfff0);
	}

	memcpy(blockptr,&dirent,FAT_ENTRY_SIZE);

	if(blockio(_WRITE,splitbuf->drive,rb,blockbuf) == -1) {	/* write block */
				kernelfree(splitbuf);
				kernelfree(blockbuf);
			return(-1);	
}
		
	/* create first two entries in subdirectory ( . and ..) */

					   memset(blockbuf,0,bpb->sectorsperblock*bpb->sectorsize);

	for(count=0;count<2;count++) {
		if(count == 0) strcpy(dirent.filename,".          ");
		if(count == 1) strcpy(dirent.filename,"..         ");

		dirent.attribs=FAT_ATTRIB_DIRECTORY;				/* attributes */
		dirent.reserved=NULL;
		dirent.create_time_fine_resolution=0;
		dirent.createtime=time;
		dirent.createdate=date;
		dirent.last_modified_time=dirent.createtime;
		dirent.last_modified_date=dirent.createdate;
		dirent.filesize=0;


		if(fattype == 32) { 
			if(count == 0) {
				dirent.block_high_word=(subdir << 16);			/* start block */
				dirent.block_low_word=(subdir & 0xffff);
			}
			else
			{
				dirent.block_high_word=0;				/* start block */
				dirent.block_low_word=0;
			}
		}

		if(fattype == 12 || fattype == 16) {
			if(count == 0) {
				dirent.block_low_word=subdir;
			}
			else
			{
				dirent.block_low_word=0;
			}
					    }

			memcpy(blockbuf+(count*FAT_ENTRY_SIZE),&dirent,FAT_ENTRY_SIZE*2);
		}

		if(blockio(_WRITE,splitbuf->drive,(uint64_t) subdir+start_of_data_area,blockbuf) == -1) {	/* write block */
			kernelfree(splitbuf);
			kernelfree(blockbuf);

			return(-1);	
		}
	}
}

/*
 * Delete file
 *
 * In:  name	File to delete
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_delete(char *filename) {
char *fullname[MAX_PATH];
size_t fattype;
size_t lfncount;
size_t count;
uint64_t rb;
BLOCKDEVICE blockdevice;
char *blockbuf;
char *blockptr;
size_t found=FALSE;
SPLITBUF *splitbuf;    
char *file[11];
char *b;
DIRENT dirent;
FILERECORD *lfnbuf;
BPB *bpb;

lfnbuf=kernelalloc(sizeof(FILERECORD));	/* allocate buffer */
if(lfnbuf == NULL) return(-1);


splitbuf=kernelalloc(sizeof(SPLITBUF));
if(splitbuf == NULL) {				/* can't allocate */
	kernelfree(lfnbuf);
	setlasterror(NO_MEM);
	return(-1);
}

splitname(filename,splitbuf);				/* split name */

fattype=fat_detectchange(splitbuf->drive);
if(fattype == -1) {
	kernelfree(splitbuf);
	kernelfree(lfnbuf);
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(splitbuf->drive,&blockdevice) == -1) {
	kernelfree(lfnbuf);
	setlasterror(INVALID_DISK);
	kernelfree(splitbuf);
	return(NO_ERROR);
}

bpb=blockdevice.superblock;		/* point to data */


blockbuf=kernelalloc((bpb->sectorsperblock*bpb->sectorsize));
if(blockbuf == NULL) {				/* can't allocate */
	kernelfree(blockbuf);
	kernelfree(splitbuf);
	kernelfree(lfnbuf);

	setlasterror(NO_MEM);
	return(-1);
}

rb=fat_getstartblock(splitbuf->dirname);		/* get directory start */

	blockptr=blockbuf;
	if(blockio(_READ,splitbuf->drive,rb,blockbuf) == -1) { /* block i/o */
		kernelfree(lfnbuf); 
		kernelfree(blockbuf);
		kernelfree(splitbuf);
		return(-1);
	}

/* search through directory until filename found, and mark as deleted */

while((fattype == 12 && rb < 0xff0) || (fattype == 16 && rb < 0xfff0) || (fattype == 32 && rb < 0xfffffff0)) {	/* until end */

/* search through block to find entry, and mark asdeleted if found */

	for(count=0;count<(bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE;count++) {
		memcpy(&dirent,blockptr,FAT_ENTRY_SIZE);

		
		if(dirent.attribs == 0x0f) {	

			lfncount=fat_readlfn(splitbuf->drive,rb,count,lfnbuf);
			if(lfncount == -1) {
				kernelfree(lfnbuf); 
				kernelfree(blockbuf);
				kernelfree(splitbuf);

				setlasterror(FILE_NOT_FOUND);
				return(-1);
			}

			if(wildcard(lfnbuf->filename,splitbuf->filename) == 0) deletelfn(splitbuf->filename,rb,count);
	
			blockptr=blockptr+(lfncount*FAT_ENTRY_SIZE);
		}

		blockptr=blockptr+FAT_ENTRY_SIZE;

		fatentrytofilename(dirent.filename,file);			/* convert to FAT format */
	
			
		if(*dirent.filename == 0) {					/* end of directory */
			setlasterror(END_OF_DIR);
			return(-1);
		}

		if(wildcard(file,splitbuf->filename) == 0) {					   /* file found */  

			blockptr=blockptr-FAT_ENTRY_SIZE;
			*blockptr=0xE5;			/* mark as deleted */

			if(blockio(_WRITE,splitbuf->drive,rb,blockbuf) == -1) {		/* write block */
				kernelfree(splitbuf);
				return(-1);
			}

			setlasterror(NO_ERROR);
			return(NO_ERROR);
		}
	}
}

return(NO_ERROR);
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
size_t  block;
char *ioaddr;
char *iobuf;
size_t bytesdone;
size_t fattype;
BLOCKDEVICE blockdevice;
TIMEBUF timebuf;
FILERECORD onext;
size_t count;
size_t blockoffset;
uint64_t blockno;
size_t datastart;
size_t rootdirsize;
size_t fatsize;
size_t  blockcount;
BPB *bpb;
size_t cr;

/*
 * find handle in struct
 */			

if(gethandle(handle,&onext) == -1) {	/* bad handle */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

/* get drive struct for the file */

fattype=fat_detectchange(onext.drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(onext.drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(NULL);
}

bpb=blockdevice.superblock;		/* point to data */

if(onext.filesize == 0) {				/* read from 0 byte file */
	setlasterror(END_OF_FILE);
	return(-1);
}

iobuf=kernelalloc(bpb->sectorsperblock*bpb->sectorsize);
if(iobuf == NULL) {
	kernelfree(iobuf);
	return(-1);
}

/* find block to read from */

if(fattype == 12 || fattype == 16) {
	fatsize=(bpb->sectorsperfat*bpb->numberoffats);
	rootdirsize=((bpb->rootdirentries*FAT_ENTRY_SIZE)+bpb->sectorsperblock-1)/bpb->sectorsize;
	datastart=bpb->reservedsectors+fatsize+rootdirsize;
	blockno=((onext.currentblock-2)*bpb->sectorsperblock)+datastart;
}

if(fattype == 32) {
	fatsize=(bpb->fat32sectorsperfat*bpb->numberoffats);
	datastart=bpb->reservedsectors+fatsize+bpb->fat32rootdirstart;
	blockno=((onext.currentblock-2)*bpb->sectorsperblock)+datastart;
}

ioaddr=addr;					/* point to buffer */
bytesdone=0;

/* starting in a block */
blockoffset=onext.currentpos % (bpb->sectorsperblock*bpb->sectorsize);	/* distance inside the block */

if(size < (bpb->sectorsperblock*bpb->sectorsize)) {
	if(blockio(_READ,onext.drive,blockno,iobuf) == -1) return(-1);

	count=(bpb->sectorsperblock*bpb->sectorsize)-blockoffset;

	memcpy(addr,iobuf+blockoffset,count);	/* copy from buffer */

	addr=addr+count;

	if(onext.currentpos+size >= (bpb->sectorsperblock*bpb->sectorsize)) {

		onext.currentblock=fat_getnextblock(onext.drive,onext.currentblock);		/* get blockdevice block */	
		updatehandle(handle,&onext);

		if((fattype == 12 && onext.currentblock >= 0xff8) || (fattype == 16 && onext.currentblock >= 0xfff0) || (fattype == 32 && onext.currentblock >= 0xfffffff0)) {
			onext.currentpos += count;

			updatehandle(handle,&onext);

			setlasterror(END_OF_FILE);
			return(-1);
		}
	}

	onext.currentpos += count;
	updatehandle(handle,&onext);
	return(size);
}

/*
 * read rest of blocks
 */
		
for(blockcount=0;blockcount< (size / (bpb->sectorsperblock*bpb->sectorsize));blockcount++) {    
	blockoffset=onext.currentpos % (bpb->sectorsperblock*bpb->sectorsize);	/* distance inside the block */
	count=(bpb->sectorsperblock*bpb->sectorsize)-blockoffset;

	if(blockio(_READ,onext.drive,blockno,iobuf) == -1) return(-1);

	memcpy(addr,iobuf+blockoffset,count);


	onext.currentblock=fat_getnextblock(onext.drive,onext.currentblock);		/* get blockdevice block */	
	updatehandle(handle,&onext);

	if(fattype == 12 || fattype == 16) {
		fatsize=(bpb->sectorsperfat*bpb->numberoffats);
		rootdirsize=((bpb->rootdirentries*FAT_ENTRY_SIZE)+bpb->sectorsperblock-1)/bpb->sectorsize;
		datastart=bpb->reservedsectors+fatsize+rootdirsize;
		blockno=((onext.currentblock-2)*bpb->sectorsperblock)+datastart;
	}

	if(fattype == 32) {
		fatsize=(bpb->fat32sectorsperfat*bpb->numberoffats);
		datastart=bpb->reservedsectors+fatsize+bpb->fat32rootdirstart;
		blockno=((onext.currentblock-2)*bpb->sectorsperblock)+datastart;
	}

	addr=addr+(bpb->sectorsperblock*bpb->sectorsize);      /* point to blockdevice address */

	onext.currentpos += count;
	updatehandle(handle,&onext);

	bytesdone=bytesdone+(bpb->sectorsperblock*bpb->sectorsize); /* increment number of bytes read */ 

	if((fattype == 12 && onext.currentblock >= 0xff0) || (fattype == 16 && onext.currentblock >= 0xfff0) || (fattype == 32 && onext.currentblock >= 0xfffffff0)) {
		setlasterror(END_OF_FILE);
		return(-1);
	}
}


/* end block with less than a block of data */

if(size % (bpb->sectorsperblock*bpb->sectorsize)) {
	if(blockio(_READ,onext.drive,blockno,iobuf) == -1) return(-1);

	count=(size % (bpb->sectorsperblock*bpb->sectorsize));			/* remainder */

	onext.currentpos=onext.currentpos+(bpb->sectorsperblock*bpb->sectorsize); /* increment number of bytes read */ 
	updatehandle(handle,&onext);

	bytesdone=bytesdone+count;

	memcpy(addr,iobuf,count);	/* copy from buffer */
	addr=addr+count;

	kernelfree(iobuf);

	return(size);
}

onext.currentblock=fat_getnextblock(onext.drive,onext.currentblock);

setlasterror(NO_ERROR);

onext.findblock=block;		/* save block  */

return(bytesdone);				/* return number of bytes */

}

/*
 * Write to file
 *
 * In:  handle	File handle
	addr	Buffer to read to
	size	Number of bytes to read
 *
 * Returns: -1 on error, number of bytes written on success
 *
 */

size_t fat_write(size_t handle,void *addr,size_t size) {
size_t block;
char *ioaddr;
char *iobuf;
size_t bytesdone;
size_t fattype;
BLOCKDEVICE blockdevice;
TIMEBUF timebuf;
FILERECORD onext;
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
uint32_t lastblock;
uint32_t sblock;

if(gethandle(handle,&onext) == -1) {	/* bad handle */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

fattype=fat_detectchange(onext.drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(onext.drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(NULL);
}


bpb=blockdevice.superblock;		/* point to data */

/* find block to read from */

if(fattype == 12 || fattype == 16) {
	fatsize=(bpb->sectorsperfat*bpb->numberoffats);

	rootdirsize=((bpb->rootdirentries*FAT_ENTRY_SIZE)+bpb->sectorsperblock-1)/bpb->sectorsize;
	datastart=bpb->reservedsectors+fatsize+rootdirsize;

	blockno=((onext.currentblock-2)*bpb->sectorsperblock)+datastart;
}

if(fattype == 32) {
	fatsize=(bpb->fat32sectorsperfat*bpb->numberoffats);
	datastart=bpb->reservedsectors+fatsize+bpb->fat32rootdirstart;
	blockno=((onext.currentblock-2)*bpb->sectorsperblock)+datastart;
}


iobuf=kernelalloc(bpb->sectorsperblock*bpb->sectorsize);

if(iobuf == NULL) {
	kernelfree(iobuf);
	return(-1);
}

/* check if enough space before write */ 

if(count > (bpb->sectorsperblock*bpb->sectorsize)) {
	count=size / (bpb->sectorsperblock*bpb->sectorsize);
}
else
{
	count=1;
}

while(count-- > 0) {
	newblock=fat_findfreeblock(onext.drive);
	if(newblock == -1) break;		/* not enough free space */
}

if(count == 0) {
	setlasterror(DISK_FULL);
	return(-1);
}	

/* writing past end of file, add new blocks to FAT chain only needs to be done once per write past end */ 

if((fattype == 12 && onext.currentblock >= 0xff0) || (fattype == 16 && onext.currentblock >= 0xfff0) || (fattype == 32 && onext.currentblock >= 0xfffffff0)) {
	/* empty file */

	onext.filesize += size;
	

	for(count=0;count< (size / (bpb->sectorsperblock*bpb->sectorsize))+1;count++) {
		newblock=fat_findfreeblock(onext.drive);

		if(count == 0) {
			lastblock=newblock;			/* first block in chain */ 	
			sblock=newblock;

			onext.startblock=newblock;
			onext.currentblock=newblock;			
		}	

		updatefat(onext.drive,lastblock,(newblock >> 16),(newblock & 0xffff));	/* update fat */

		lastblock=newblock;
	}

	/* end of fat chain */
	if(fattype == 12) {
		updatefat(onext.drive,lastblock,0,0xff0);
	}
	else if(fattype == 16) {
		updatefat(onext.drive,lastblock,0,0xfff0);
	}
	else if(fattype == 32) {
		updatefat(onext.drive,lastblock,0xffff,0xfff0);
	}

	/* update start block and file size in directory entry */
	if(strlen(onext.filename) > 11) {	/* long filename */
		createlfn(&onext,onext.findlastblock,onext.dirent);
	}
	else
	{
		blockbuf=kernelalloc(MAX_BLOCK_SIZE);
		if(blockbuf == NULL) return(-1);

		if(blockio(_READ,onext.drive,onext.findlastblock,blockbuf) == -1) return(-1); /* read block */

		memcpy(&dirent,blockbuf+(onext.dirent*FAT_ENTRY_SIZE),FAT_ENTRY_SIZE);	
		
		if(dirent.filesize == 0) {			/* first block */
			dirent.block_high_word=(sblock & 0xffff0000) >> 16;
			dirent.block_low_word=(sblock & 0xffff);
		}

		dirent.filesize += size;

		memcpy(blockbuf+(onext.dirent*FAT_ENTRY_SIZE),&dirent,FAT_ENTRY_SIZE);

		if(blockio(_WRITE,onext.drive,onext.findlastblock,blockbuf) == -1) return(-1); /* write block */

		kernelfree(blockbuf);

		/* update blockno */
		if(fattype == 12 || fattype == 16) blockno=((onext.currentblock-2)*bpb->sectorsperblock)+datastart;
		if(fattype == 32) blockno=((onext.currentblock-2)*bpb->sectorsperblock)+datastart;
	}
	
}

ioaddr=addr;					/* point to buffer */

/* starting in a block */

blockoffset=onext.currentpos % (bpb->sectorsperblock*bpb->sectorsize);	/* distance inside the block */

if(size < (bpb->sectorsperblock*bpb->sectorsize)) {

	if(blockio(_READ,onext.drive,blockno,iobuf) == -1) return(-1);
	count=(bpb->sectorsperblock*bpb->sectorsize)-blockoffset;

	memcpy(iobuf+blockoffset,addr,size);	/* copy to buffer */

	if(blockio(_WRITE,onext.drive,blockno,iobuf) == -1) return(-1);

	if(onext.currentpos+size >= (bpb->sectorsperblock*bpb->sectorsize)) {
		onext.currentblock=fat_getnextblock(onext.drive,onext.currentblock);		/* get next block */	
		updatehandle(handle,&onext);

		if((fattype == 12 && onext.currentblock >= 0xff0) || (fattype == 16 && onext.currentblock >= 0xfff0) || (fattype == 32 && onext.currentblock >= 0xfffffff0)) {
			onext.currentpos += count;
			setlasterror(END_OF_FILE);

			return(-1);
		}
	}

	onext.currentpos += count;
	updatehandle(handle,&onext);

	return(size);
}


for(count=0;count< (size / (bpb->sectorsperblock*bpb->sectorsize))+1;count++) {

	blockoffset=onext.currentpos % (bpb->sectorsperblock*bpb->sectorsize);	/* distance inside the block */
	count=(bpb->sectorsperblock*bpb->sectorsize)-blockoffset;

	memcpy(iobuf+blockoffset,addr,count);
		
	if(blockio(_WRITE,onext.drive,blockno,iobuf+blockoffset) == -1) {
		kernelfree(iobuf);

		return(-1);
	}

	if(fattype == 12 || fattype == 16) {
		fatsize=(bpb->sectorsperfat*bpb->numberoffats);
		rootdirsize=((bpb->rootdirentries*FAT_ENTRY_SIZE)+bpb->sectorsperblock-1)/bpb->sectorsperblock;
		datastart=bpb->reservedsectors+fatsize+rootdirsize;
		blockno=((onext.currentblock-2)*bpb->sectorsperblock)+datastart;
	}

	if(fattype == 32) {
		fatsize=(bpb->fat32sectorsperfat*bpb->numberoffats);
		datastart=bpb->reservedsectors+fatsize+bpb->fat32rootdirstart;
		blockno=((onext.currentblock-2)*bpb->sectorsperblock)+datastart;
	}


	onext.currentblock=fat_getnextblock(onext.drive,onext.currentblock);		/* get next block */	

	ioaddr=ioaddr+bpb->sectorsperblock*bpb->sectorsize;      /* point to next address */
	onext.currentpos=onext.currentpos+(bpb->sectorsperblock*bpb->sectorsize); /* increment number of bytes read */
	bytesdone=bytesdone+(bpb->sectorsperblock*bpb->sectorsize); /* increment number of bytes read */ 
}

/* file is smaller than a block or is an end block with less than a block of data */

if(size < (bpb->sectorsperblock*bpb->sectorsize) || size % (bpb->sectorsperblock*bpb->sectorsize)) {
	ioaddr=kernelalloc((size % bpb->sectorsperblock*bpb->sectorsize));				/* allocate buffer */
	if(ioaddr == NULL) return(-1);

	onext.currentblock=fat_getnextblock(onext.drive,onext.currentblock);		/* get next block */
	updatehandle(handle,&onext);

	if((fattype == 12 && onext.currentblock >= 0xff0) || (fattype == 16 && onext.currentblock >= 0xfff0) || (fattype == 32 && onext.currentblock >= 0xfffffff0)) {
		last=onext.currentblock;
		onext.currentblock=fat_findfreeblock(onext.drive);
		updatefat(onext.drive,last,(onext.currentblock & 0xffff0000) >> 16,(onext.currentblock >> 8));
	}

	if(blockio(_READ,onext.drive,onext.currentblock,ioaddr) == -1) {			/* readblock */
		kernelfree(ioaddr);
		return(-1);
	}

	count=size-(bpb->sectorsperblock*bpb->sectorsize);			/* remainder */
	bytesdone=bytesdone+count;
	memcpy(addr+count,addr,count);	/* copy to buffer */

	if(blockio(_WRITE,onext.drive,onext.currentblock,ioaddr) == -1) {			/* readblock */
		kernelfree(ioaddr);
		return(-1);
	}

	kernelfree(ioaddr);
	return;
}

onext.currentblock=fat_getnextblock(onext.drive,onext.currentblock-(bpb->sectorsperfat*bpb->numberoffats)-((bpb->rootdirentries*FAT_ENTRY_SIZE)/bpb->sectorsperblock)-bpb->reservedsectors+2);
updatehandle(handle,&onext);

setlasterror(NO_ERROR);

return(bytesdone);				/* return number of bytes */
}

/*
 * Set file attributes
 *
 * In:  name	File to change attributes
					   attribs Attributes. See fat.h for attributes
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_chmod(char *filename,size_t attribs) {
SPLITBUF splitbuf;
DIRENT dirent;
FILERECORD buf;
char *blockbuf;
FILERECORD *lfnbuf;
char *b;

lfnbuf=kernelalloc(sizeof(FILERECORD));	/* allocate split buffer */
if(lfnbuf == NULL) return(-1);

if((attribs & FAT_ATTRIB_DIRECTORY)) {		/* can't set directory bit */
	kernelfree(lfnbuf); 

	setlasterror(ACCESS_DENIED);  
	return(-1);
}

if((attribs & FAT_ATTRIB_VOLUME_LABEL)) {	/* can't set volume label bit */
	kernelfree(lfnbuf); 

	setlasterror(ACCESS_DENIED);  
	return(-1);
}


blockbuf=kernelalloc(MAX_BLOCK_SIZE);			/* allocate block buffers */
if(blockbuf == NULL) {
	kernelfree(lfnbuf); 

	setlasterror(NO_MEM);
	return(-1);
}

splitname(filename,&splitbuf);				/* split name */

if(findfirst(filename,&buf) == -1) {		/* get file entry */
	kernelfree(lfnbuf); 
	kernelfree(blockbuf);

	setlasterror(FILE_NOT_FOUND);
	return(-1);
}

if(strlen(filename) > 11) {	/* long filename */
	kernelfree(lfnbuf); 
	kernelfree(blockbuf);
	
	buf.attribs=attribs;

	return(createlfn(&buf,buf.findlastblock,buf.dirent));
}
			

if(blockio(_READ,splitbuf.drive,buf.findlastblock,blockbuf) == -1) return(-1); /* read block */

memcpy(&dirent,blockbuf+(buf.dirent*FAT_ENTRY_SIZE),FAT_ENTRY_SIZE);

dirent.attribs=(uint8_t) attribs;

memcpy(blockbuf+(buf.dirent*FAT_ENTRY_SIZE),&dirent,FAT_ENTRY_SIZE);

if(blockio(_WRITE,splitbuf.drive,buf.findlastblock,blockbuf) == -1) return(-1); /* write block */

setlasterror(NO_ERROR);  

return(NO_ERROR);				/* no error */
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
size_t fat_setfiletimedate(char *filename,TIMEBUF *createtime,TIMEBUF *last_modified_time) {
SPLITBUF splitbuf;
DIRENT dirent;
FILERECORD buf;
char *blockbuf;
FILERECORD *lfnbuf;
char *b;

lfnbuf=kernelalloc(sizeof(FILERECORD));	/* allocate split buffer */
if(lfnbuf == NULL) return(-1);

blockbuf=kernelalloc(MAX_BLOCK_SIZE);			/* allocate block buffers */
if(blockbuf == NULL) {
	kernelfree(lfnbuf); 

	setlasterror(NO_MEM);
	return(-1);
}

splitname(filename,&splitbuf);				/* split name */

if(findfirst(filename,&buf) == -1) {		/* get file entry */
	kernelfree(lfnbuf); 
	kernelfree(blockbuf);

	setlasterror(FILE_NOT_FOUND);
	return(-1);
}

if(strlen(filename) > 11) {	/* long filename */
	kernelfree(lfnbuf); 
	kernelfree(blockbuf);
	
	memcpy(&buf.timebuf,createtime,sizeof(TIMEBUF));
	memcpy(&buf.timebuf,last_modified_time,sizeof(TIMEBUF));
	return(createlfn(&buf,buf.findlastblock,buf.dirent));
}
			
if(blockio(_READ,splitbuf.drive,buf.findlastblock,blockbuf) == -1) return(-1); /* read block */

memcpy(&dirent,blockbuf+(buf.dirent*FAT_ENTRY_SIZE),FAT_ENTRY_SIZE);

dirent.createtime=(createtime->hours << 16)+(createtime->minutes << 11)+createtime->seconds;
dirent.createdate=((createtime->year << 16)-1980)+(createtime->month << 9)+createtime->day;

dirent.last_modified_time=(last_modified_time->hours << 16)+(last_modified_time->minutes << 11)+last_modified_time->seconds;
dirent.last_modified_date=((last_modified_time->year << 16)-1980)+(last_modified_time->month << 9)+last_modified_time->day;

memcpy(blockbuf+(buf.dirent*FAT_ENTRY_SIZE),&dirent,FAT_ENTRY_SIZE);

if(blockio(_WRITE,splitbuf.drive,buf.findlastblock,blockbuf) == -1) return(-1); /* write block */

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

size_t fat_findfreeblock(size_t drive) {
uint64_t rb;
uint64_t count;
uint16_t *shortptr;
uint32_t *longptr;
size_t fattype;
uint64_t countx;
uint32_t longentry;
size_t fatsize;
uint16_t shortentry;
uint32_t startblock;
BLOCKDEVICE blockdevice;
BPB *bpb;
char *buf;
size_t blockcount;

fattype=fat_detectchange(drive);
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

blockcount=2;

/* until free block found */ 

if(fattype == 12) {				/* fat 12 */
	fatsize=bpb->sectorsperfat;
	rb=bpb->reservedsectors;

	while(rb < (fatsize*bpb->sectorsperblock)) {

		b=buf;

		for(countx=rb;countx<rb+3;countx++) {
			if(blockio(_READ,drive,countx,b) == -1) {			/* read block */ 
				kernelfree(buf);
				return(-1);
			}

			b += bpb->sectorsize;
		}
			
		rb += 3;
		countx=0;
		shortptr=buf;

		while(countx<(bpb->sectorsperblock*bpb->sectorsize)*3) {
			shortentry=shortptr[countx];

			if(countx & 1) {	
				if((shortentry >> 4) == 0) {
					setlasterror(NO_ERROR);
					return(blockcount);
			         }
			}
			else
			{
				if((shortentry & 0xfff) == 0) {
					setlasterror(NO_ERROR);
					return(blockcount);
			}
					                  }

			blockcount++;
			countx++;
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

		shortptr=buf;

		for(countx=0;countx<(bpb->sectorsperblock*bpb->sectorsize);countx++) {
			if(shortptr[countx] == 0) {
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

		for(countx=0;countx<(bpb->sectorsperblock*bpb->sectorsize);countx++) {
					       
			if(longptr[countx] == 0) {
				setlasterror(NO_ERROR);
				return(blockcount);
			}

			blockcount++;
		}

	}
}

kernelfree(buf);	
setlasterror(END_OF_DIR);

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

size_t fat_getstartblock(char *name) {
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

fattype=fat_detectchange(splitbuf->drive);
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
 *   get next block in directory from fat using fat_getnextblock
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

if(strcmp(splitbuf->dirname,"\\") == 0) {						           /* root directory */
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
	considered_harmful:

	memset(token_buffer,0,MAX_PATH);
	c=get_filename_token_count(fullpath);
	get_filename_token(fullpath,token_buffer);					/* tokenize filename */

	tc--;	

	while((fattype == 12 && rb <= 0xff0) || (fattype == 16 && rb <= 0xfff0) || (fattype == 32 && rb <= 0xfffffff0)) {
		if(blockio(_READ,splitbuf->drive,(uint64_t) rb+start_of_data_area,blockbuf) == -1) {			/* read block for entry */
			kernelfree(splitbuf);
			kernelfree(blockbuf);
		     	kernelfree(lfnbuf);
	
			setlasterror(PATH_NOT_FOUND);
			return(-1);
		}

		blockptr=blockbuf;

		for(count=0;count<bpb->sectorsperblock*bpb->sectorsize;count++) {
			memcpy(&directory_entry,blockptr,FAT_ENTRY_SIZE);

			if(*blockptr == 0) {				/* end of directory */     
				kernelfree(lfnbuf);
				kernelfree(splitbuf);
				kernelfree(blockbuf);
				setlasterror(PATH_NOT_FOUND);

				return(-1);
			}


			fatentrytofilename(directory_entry.filename,file);			/* convert filename */

			touppercase(token_buffer);			/* convert to uppercase */

			if(wildcard(token_buffer,file) == 0) { 		/* if short entry filename found */         
				if(fattype == 12 || fattype == 16) rb=directory_entry.block_low_word;				/* get next block */
				if(fattype == 32) rb=(uint64_t) ((directory_entry.block_high_word << 16)+directory_entry.block_low_word); 	/* get next block */

				if(tc == 0) {
					setlasterror(NO_ERROR);
		
					kernelfree(lfnbuf);
				  	kernelfree(blockbuf);
				  	kernelfree(splitbuf);
			
					return(rb);				/* at end, return block */
				 }

					rb += (datastart-2);			/* add start of data area */

					goto  considered_harmful;
				}

				blockptr=blockptr+FAT_ENTRY_SIZE;					/* point to next entry */
			}

			rb=fat_getnextblock(splitbuf->drive,rb);
		}

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


size_t fat_getnextblock(size_t drive,uint64_t block) {
uint8_t *buf;
size_t entriesperblock;
size_t fattype;
uint64_t blockno;
uint16_t entry;
uint32_t entry_dword;
size_t entryno;
uint32_t entry_offset;
BLOCKDEVICE blockdevice;
BPB *bpb;

fattype=fat_detectchange(drive);
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

size_t updatefat(size_t drive,uint64_t block,uint16_t block_high_word,uint16_t block_low_word) {
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

fattype=fat_detectchange(drive);
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
		thirdbyte=((block_low_word & 0xff00) >> 8);
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
	if(blockio(_WRITE,drive,blockno,buf) == -1)  return(-1);

	blockno=blockno+(((bpb->sectorsperfat*bpb->sectorsperblock)-blockno));
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


size_t fat_detectchange(size_t drive) {
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
 * Convert filenamw (only filename) to FAT entry name
 *
 * In:  name	Filename
		outname Buffer to hold converted name
 *
 * Returns: nothing
 *
 */

size_t fat_convertfilename(char *filename,char *outname) {
char *s;
char *d;
size_t  count;
char *f[MAX_PATH];

strcpy(f,filename);		/* copy filename */

s=f;
d=outname;

memset(outname,' ',11);				/* fill with spaces */

for(count=0;count<strlen(filename);count++) {
	if(*s > 'a' && *s < 'z') *s=*s-32;			/* convert to uppercase */

	s++;
}

s=filename;

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
 * read long filename entry into of buffer starting from block, internal use only
 *
 * In:  drive	Drive
	block	Block number of long filename
	entryno FAT entry number of long filename
	n	FILERECORD to hold information about long filename
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t fat_readlfn(size_t drive,uint64_t block,size_t entryno,FILERECORD *n) {
char *s;
size_t entrycount=0;
size_t ec;
char *d;
DIRENT dirent;
char *buf[MAX_PATH];
size_t fattype;
size_t blockcount;
size_t count;
char *blockbuf;
char *blockptr;
uint8_t l;
size_t lfncount=0;
char *b;
struct lfn_entry *lfn;
BPB *bpb;
BLOCKDEVICE blockdevice;

fattype=fat_detectchange(drive);
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

while((fattype == 12 && block != 0xfff) || (fattype == 16 && block !=0xffff) || (fattype == 32 && block !=0xffffffff)) {	/* until end of chain */

	if(blockio(_READ,drive,block,blockbuf) == -1) {	/* read block for entry */
		kernelfree(blockbuf);
		kernelfree(lfn);
		return(-1);
	}

	for(count=0;count<((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE)-(entryno*FAT_ENTRY_SIZE);count++) {
		n->dirent=count;
					
		memset(lfn,0,sizeof(struct lfn_entry));
		memcpy(lfn,(void *) blockptr,FAT_ENTRY_SIZE);

		if(lfn->sequence == 0) {
			kernelfree(blockbuf);
			kernelfree(lfn);
			return(lfncount);		/* no more lfn */
		}
		
		blockptr=blockptr+FAT_ENTRY_SIZE;

//  if(lfn->sequence == 0xE5) continue;			/* skip deleted entries */
		
		entrycount=lfn->sequence;				/* get number of entries */
		if(entrycount >= 0x40) entrycount=entrycount-0x40;
		
		if(strlen_unicode(lfn->lasttwo_chars,2) > 0) {  
			d=d-strlen_unicode(lfn->lasttwo_chars,2);

			s=lfn->lasttwo_chars;
			b=d;

				for(ec=0;ec<strlen_unicode(lfn->lasttwo_chars,2);ec++) {

					//    if(*s == 0xFF) break;				/* at end */
					*b++=*s;					/* copy */
					s=s+2;
				}   

			lfncount++;
		}

		if(strlen_unicode(lfn->nextsix_chars,6) > 0) {
			d=d-strlen_unicode(lfn->nextsix_chars,6);
			s=lfn->nextsix_chars;

			b=d;

			for(ec=0;ec<strlen_unicode(lfn->nextsix_chars,6);ec++) {
				//    if(*s == 0xFF) break;				/* at end */

				*b++=*s;						/* copy */
				s=s+2;
			}  

			lfncount++;
		}

		if(strlen_unicode(lfn->firstfive_chars,5) > 0) {
			d=d-strlen_unicode(lfn->firstfive_chars,5);
			s=lfn->firstfive_chars;
					  
			b=d;

			for(ec=0;ec<strlen_unicode(lfn->firstfive_chars,5);ec++) {
				//   if(*s == 0xFF) break;				/* at end */

				*b++=*s;						/* copy */
				s=s+2;	
			} 

			lfncount++;
		}

	
		if(lfn->sequence == 1 || lfn->sequence == 0x41) {		/* at end of filename copy filename */
			strcpy(n->filename,d);			/* copy filename */

/* at end, so will point to short entry */

			memcpy(&dirent,blockptr,FAT_ENTRY_SIZE);

			n->attribs=dirent.attribs;

			n->timebuf.hours=(dirent.createtime & 0xf800) >> 11;
			n->timebuf.minutes=(dirent.createtime  & 0x7e0) >> 6;
			n->timebuf.seconds=(dirent.createtime & 0x1f);
			n->timebuf.year=((dirent.createdate & 0xfe00) >> 9)+1980;
			n->timebuf.month=(dirent.createdate & 0x1e0) >> 5;
			n->timebuf.day=(dirent.createdate & 0x1f);
			n->filesize=dirent.filesize;
			n->startblock=(dirent.block_high_word << 16)+dirent.block_low_word;

			kernelfree(blockbuf);
			kernelfree(lfn);
			return(lfncount);
		}

	}


}

kernelfree(blockbuf);
kernelfree(lfn);

return(lfncount);
}
		
/*
 * Write long filename, internal use only
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

size_t fat_writelfn(size_t drive,uint64_t block,size_t entryno,char *n,char *newname) {
BLOCKDEVICE blockdevice;
char *buf[MAX_PATH];
size_t countx;
uint32_t fattype;
void *blockbuf;
size_t count;
char *fatname[11];
FILERECORD *lfn;
char *shortname[FAT_ENTRY_SIZE];
char *blockptr;
SPLITBUF splitbuf;
size_t entry;
char *fullpath[MAX_PATH];
size_t flen;
size_t firstblock;
size_t freeblock;
size_t  blockcount;
size_t firstfreeblock;
size_t lastentry;
BPB *bpb;

fattype=fat_detectchange(drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(NULL);
}

bpb=blockdevice.superblock;		/* get bpb */


blockbuf=kernelalloc((bpb->sectorsperblock*bpb->sectorsize));		/* allocate block buffer */
if(blockbuf == NULL) return(-1);

lfn=kernelalloc((bpb->sectorsperblock*bpb->sectorsize));		/* allocate block buffer */
if(lfn == NULL) return(-1);

/*
 * new long filename is shorter or same length as old long filename
 *
 */

if(strlen(newname) <= strlen(n)) {

	if(fat_readlfn(drive,block,entryno,lfn) == -1) {		/* read long filename */
		kernelfree(blockbuf);
		return(-1);
	}

	strcpy(lfn->filename,newname);

	deletelfn(n,block,entryno);			/* delete filename and recreate it */

	createlfn(lfn,block,entryno);

	kernelfree(blockbuf);
	return;
}
	
/*oherwise new long filename is longer than old long filename
 *
 */

/* attempt to create free entries in free directory entries, if none could be found, create entries at end of directory */

getfullpath(n,fullpath);			/* get full path */
splitname(fullpath,&splitbuf);

block=fat_getstartblock(splitbuf.dirname);
firstblock=block;					/* save block number */

flen=strlen(newname)/13;				/* get number of 13 byte sections */
if(strlen(newname) % 13 > 0) flen++;

firstblock=block;					/* save block number */
entry=0;
blockptr=blockbuf;

while((fattype == 12 && firstblock != 0xfff) || (fattype == 16 && firstblock !=0xffff) || (fattype == 32 && firstblock !=0xffffffff)) {	/* until end of chain */

	if(blockio(_READ,splitbuf.drive,firstblock,blockbuf) == -1) {	/* read block for entry */
		kernelfree(blockbuf);
		return(-1);
	}

	countx=0;
	lastentry=0;

	for(count=0;count<((bpb->sectorsperblock*bpb->sectorsize)/FAT_ENTRY_SIZE);count++) {
		if((*blockptr == 0) || (*blockptr == 0xE5)) {
			countx++;	/* found free entry */
		}
		else
		{
			lastentry=count+1;
			countx=0;	/* entries must be contingous */
		}


		if(countx > flen) {			/* found free entries */
			deletelfn(n,block,entryno);			/* delete filename and recreate it */

			strcpy(lfn->filename,newname);
			lfn->findlastblock=block;
			lfn->dirent=lastentry;

			createlfn(lfn,block,lastentry);

			kernelfree(blockbuf);
			return(NO_ERROR);
		}
		
		blockptr=blockptr+FAT_ENTRY_SIZE;		/* point to next entry */
	}

	block=fat_getnextblock(splitbuf.drive,block);
	blockptr=blockbuf;
}

/* no free entries could be found, create entries at end of directory */

/* root directory is fixed size for fat12 and fat16 */

if(fattype == 12 || fattype == 16) {
	if(strcmp(splitbuf.dirname,"\\") == 0) {
		setlasterror(DIRECTORY_FULL);
		return(-1);
	}
}

block=firstblock;

while((fattype == 12 && block != 0xfff) || (fattype == 16 && block !=0xffff) || (fattype == 32 && block !=0xffffffff)) {	/* until end of chain */
	b=fat_getnextblock(splitbuf.drive,block);			/* get next block */
				
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

firstfreeblock=fat_findfreeblock(drive);			/* find free block */
if(firstfreeblock == -1) {
	kernelfree(blockbuf);
	return(-1);
}
			
if(updatefat(drive,block,(freeblock >> 8),(freeblock & 0xffff)) == -1) {
	kernelfree(blockbuf);
	return(-1);
}

freeblock=firstfreeblock;

for(count=0;count<blockcount;count++) {
	freeblock=fat_findfreeblock(drive);			/* add remainder of blocks */

	if(freeblock == -1) {
		kernelfree(blockbuf);
		return(-1);
	}

	if(updatefat(drive,block,(freeblock >> 8),(freeblock & 0xffff)) == -1) {
		kernelfree(blockbuf);
		return(-1);
	}

}


deletelfn(n,block,entryno);			/* delete filename and recreate it */
createlfn(lfn,block,entryno);
			
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

size_t createshortname(char *filename,char *out) {
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

	strcat(s,f);	/* append extension */

	s=out;

	for(count=0;count<strlen(out);count++) {
		if(*s > 'a' && *s < 'z') *s=*s-32;
		s++;
	}    

	break;

	if(findfirst(out,&searchbuf) == NULL) return(NO_ERROR);			/* check if file already exists */
}


	return(-1);
}

uint8_t createlfnchecksum(char *filename) {
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

size_t deletelfn(char *filename,uint64_t block,size_t entry) {
size_t count;
char *blockptr;
char *blockbuf;
SPLITBUF splitbuf;
char *fullpath[MAX_PATH];
size_t blockcount;
size_t flen;
BLOCKDEVICE blockdevice;
BPB *bpb;
int fattype;
uint64_t block_write_count;

getfullpath(filename,fullpath);				/* get full path */
splitname(fullpath,&splitbuf);				/* split filename */

fattype=fat_detectchange(splitbuf.drive);
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
//flen++;			/* plus short entry */

/* allocate buffer */

if(flen*FAT_ENTRY_SIZE < (bpb->sectorsperblock*bpb->sectorsize)) {		/* one block */
	blockcount=1;
}
else
{
	blockcount=(flen*FAT_ENTRY_SIZE)/(bpb->sectorsperblock*bpb->sectorsize);	/* more than one block */
}

blockbuf=kernelalloc((bpb->sectorsperblock*bpb->sectorsize)*blockcount);	/* allocate block buffer */
if(blockbuf == NULL) return(-1);

/* read in blocks */

for(count=0;count<blockcount;count++) {

	if(blockio(_READ,splitbuf.drive,block,blockbuf) == -1) {	/* read block for entry */
		kernelfree(blockbuf);
		return(-1);
	}

	blockptr=blockptr+(bpb->sectorsperblock*bpb->sectorsize);
}

blockptr=blockbuf;
blockptr += (entry*FAT_ENTRY_SIZE);			/* point to entries */

for(count=0;count<flen+1;count++) {
	*blockptr=0xE5;
	blockptr=blockptr+FAT_ENTRY_SIZE;
}

/* write out blocks */

blockptr=blockbuf;

for(block_write_count=0;block_write_count<blockcount;count++) {

	if(blockio(_WRITE,splitbuf.drive,block,blockbuf) == -1) {	/* read block for entry */
		kernelfree(blockbuf);

		return(-1);
	}

	blockptr=blockptr+(bpb->sectorsperblock*bpb->sectorsize);	/* point to next */
}

kernelfree(blockbuf);

return(NO_ERROR);
}

	/*
 * Create long filename
 *
 * In:  newname		FILERECORD of new filename
	block		Block number of long filename
	entry			FAT entry number of long filename
 *
 * Returns: -1 on error, 0 on success
 *
 */

size_t createlfn(FILERECORD *newname,uint64_t block,size_t entry) {
size_t count;
size_t countx;
size_t fcount;
char *fptr;
size_t flen;
char *s;
char *d;
char *f;
struct lfn_entry lfn[255];
DIRENT dirent;
size_t ec;
char *blockptr;
char *blockbuf;
BLOCKDEVICE blockdevice;
SPLITBUF splitbuf;
char *fullpath[MAX_PATH];
size_t blockcount;
char *file[11];
uint32_t seq;
char b;
BPB *bpb;
int fattype;

getfullpath(newname,fullpath);				/* get full path */
splitname(fullpath,&splitbuf);				/* split filename */

fattype=fat_detectchange(splitbuf.drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

if(getblockdevice(splitbuf.drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(NULL);
}

bpb=blockdevice.superblock;	/* point to data */


memset(lfn,0,sizeof(struct lfn_entry)*255);	/* clear struct */

flen=strlen(splitbuf.filename)/13;		/* get number of 13 byte sections */
if(strlen(splitbuf.filename) % 13) flen++;		/* get number of 13 byte sections */
flen++;

/* copy filename to new lfn entries */

seq=flen;

createshortname(splitbuf.filename,file);			/* filename */
fat_convertfilename(file,dirent.filename);

fcount=1;
count=flen;
fptr=&splitbuf.filename;		/* point to name */

while(1) {
	lfn[count].attributes=0x0f;					/* attributes must be 0x0f */
	lfn[count].sc_alwayszero=0;					/* must be zero */
	lfn[count].checksum=createlfnchecksum(dirent.filename);	/* create checksum */
	lfn[count].typeindicator=0;

	lfn[count].sequence=fcount;
	if(fcount == flen) lfn[count].sequence |= 0x40;			/* last entry */

	seq--;
	s=lfn[count].firstfive_chars;

	for(ec=0;ec<5;ec++) {
		if(*fptr == 0) goto lfn_over;
		*s=*fptr++;
		s=s+2;
	}

	s=lfn[count].nextsix_chars;

	for(ec=0;ec<6;ec++) {
		if(*fptr == 0) goto lfn_over;
		*s=*fptr++;
		s=s+2;
	}


	s=lfn[count].lasttwo_chars;

	for(ec=0;ec<2;ec++) {
		if(*fptr == 0) goto lfn_over;
		*s=*fptr++;
		s=s+2;
	}

	fcount++;
	count--;
}

lfn_over:
	/* create short entry after */

dirent.attribs=newname->attribs;
dirent.reserved=0;
dirent.reserved=NULL;
dirent.create_time_fine_resolution=0;


dirent.createtime=(newname->timebuf.hours << 16)+(newname->timebuf.minutes << 11)+newname->timebuf.seconds;
dirent.createdate=((newname->timebuf.year << 16)-1980)+(newname->timebuf.month << 9)+newname->timebuf.day;
dirent.last_modified_time=dirent.createtime;
dirent.last_modified_date=dirent.createdate;
dirent.filesize=newname->filesize;
dirent.block_low_word=(newname->startblock & 0xffff);
dirent.block_high_word=(newname->startblock >> 16);

/* allocate buffer */

if(flen*FAT_ENTRY_SIZE < (bpb->sectorsperblock*bpb->sectorsize)) {		/* one block */
	blockcount=1;
}
else
{
	blockcount=(flen*FAT_ENTRY_SIZE)/(bpb->sectorsperblock*bpb->sectorsize);	/* more than one block */
}

blockbuf=kernelalloc((bpb->sectorsperblock*bpb->sectorsize)*blockcount);	/* allocate block buffer */
if(blockbuf == NULL) return(-1);


blockptr=blockbuf;

/* read in blocks */

for(countx=0;countx<blockcount;countx++) {

	if(blockio(_READ,splitbuf.drive,block,blockptr) == -1) {	/* read block for entry */
		kernelfree(blockbuf);
		return(-1);
	}

}


blockptr=blockbuf+(entry*FAT_ENTRY_SIZE);

memcpy(blockptr,&lfn[count],(flen*FAT_ENTRY_SIZE));			/* copy to buffer */

blockptr=blockptr+(flen*FAT_ENTRY_SIZE);
memcpy(blockptr,&dirent,FAT_ENTRY_SIZE);		/* copy to buffer */

/* write out blocks */

blockptr=blockbuf;

for(countx=0;countx<blockcount;countx++) {

		if(blockio(_WRITE,splitbuf.drive,block,blockptr) == -1) {	/* read block for entry */
			kernelfree(blockbuf);
			return(-1);
		}

		blockptr=blockptr+(bpb->sectorsperblock*bpb->sectorsize);	/* point to next */
}

kernelfree(blockbuf);

return(NO_ERROR);
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
size_t fat_seek(size_t handle,size_t pos,size_t whence) {
size_t count=0;
size_t sb;
size_t posp;
FILERECORD onext;
SPLITBUF splitbuf;
BLOCKDEVICE blockdevice;
int fattype;
BPB *bpb;

if(gethandle(handle,&onext) == -1) {	/* bad handle */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

fattype=fat_detectchange(onext.drive);
if(fattype == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
} 

/* update current block */
if(getblockdevice(onext.drive,&blockdevice) == -1) {
	setlasterror(INVALID_DISK);
	return(-1);
}

bpb=blockdevice.superblock;		/* point to data */

/* round down if not a multiple of sector size */ 

if(pos >= (bpb->sectorsperblock*bpb->sectorsize)) {
if(pos % (bpb->sectorsperblock*bpb->sectorsize) > 0) posp -= (pos % (bpb->sectorsperblock*bpb->sectorsize));

sb=fat_getstartblock(onext.filename);			/* get first block */

	if(posp < (bpb->sectorsperblock*bpb->sectorsize)) {
		posp=1;
	}
	else
	{
		posp=(pos/(bpb->sectorsperblock*bpb->sectorsize));
	}
	
/* find block number from fat */

	for(count=0;count<posp;count++) {
		sb=fat_getnextblock(onext.drive,sb);			/* get next block */
	}

onext.currentblock=sb;
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


/*
 * Convert FAT entry to filename
 *
 * In:  filename	FAT entry to convert
	out		Buffer to hold filename
 *
 * Returns: nothing
 *
 */	
void fatentrytofilename(char *filename,char *out) {
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

return;
}


