/*  CCP Version 0.0.1
	   (C) Matthew Boote 2020-2023-2022

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
 * device manager
 */

#include <stdint.h>
#include "../processmanager/mutex.h"
#include "device.h"
#include "../filemanager/vfs.h"
#include "../header/errors.h"
#include "../header/bootinfo.h"
#include "../header/kernelhigh.h"
#include "../header/debug.h"

size_t add_block_device(BLOCKDEVICE *driver);
size_t add_char_device(CHARACTERDEVICE *device);
size_t blockio(size_t op,size_t drive,uint64_t block,void *buf);
size_t findcharacterdevice(char *name,CHARACTERDEVICE *buf);
size_t getblockdevice(size_t drive,BLOCKDEVICE *buf);
size_t getdevicebyname(char *name,BLOCKDEVICE *buf);
size_t update_block_device(size_t drive,BLOCKDEVICE *driver);
size_t remove_block_device(char *name);
size_t remove_char_device(char *name);
size_t allocatedrive(void);
void devicemanager_init(void);

BLOCKDEVICE *blockdevices=NULL;
size_t lastdrive=2;
CHARACTERDEVICE *characterdevs=NULL;
MUTEX blockdevice_mutex;
MUTEX characterdevice_mutex;
uint32_t drive_bitmap=0;
	
/*
 * Add block device
 *
 * In: driver	Block device to add
 *
 * Returns 0 on success, -1 otherwise
 * 
 */
size_t add_block_device(BLOCKDEVICE *device) {
BLOCKDEVICE *next;
BLOCKDEVICE *last;

lock_mutex(&blockdevice_mutex);			/* lock mutex */

/* find end of device list */

next=blockdevices;
	
while(next != NULL) {
	 if(strcmpi(next->name,device->name) == 0) {		/* block device exists */
	 	setlasterror(DEVICE_EXISTS);
		return(-1);
	 }

	 last=next;
	 next=next->next;
}

if(blockdevices == NULL) {				/* if empty allocate struct */
	blockdevices=kernelalloc(sizeof(BLOCKDEVICE));
	if(blockdevices == NULL) return(-1);

	memset(blockdevices,0,sizeof(BLOCKDEVICE));
	last=blockdevices;
}
else
{
	last->next=kernelalloc(sizeof(BLOCKDEVICE)); /* add to struct */
	if(last->next == NULL) return(-1);

	last=last->next;
	memset(last,0,sizeof(BLOCKDEVICE));
}

/* at end here */
memcpy(last,device,sizeof(BLOCKDEVICE));

last->next=NULL;
last->superblock=NULL;

unlock_mutex(&blockdevice_mutex);		/* unlock mutex */
setlasterror(NO_ERROR);
return(NO_ERROR);
}

/*
 * Register character device
 *
 * In: CHARACTERKDEVICE *device		Character device to register
 *
 * Returns 0 on success, -1 otherwise
 * 
 */
size_t add_character_device(CHARACTERDEVICE *device) {
CHARACTERDEVICE *charnext;
CHARACTERDEVICE *charlast;

charnext=characterdevs;

/* find device */

if(characterdevs == NULL) {				/* if empty allocate struct */
	characterdevs=kernelalloc(sizeof(CHARACTERDEVICE));
	if(characterdevs == NULL) return(-1);

	memset(characterdevs,0,sizeof(CHARACTERDEVICE));
	charnext=characterdevs;
}
else
{
	charnext=characterdevs;

	do {
		if(strcmpi(device->name,charnext->name) == 0) {		 		/* already loaded */
			setlasterror(DEVICE_EXISTS);
			return(-1);
		}

		charlast=charnext;
	 	charnext=charnext->next;
	} while(charnext != NULL);

charlast->next=kernelalloc(sizeof(CHARACTERDEVICE)); /* add to struct */
if(charlast->next == NULL) return(-1);

charnext=charlast->next;
memset(charnext,0,sizeof(CHARACTERDEVICE));
	
}

/* at end here */
memcpy(charnext,device,sizeof(CHARACTERDEVICE));

charnext->next=NULL;

setlasterror(NO_ERROR);
return(NO_ERROR);
}

