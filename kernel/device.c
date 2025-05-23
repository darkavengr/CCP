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
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "errors.h"
#include "bootinfo.h"
#include "kernelhigh.h"
#include "debug.h"
#include "memorymanager.h"
#include "string.h"

BLOCKDEVICE *blockdevices=NULL;
size_t lastdrive=2;
CHARACTERDEVICE *characterdevs=NULL;
MUTEX blockdevice_mutex;
MUTEX characterdevice_mutex;
uint32_t drive_bitmap=0;
IRQ_HANDLER *irq_handlers=NULL;
IRQ_HANDLER *irq_handlers_end=NULL;

	
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
	 if(strncmpi(next->name,device->name,MAX_PATH) == 0) {		/* block device exists */
		unlock_mutex(&blockdevice_mutex);			/* lock mutex */

	 	setlasterror(FILE_EXISTS);
		return(-1);
	 }

	 last=next;
	 next=next->next;
}

if(blockdevices == NULL) {				/* if empty allocate struct */
	blockdevices=kernelalloc(sizeof(BLOCKDEVICE));
	if(blockdevices == NULL) {
		unlock_mutex(&blockdevice_mutex);			/* lock mutex */

		return(-1);
	}

	memset(blockdevices,0,sizeof(BLOCKDEVICE));
	last=blockdevices;
}
else
{
	last->next=kernelalloc(sizeof(BLOCKDEVICE)); /* add to struct */
	if(last->next == NULL) {
		unlock_mutex(&blockdevice_mutex);			/* lock mutex */

		return(-1);
	}

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
 * In: device		Character device to register
 *
 * Returns 0 on success, -1 otherwise
 * 
 */
size_t add_character_device(CHARACTERDEVICE *device) {
CHARACTERDEVICE *charnext;
CHARACTERDEVICE *charlast;

lock_mutex(&characterdevice_mutex);			/* lock mutex */

charnext=characterdevs;

/* find device */

if(characterdevs == NULL) {				/* if empty allocate struct */
	characterdevs=kernelalloc(sizeof(CHARACTERDEVICE));
	if(characterdevs == NULL) {
		unlock_mutex(&characterdevice_mutex);			/* unlock mutex */

		return(-1);
	}

	memset(characterdevs,0,sizeof(CHARACTERDEVICE));
	charnext=characterdevs;
}
else
{
	charnext=characterdevs;

	do {
		if(strncmpi(device->name,charnext->name,MAX_PATH) == 0) {	/* device exists */
			unlock_mutex(&characterdevice_mutex);			/* unlock mutex */

	 		setlasterror(FILE_EXISTS);
			return(-1);
		}

		charlast=charnext;
	 	charnext=charnext->next;
	} while(charnext != NULL);

charlast->next=kernelalloc(sizeof(CHARACTERDEVICE)); /* add to struct */
if(charlast->next == NULL) {
	unlock_mutex(&characterdevice_mutex);			/* unlock mutex */

	return(-1);
}

charnext=charlast->next;
memset(charnext,0,sizeof(CHARACTERDEVICE));	
}

/* at end here */
memcpy(charnext,device,sizeof(CHARACTERDEVICE));

charnext->next=NULL;

unlock_mutex(&characterdevice_mutex);			/* unlock mutex */

setlasterror(NO_ERROR);
return(NO_ERROR);
}

/*
 * Block I/O handler
 *
 * This calls a hardware-specific routine
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
	kprintf_direct("debug: No blockio() handler defined\n");
	asm("xchg %bx,%bx");

	setlasterror(NOT_IMPLEMENTED);
	return(-1);
}


for(count=0;count<next->sectorsperblock;count++) {
	if(next->blockio(op,next->physicaldrive,(uint64_t) (next->startblock+block)+count,b) == -1) {
		lasterr=getlasterror();

		/* handle any critical errors */

		if((lasterr == WRITE_PROTECT_ERROR) || (lasterr == DRIVE_NOT_READY) || (lasterr == INVALID_CRC) \
		|| (lasterr == GENERAL_FAILURE) || (lasterr == READ_FAULT) || (lasterr == WRITE_FAULT)) { 
	
	  		error=call_critical_error_handler(next->name,drive,(op | 0x80000000),lasterr);	/* call exception handler */

	  		if(error == CRITICAL_ERROR_ABORT) {			/* abort */
	   			unlock_mutex(&blockdevice_mutex);
	   			exit(0);
	  		}
	  		else if(error == CRITICAL_ERROR_RETRY) {		/* retry */
				continue;
	  		}
	  		else if((error == CRITICAL_ERROR_FAIL) || (error == -1)) {	/* fail */
	   			next->flags ^= DEVICE_LOCKED;			/* unlock drive */

	   			unlock_mutex(&blockdevice_mutex);			/* unlock mutex */
	   			return(-1);
	  		}
	 	}
		else
		{
	   		unlock_mutex(&blockdevice_mutex);			/* unlock mutex */
	   		return(-1);	/* FAIL */
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
 * In: name	Name of device
       buf	Buffer to hold information about device
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t findcharacterdevice(char *name,CHARACTERDEVICE *buf) {
CHARACTERDEVICE *next;

lock_mutex(&characterdevice_mutex);			/* lock mutex */

next=characterdevs;

while(next != NULL) {
	if(strncmpi(next->name,name,MAX_PATH) == 0) {		/* found */
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
 * In: drive	Drive number
       buf	Buffer to hold information about device
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t getblockdevice(size_t drive,BLOCKDEVICE *buf) {
BLOCKDEVICE *next;

lock_mutex(&blockdevice_mutex);			/* lock mutex */

next=blockdevices;
while(next != NULL) {

//	kprintf_direct("getblockdevice=%d %d\n",next->drive,drive);

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
 * In: name	Name of device
       buf	Buffer to hold information about device
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
	if(strncmpi(next->name,name,MAX_PATH) == 0) {		/* found device */
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
 * In: drive	Drive
       buf	information about device
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

//		asm("xchg %bx,%bx");

		memcpy(next,driver,sizeof(BLOCKDEVICE));		/* update struct */

		next->next=save_next;

		unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

		setlasterror(NO_ERROR);
		return(0);
	}

	next=next->next;
}

unlock_mutex(&blockdevice_mutex);			/* unlock mutex */
	
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
FILERECORD buf;

/* find device */
lock_mutex(&blockdevice_mutex);			/* lock mutex */

next=blockdevices;
last=blockdevices;

while(next != NULL) {
	 if(strncmpi(next->name,name,MAX_PATH) == 0) {		 		/* already loaded */

		if(gethandle(name,&buf) == 0) {		/* file is open */
		 	unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

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
FILERECORD buf;

/* find device */

lock_mutex(&characterdevice_mutex);			/* lock mutex */

next=characterdevs;
last=next;

while(next != NULL) {
	 if(strncmpi(next->name,name,MAX_PATH) == 0) {		/* device found */

		if(gethandle(name,&buf) == 0) {		/* device is in use */
			unlock_mutex(&characterdevice_mutex);	/* unlock mutex */

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
 * Returns: next drive number or -1 on error
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
irq_handlers=NULL;

initialize_mutex(&blockdevice_mutex);		/* intialize mutex */
initialize_mutex(&characterdevice_mutex);	/* intialize mutex */

drive_bitmap=0;
}

/*
 * Set IRQ handler
 *
 * In: irqnumber	IRQ number
 *     refnumber	Unique reference number to distinguish between multiple entries for same IRQs
 *     handler		IRQ handler
 *
 *  Returns: 0 on success, -1 on failure
 *
 */
size_t setirqhandler(size_t irqnumber,size_t refnumber,void *handler) {
if(irq_handlers == NULL) {		/* first entry */
	irq_handlers=kernelalloc(sizeof(IRQ_HANDLER));
	if(irq_handlers == NULL) return(-1);

	irq_handlers_end=irq_handlers;	/* save end */
}
else
{
	irq_handlers_end->next=kernelalloc(sizeof(IRQ_HANDLER));
	if(irq_handlers_end->next == NULL) return(-1);

	irq_handlers_end=irq_handlers_end->next;	/* point to end */
}

irq_handlers_end->handler=handler;		/* set entry */
irq_handlers_end->refnumber=refnumber;
irq_handlers_end->irqnumber=irqnumber;
irq_handlers_end->next=NULL;

setlasterror(NO_ERROR);
return(0);
}

/*
 * Call IRQ handlers
 *
 * In: irqnumber	IRQ number
 *     stackparams	Pointer to registers pushed onto stack
 *
 *  Returns: Nothing
 *
 */
void callirqhandlers(size_t irqnumber,void *stackparams) {
IRQ_HANDLER *next=irq_handlers;

while(next != NULL) {
	//kprintf_direct("IRQ=%X %X\n",next->irqnumber,next->handler);
	
	if((next->irqnumber == irqnumber) && (next->handler != NULL)) {
		next->handler(stackparams);		/* call handler */
	}

	next=next->next;
}

return;
}