/*
 * block i/o handler
 *
 * calls a hardware-specific routine
 *
 * In: op		Operation (0=read, 1=write)
 *     block		Block number
       buf		Buffer
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t blockio(size_t op,size_t drive,uint64_t block,void *buf) {
BLOCKDEVICE *next;
char *b;
size_t count;
size_t error;
size_t lasterr;

lock_mutex(&blockdevice_mutex);			/* lock mutex */

next=blockdevices;					/* find drive entry */
	
while(next != NULL) {
	 if(next->drive == drive) break;
	 next=next->next;
}

if(next == NULL) {					/* bad drive */
	 unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

	 setlasterror(INVALID_DRIVE);
	 return(-1);
}

unlock_mutex(&blockdevice_mutex);			/* unlock mutex */
b=buf;

if(next->sectorsperblock == 0) next->sectorsperblock=1;

if(next->blockio == NULL) {				/* no handler defined */
	kprintf_direct("debug: No blockio handler defined\n");
	asm("xchg %bx,%bx");

	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}


for(count=0;count<next->sectorsperblock;count++) {

	if(next->blockio(op,next->physicaldrive,(uint64_t) (next->startblock+block)+count,b) == -1) {
		lasterr=getlasterror();

		if(lasterr == WRITE_PROTECT_ERROR || lasterr == DRIVE_NOT_READY || lasterr == INVALID_CRC \
		|| lasterr == GENERAL_FAILURE || lasterr == DEVICE_IO_ERROR) { 
	
	  		error=call_critical_error_handler(next->name,drive,(op & 0x80000000),lasterr);	/* call exception handler */

	  		if(error == CRITICAL_ERROR_ABORT) {
	   			unlock_mutex(&blockdevice_mutex);			/* unlock mutex */
	   			exit(0);	/* abort */
	  		}

	  		if(error == CRITICAL_ERROR_RETRY) {		/* RETRY */
	   			count--;
	   			continue;
	  		}

	  		if(error == CRITICAL_ERROR_FAIL) {
	   			next->flags ^= DEVICE_LOCKED;				/* unlock drive */

	   			unlock_mutex(&blockdevice_mutex);			/* unlock mutex */
	   			return(-1);	/* FAIL */
	  		}
	 	}
	}

b=b+next->sectorsize;
}


unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

setlasterror(NO_ERROR);
return(NO_ERROR);
}	


/*
 * Get information about a character device
 *
 * In: char *name		Name of device
	      CHARACTERDEVICE *buf	Buffer to hold information about device
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t findcharacterdevice(char *name,CHARACTERDEVICE *buf) {
CHARACTERDEVICE *next;

lock_mutex(&characterdevice_mutex);			/* lock mutex */

next=characterdevs;

while(next != NULL) {
	if(strcmpi(next->name,name) == 0) {		/* found */
		memcpy(buf,next,sizeof(CHARACTERDEVICE));

		unlock_mutex(&characterdevice_mutex);			/* unlock mutex */

		setlasterror(NO_ERROR);
		return(NO_ERROR);
	}

next=next->next;
}

setlasterror(FILE_NOT_FOUND);
return(-1);
}

/*
 * Get information about a block device using drive number
 *
 * In: size_t drive		Drive number
	      BLOCKDEVICE *buf		Buffer to hold information about device
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t getblockdevice(size_t drive,BLOCKDEVICE *buf) {
BLOCKDEVICE *next;

lock_mutex(&blockdevice_mutex);			/* lock mutex */

next=blockdevices;
while(next != NULL) {

	if(next->drive == drive) {		/* found device */

		memcpy(buf,next,sizeof(BLOCKDEVICE));

		unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

		setlasterror(NO_ERROR);
		return(NO_ERROR);
	}

next=next->next;
}

unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

setlasterror(INVALID_DRIVE);
return(-1);
}

/*
 * Get information about a character device using it's name
 *
 * In: char *name		Name of device
	      CHARACTERDEVICE *buf	Buffer to hold information about device
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t getdevicebyname(char *name,BLOCKDEVICE *buf) {
BLOCKDEVICE *next;
BLOCKDEVICE *savenext;

SPLITBUF splitbuf;

lock_mutex(&blockdevice_mutex);			/* lock mutex */

next=blockdevices;

while(next != NULL) {
	if(strcmpi(next->name,name) == 0) {		/* found device */
		savenext=next->next;

		memcpy(buf,next,sizeof(BLOCKDEVICE));
		next->next=savenext;

		unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

		setlasterror(NO_ERROR);
		return(NO_ERROR);
	}

	next=next->next;
}

unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

setlasterror(INVALID_DRIVE);
return(-1);
}

/*
 * Update block device information
 *
 * In: size_t drive		Drive
	      BLOCKDEVICE *buf		information about device
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t update_block_device(size_t drive,BLOCKDEVICE *driver) {
BLOCKDEVICE *next;
BLOCKDEVICE *save_next;

/* find device */
lock_mutex(&blockdevice_mutex);			/* lock mutex */

next=blockdevices;

while(next != NULL) {
	if(next->drive == drive) {		 		/* already loaded */
		save_next=next->next;

		memcpy(next,driver,sizeof(BLOCKDEVICE));		/* update struct */

		next->next=save_next;

		unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

		setlasterror(NO_ERROR);
		return(0);
	}

next=next->next;
}
	
setlasterror(INVALID_DRIVE);
return(-1);
}

/*
 * Remove block device
 *
 * In: name	Name of device
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t remove_block_device(char *name) {
BLOCKDEVICE *next;
BLOCKDEVICE *last;

/* find device */
lock_mutex(&blockdevice_mutex);			/* lock mutex */

next=blockdevices;
last=blockdevices;

while(next != NULL) {
	 if(strcmpi(next->name,name) == 0) {		 		/* already loaded */

		if(gethandle(name) == 0) {		/* file is open */
			setlasterror(FILE_IN_USE);
			return(-1);
		}

	 	if(last == blockdevices) {			/* start */
	 		blockdevices=next->next;
	 	}
	  	else if(next->next == NULL) {		/* end */
			last->next=NULL;
	  	}
	  	else						/* middle */
	  	{
	   		last->next=next->next;	
	  	}
	  
		kernelfree(next);

	  	unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

		setlasterror(NO_ERROR);
		return(0);
	}

	last=next;
	next=next->next;
}

setlasterror(INVALID_DRIVE);
return(-1);
}

/*
 * Remove character device
 *
 * In: name		Name of device
	buf		Buffer to hold information about device
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t remove_character_device(char *name) {
CHARACTERDEVICE *next;
CHARACTERDEVICE *last;

/* find device */

lock_mutex(&characterdevice_mutex);			/* lock mutex */

next=characterdevs;
last=next;

while(next != NULL) {
	 if(strcmpi(next->name,name) == 0) {		/* device found */

		if(gethandle(name) == 0) {		/* device is in use */
			setlasterror(FILE_IN_USE);
			return(-1);
		}

	 	if(last == characterdevs) {			/* start */
	 		characterdevs=next->next;
	 	}
		else if(next->next == NULL) {		/* end */
			last->next=NULL;
	  	}
	  	else						/* middle */
	  	{
	   		last->next=next->next;	
		  	kernelfree(next);

	  	}

		unlock_mutex(&characterdevice_mutex);	/* unlock mutex */

		setlasterror(NO_ERROR);
		return(0);
	 }

last=next;
next=next->next;
}

setlasterror(INVALID_DEVICE);
return(-1);
}

/*
 * Allocate drive number
 *
 * In: nothing
 *
 * Returns next drive number or -1 on error
 * 
 */

size_t allocatedrive(void) {
size_t count;
size_t drivenumber=2;

/* loop through bitmap to find free drive
	  starts at bit 2 to skip first two drives which are reserved */

/* from 2**2 to 2**26 */

for(count=4;count<67108864;count *= 2) {
	if((drive_bitmap & count) == 0) {		/* found free drive */
		drive_bitmap |= count;		/* allocate drive */
		return(drivenumber);
	}

	drivenumber++;
}

return(-1);		/* no free drives */
}

/*
 * Free drive letter
 *
 * In: drive to free 
 *
 * Returns nothing
 * 
 */
size_t freedrive(size_t drive) {
drive_bitmap &= (ipow(2,drive));		/* free drive */
}


/*
 * Initialize device manager
 *
 * In: nothing       
 *
 * Returns nothing
 * 
 */

void devicemanager_init(void) {
blockdevices=NULL;
characterdevs=NULL;

initialize_mutex(&blockdevice_mutex);		/* intialize mutex */
initialize_mutex(&characterdevice_mutex);	/* intialize mutex */

drive_bitmap=0;
}

