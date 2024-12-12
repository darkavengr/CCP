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

/* *ATA driver for CCP
 *
 */
#include <stdint.h>
#include "kernelhigh.h"
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "ata.h"
#include "../pci/pci.h"
#include "debug.h"

#define ATA_DEBUG 1

#define MODULE_INIT ata_init

prdt_struct *prdt;

char *hddmastruct;
size_t ata_irq14done=FALSE;
size_t ata_irq15done=FALSE;

/*
 * Initialize ATA
 *
 * In:  char *init	Initialization string
 *
 * Returns: 0 on success, -1 on failure
 *
 */
size_t ata_init(char *initstring) {
ATA_IDENTIFY ident;
size_t physdiskcount;
prdt_struct *prdt_virtual;

char *memory_alloc_err = "\nata: Error allocating memory for ATA module initialization\n";

prdt=dma_alloc(sizeof(prdt_struct));	/* allocate DMA buffer */
if(prdt == NULL) {
	kprintf_direct("%s\n",memory_alloc_err);
	return(-1);
}

prdt_virtual=(size_t) prdt+KERNEL_HIGH;			/* get virtual address */

prdt_virtual->address=dma_alloc(ATA_DMA_BUFFER_SIZE);	/* allocate dma buffer */

if(prdt_virtual->address == NULL) {
	kprintf_direct("%s\n",memory_alloc_err);
	return(-1);
}


/* Add hard disk partitions */

for(physdiskcount=0x80;physdiskcount < 0x82;physdiskcount++) {  /* for each disk */

	 if(ata_ident(physdiskcount,&ident) == 0) {	/* get ATA information */

	 	if(partitions_init(physdiskcount,&ata_io_pio) == -1) {	/* initalize partitions */
	 		kprintf_direct("ata: Unable to intialize partitions for physical drive %X: %s\n",physdiskcount,kstrerr(getlasterror()));
	 		continue;
	  	}
	 }
}

	 
setirqhandler(14,'ATA$',&irq14_handler);		/* set irq handler for master */
setirqhandler(15,'ATA$',&irq15_handler);		/* set irq handler for slave */

return(0);
}

/*
 * ATA I/O function
 *
 * In:  op		Operation (0=read,1=write)
	physdrive	Physical drive numbe
	block		Block to read
	buf		I/O buffer
 *
 *  Returns: 0 on success, -1 on error
 *
 */	
size_t ata_io_pio(size_t op,size_t physdrive,uint64_t block,uint16_t *buf) {
size_t count;
size_t cylinder;
size_t b;
size_t head;
size_t sector;
size_t islba;
uint16_t controller;
size_t temp;
uint32_t highblock;
uint32_t lowblock;
ATA_IDENTIFY ident;
uint16_t *bb;
uint8_t atastatus;

highblock=((uint64_t) block >> 32);			/* get high block */
lowblock=((uint64_t) block & 0x000000000ffffffff);	/* get low block */

switch(physdrive) {			/* which controller */

	case 0x80:
		controller=0x1f0;					/* primary controller, master */
		break;

	case 0x81:
		controller=0x1f0;					/* secondary controller, slave */
		break;

	case 0x82:
		controller=0x170;					/* secondary controller, master */
	  	break;

	case 0x83:
		controller=0x170;					/* secondary controller, slave */
		break;
}

if(ata_ident(physdrive,&ident) == -1) return(-1);	/* get information */

islba=0;

if(ident.lba28_size != 0) islba=28;	/* is 28bit lba */
if(ident.commands_and_feature_sets_supported2 & 1024) islba=48; /* is 48bit lba */

if(islba == 28) {						/* use lba28 */
	if(block > ident.lba28_size) {			/* invalid block number */
		setlasterror(INVALID_BLOCK_NUMBER);
		return(-1);
	}

	switch(physdrive) {					/* master or slave */
		case 0x80:							/* master */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xe0 | (0 << 4) | ((uint64_t)  lowblock >> 24)  & 0xf);
	   		break;

	  	case 0x81:							/* slave */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xf0 | (1 << 4) | ( (uint64_t) lowblock >> 24) & 0xf);
	   		break;

	  	case 0x82:							/* master */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xe0 | (0 << 4) |  ((uint64_t) lowblock >> 24)  & 0xf);
	   		break;

	  	case 0x83:							/* slave */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xf0 | (1 << 4) | ((uint64_t) lowblock >> 24) & 0xf);
	   		break;
	  }

	outb(controller+ATA_ERROR_PORT,0);
	outb(controller+ATA_SECTOR_COUNT_PORT,1);					/* sector count */

	outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) ((uint64_t) lowblock));			/* bits 0-7 */
	outb(controller+ATA_CYLINDER_LOW_PORT,(uint8_t) ((uint64_t) lowblock >> 8));		/* bits 8-15 */
	outb(controller+ATA_CYLINDER_HIGH_PORT,(uint8_t) ((uint64_t) lowblock >> 16));		/* bits 16-23 */

	if(op == DEVICE_READ)  outb(controller+ATA_COMMAND_PORT,READ_SECTORS);			/* operation */
	if(op == DEVICE_WRITE) outb(controller+ATA_COMMAND_PORT,WRITE_SECTORS);
}

if(islba == 48) {				/* use lba48 */
	if(block > ident.lba48_size) {			/* invalid block number */
		setlasterror(INVALID_BLOCK_NUMBER);
		return(-1);
	}

	switch(physdrive) {						/* master or slave */
	
		case 0x80:							/* master */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0x40 | (0 << 4));
			break;

		case 0x81:							/* slave */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0x50 | (1 << 4));
			break;

		case 0x82:							/* master */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0x40 | (0 << 4));
			break;

		case 0x83:							/* slave */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0x50 | (1 << 4));
			break;
		}

	 outb(controller+ATA_SECTOR_COUNT_PORT,0);					/* sectorcount high byte */
	 outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) (lowblock >> 24));		/* lba 4 */
	 outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) (highblock));			/* lba 5 */
	 outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) (highblock >> 8));		/* lba 6 */
	 outb(controller+ATA_SECTOR_COUNT_PORT,1);					/* sectorcount low byte */
	 outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) (lowblock));			/* lba 1 */
	 outb(controller+ATA_CYLINDER_LOW_PORT,(uint8_t) (lowblock >> 8));		/* lba 2 */
	 outb(controller+ATA_CYLINDER_HIGH_PORT,(uint8_t) (lowblock >> 16));	/* lba 3 */

	if(op == DEVICE_READ) outb(controller+ATA_COMMAND_PORT,READ_SECTORS_EXT);
	if(op == DEVICE_WRITE) outb(controller+ATA_COMMAND_PORT,WRITE_SECTORS_EXT);
}

do {
	atastatus=inb(controller+ATA_COMMAND_PORT);						/* get status */

	if((atastatus & ATA_DRIVE_FAULT) || (atastatus & ATA_ERROR)) {			/* controller returned error */
		kprintf_direct("kernel: drive returned error\n");

		if(op == DEVICE_READ) {
			setlasterror(READ_FAULT);
		}
		else
		{
			setlasterror(WRITE_FAULT);
		}

		return(-1);
	 }
} while((atastatus & ATA_DATA_READY) == 0 || (atastatus & ATA_BUSY));

count=0;
bb=buf;

while(count++ < 512/2) { 
	if(op == DEVICE_READ) *bb++=inw(controller+ATA_DATA_PORT);			/* read data */
	if(op == DEVICE_WRITE) outw(controller+ATA_DATA_PORT,*bb++);			/* write data */
}

return(NO_ERROR);
}

/*
 * PIO ATA I/O function using Cylinder, Head, Sector addressing
 *
 * In:  op		Operation (0=read,1=write)
	physdrive	Physical drive number
	blocksize	Block size
	Head		Head number
	Cylinder	Cylinder number
	Sector		Sector number
	buf		Buffer
 *
 *  Returns: 0 on success, -1 on failure
 *
 */

/* Cylinder/head/sector routine is seperated for use in ata_init */

size_t ata_io_chs (size_t op,size_t physdrive,size_t blocksize,size_t head,size_t cylinder,size_t sector,uint16_t *buf) {
size_t controller;
size_t count;
uint16_t *bb;
ATA_IDENTIFY result;
uint8_t atastatus;
	
switch(physdrive) {
	case 0x80:
		controller=0x1f0;					/* primary controller, master */
		break;

	case 0x81:
		controller=0x1f0;					/* secondary controller, slave */
		break;

	case 0x82:
		controller=0x170;					/* secondary controller, master */
		break;

	case 0x83:
		controller=0x170;					/* secondary controller, slave */
		break;
	}


if(ata_ident(physdrive,&result) == -1) return(-1);	/* bad drive */

switch(physdrive) {						/* master or slave */
	case 0x80:							/* master */
		outb(controller+ATA_DRIVE_HEAD_PORT,(0xA0 | (0 << 4) | head));
		break;

	case 0x81:							/* slave */
		outb(controller+ATA_DRIVE_HEAD_PORT,0xB0 | (1 << 4) | head);
		break;

	case 0x82:							/* master */
		outb(controller+ATA_DRIVE_HEAD_PORT,0xA0 | head);
		break;

	case 0x83:							/* slave */
		outb(controller+ATA_DRIVE_HEAD_PORT,0xB0 | (1 << 4) | head);
		break;
	}

outb(controller+ATA_SECTOR_COUNT_PORT,blocksize);				/* sector count */
outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) sector);				/* sector */
outb(controller+ATA_CYLINDER_LOW_PORT,(uint8_t) (cylinder & 0xff));		/* cylinder low byte */
outb(controller+ATA_CYLINDER_HIGH_PORT,(uint8_t) (cylinder >> 8));			/* cylinder high byte */


if(op == DEVICE_READ)  outb(controller+ATA_COMMAND_PORT,READ_SECTORS);		/* operation */
if(op == DEVICE_WRITE) outb(controller+ATA_COMMAND_PORT,WRITE_SECTORS);


atastatus=(inb(controller+ATA_COMMAND_PORT) & ATA_BUSY);

while(atastatus == ATA_BUSY) {					/* wait for data to be ready */
	 atastatus=inb(controller+ATA_COMMAND_PORT) & ATA_BUSY;						/* get status */

	if((atastatus & ATA_DRIVE_FAULT) || (atastatus & ATA_ERROR) || (atastatus & ATA_RDY)) {			/* controller returned error */
		kprintf_direct("kernel: drive returned error\n");

		if(op == DEVICE_READ) {
			setlasterror(READ_FAULT);
		}
		else
		{
			setlasterror(WRITE_FAULT);
		}

		return(-1);
	}
}

count=0;
bb=buf;

while(count++ < 512/2) {
	 if(op == DEVICE_READ)  *bb++=inw(controller+ATA_DATA_PORT);			/* read data from harddisk */
	 if(op == DEVICE_WRITE) outw(controller+ATA_DATA_PORT,*bb++);			/* write data to harddisk */
}

outb(controller+ATA_COMMAND_PORT,FLUSH);						/* flush buffer */

setlasterror(NO_ERROR);
return(NO_ERROR);
}

/*
 * Get ATA drive information
 *
 * In:  physdrive	Physical drive
	       buf		Buffer to hold information about drivee
 *
 *  Returns: -1 on error, 0 on success
 *
 */
size_t ata_ident(size_t physdrive,ATA_IDENTIFY *buf) {
uint16_t controller;
size_t count;
uint16_t *b;
uint8_t lowbyte;
uint8_t highbyte;

switch(physdrive) {					/* which controller */
	case 0x80:
		controller=0x1f0;					/* primary controller, master */
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xA0);	
		break;

	 case 0x81:
		controller=0x1f0;					/* secondary controller, slave */
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xB0);
		break;

	 case 0x82:
		controller=0x370;					/* secondary controller, master */
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xA0);	
		break;

	 case 0x83:
		controller=0x370;					/* secondary controller, slave */
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xB0);
		break;
}

outb(controller+ATA_SECTOR_COUNT_PORT,0);
outb(controller+ATA_SECTOR_NUMBER_PORT,0);
outb(controller+ATA_CYLINDER_LOW_PORT,0);
outb(controller+ATA_CYLINDER_HIGH_PORT,0);

outb(controller+ATA_COMMAND_PORT,IDENTIFY);				/* send identify command */

if(inb(controller+ATA_COMMAND_PORT) == 0) {		/* drive doesn't exist */
	setlasterror(INVALID_DRIVE);
	return(-1);
}

while((inb(controller+ATA_COMMAND_PORT) & ATA_BUSY) != 0) ;;		/* wait until ready */

lowbyte=inb(controller+ATA_CYLINDER_LOW_PORT);
highbyte=inb(controller+ATA_CYLINDER_HIGH_PORT);

if((highbyte == 0) && (lowbyte == 0)) {
	b=buf;

	for(count=0;count<127;count++) {			/* read result words */
		*b++=inw(controller+ATA_DATA_PORT);
	}

	setlasterror(NO_ERROR);
	return(0);
}

setlasterror(INVALID_DEVICE);
return(-1);
}


/*
 * ATA I/O function using DMA
 *
 * In:  op	Operation (0=read,1=write)
	buf	Buffer
	len	Number of bytes to read/write
 *
 *  Returns: 0 on success, -1 on error
 *
 */	
size_t ata_io_dma(size_t op,size_t physdrive,uint64_t block,uint16_t *buf) {
ATA_IDENTIFY ident;
size_t count;
BLOCKDEVICE blockdevice;
uint16_t barfour;
size_t islba;
prdt_struct *prdt_virtual;
uint32_t status=0;
uint16_t controller;
uint32_t highblock;
uint32_t lowblock;
uint32_t commandregister;

if(ata_ident(physdrive,&ident) == -1) {	/* invalid drive */
	setlasterror(INVALID_DRIVE);
	return(-1);
}

barfour=pci_get_bar4(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER);
if(barfour == -1) {
	kprintf_direct("ata: Unable to get PCI configuration for BAR #4\n");

	return(-1);
}

highblock=((uint64_t) block >> 32);			/* get high block */
lowblock=((uint64_t) block & 0x000000000ffffffff);	/* get low block */

barfour &= 0xFFFFFFFC;			/* clear bottom two bits */

DEBUG_PRINT_HEX(barfour);

/* create prdt */

kprintf_direct("ata_debug: setup PRDT\n");

prdt_virtual=(size_t) prdt+KERNEL_HIGH;			/* get virtual address */

/* address was initialized on module init */
prdt_virtual->last=0x8000;
prdt_virtual->size=512;

/* send information to ata barfour */

if((physdrive == 0x80) || (physdrive == 0x81)) {			/* primary controller */
	ata_irq14done=FALSE;

	kprintf_direct("Primary ATA PRDT\n");

	outb(barfour+PRDT_STATUS_PRIMARY,4);

	outd(barfour+PRDT_ADDRESS_PRIMARY,(uint32_t) prdt); 	/* PRDT physical address */
	outb(barfour+PRDT_COMMAND_PRIMARY,8 | 1); 			/* read/write */
}
else if((physdrive == 0x82) || (physdrive == 0x83)) {			/* secondary controller */
	ata_irq15done=FALSE;

	kprintf_direct("Secondary ATA PRDT\n");

	outb(barfour+PRDT_STATUS_SECONDARY,0);
	outb(barfour+PRDT_COMMAND_SECONDARY,8 | 1); 		/* read/write */
	outd(barfour+PRDT_ADDRESS_PRIMARY,(uint32_t) prdt); 	/* PRDT address */
}


switch(physdrive) {
	case 0x80:
		controller=0x1f0;					/* primary controller, master */
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xA0);	
		break;

	case 0x81:
		controller=0x1f0;					/* primary controller, slave */
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xB0);
		break;

	 case 0x82:
		controller=0x170;					/* secondary controller, master */
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xA0);	
		break;

	 case 0x83:
		controller=0x170;					/* secondary controller, slave */
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xB0);
		break;
	}

islba=0;

if(ident.lba28_size != 0) islba=28;	/* is 28bit lba */
if(ident.commands_and_feature_sets_supported2 & 1024) islba=48; /* is 48bit lba */

if((physdrive == 0x80) || (physdrive == 0x81)) {
	ata_irq14done=FALSE;
}
else
{
	ata_irq15done=FALSE;
}

kprintf_direct("is_lba=%d\n",islba);

if(islba == 28) {						/* use lba28 */

	DEBUG_PRINT_HEX(lowblock);

	switch(physdrive) {					/* master or slave */
	
		case 0x80:							/* master */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xe0 | (0 << 4) | ((lowblock >> 24) & 0xf));
			break;

		case 0x81:							/* slave */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xf0 | (1 << 4) | ((lowblock >> 24) & 0xf));
			break;

		case 0x82:							/* master */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xe0 | (0 << 4) | ((lowblock >> 24) & 0xf));
			break;

		case 0x83:							/* slave */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xf0 | (1 << 4) | ((lowblock >> 24) & 0xf));
			break;
	}



	outb(controller+ATA_SECTOR_COUNT_PORT,1);					/* sector count */
	outb(controller+ATA_FEATURES_PORT,0);

	outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) (lowblock & 0xff));			/* bits 0-7 */
	outb(controller+ATA_CYLINDER_LOW_PORT,(uint8_t) (lowblock >> 8) &  0xff);			/* lba 2 */
	outb(controller+ATA_CYLINDER_HIGH_PORT,(uint8_t) (lowblock >> 16) & 0xff);			/* lba 3 */
	
	commandregister=pci_get_command(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER);
	commandregister |= 0x04;		/* enable bus mastering */
	pci_put_command(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER,commandregister);

	if(op == DEVICE_READ) outb(controller+ATA_COMMAND_PORT,READ_SECTORS_DMA);			/* operation */
	if(op == DEVICE_WRITE) outb(controller+ATA_COMMAND_PORT,WRITE_SECTORS_DMA);
}

if(islba == 48) {						/* use lba48 */
	switch(blockdevice.physicaldrive) {						/* master or slave */
	
		case 0x80:							/* master */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0x40 | (0 << 4));
			break;

		case 0x81:							/* slave */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0x50 | (1 << 4));
			break;

		case 0x82:							/* master */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0x40 | (0 << 4));
			break;

		case 0x83:							/* slave */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0x50 | (1 << 4));
			break;
		}

	 outb(controller+ATA_SECTOR_COUNT_PORT,0);					/* sectorcount high byte */
	 outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) (lowblock >> 24));		/* lba 4 */
	 outb(controller+ATA_CYLINDER_LOW_PORT,(uint8_t) (highblock >> 16));		/* lba 5 */
	 outb(controller+ATA_CYLINDER_HIGH_PORT,(uint8_t) (highblock & 0xff));		/* lba 6 */
	 outb(controller+ATA_SECTOR_COUNT_PORT,1);					/* sectorcount low byte */
	 outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) lowblock);					/* lba 1 */
	 outb(controller+ATA_CYLINDER_LOW_PORT,(uint8_t) (lowblock >> 8));				/* lba 2 */
	 outb(controller+ATA_CYLINDER_HIGH_PORT,(uint8_t) (lowblock >> 16));				/* lba 3 */
	
	commandregister=pci_get_command(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER);
	commandregister |= 0x04;		/* enable bus mastering */
	commandregister=pci_put_command(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER,commandregister);

	if(op == DEVICE_READ)  outb(controller+ATA_COMMAND_PORT,READ_SECTORS_EXT_DMA);			/* operation */
	if(op == DEVICE_WRITE) outb(controller+ATA_COMMAND_PORT,WRITE_SECTORS_EXT_DMA);
}

if(controller == 0x1f0) {			/* wait for IRQ 14 */
	kprintf_direct("waiting for irq 14\n");
	while(ata_irq14done == FALSE) ;;
}

if(controller == 0x370) {			/* wait for IRQ 15 */
	kprintf_direct("waiting for irq 15\n");

	while(ata_irq15done == FALSE) ;;
}

count=0;				/*clear start bit */
if((physdrive == 0x80) || (physdrive == 0x81)) {
	outb(barfour+PRDT_COMMAND_PRIMARY,count);
}

if((physdrive == 0x82) || (physdrive == 0x83)) {			/* which barfour */
	outb(barfour+PRDT_COMMAND_SECONDARY,count);
}

if((inb(ATA_ERROR_PORT) & 1) == 1) {	/* error occurred */
	if(op == DEVICE_READ) {
		setlasterror(READ_FAULT);
	}
	else
	{
		setlasterror(WRITE_FAULT);
	}
	return(-1);
}
	
kprintf_direct("dma buf=%X\n",prdt_virtual->address+KERNEL_HIGH);	
kprintf_direct("buf=%X\n",buf);

memcpy(buf,prdt_virtual->address+KERNEL_HIGH,prdt_virtual->size);
asm("xchg %bx,%bx");

return(NO_ERROR);
}

/*
 * IRQ 14 handler
 *
 * In: nothing
 *
 *  Returns: nothing
 *
 */	
size_t irq14_handler(void) {
uint16_t barfour;

#ifdef ATA_DEBUG
	kprintf_direct("received irq 14\n");
#endif

//asm("xchg %bx,%bx");

barfour=pci_get_bar4(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER);
outb(barfour+PRDT_COMMAND_PRIMARY,0); 			/* clear read/write bit */

ata_irq14done=TRUE;
return;
}

/*
 * IRQ 15 handler
 *
 * In:  nothing
 *
 *  Returns nothing
 *
 */	
size_t irq15_handler(void) {
uint16_t barfour;

#ifdef ATA_DEBUG
	kprintf_direct("received irq 15\n");
#endif

barfour=pci_get_bar4(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER);
outb(barfour+PRDT_COMMAND_PRIMARY,0); 			/* clear read/write bit */

ata_irq15done=TRUE;
return;
}

/*
 * ATA ioctl handler
 *
 * In:  handle	Handle created by open() to reference device
	       request Request number
	       buffer  Buffer
 *
 *  Returns: -1 on error, 0 on success
 *
 */
size_t ata_ioctl(size_t handle,unsigned long request,char *buffer) {
FILERECORD atadevice;
size_t drive;
BLOCKDEVICE bd;

if(gethandle(handle,&atadevice) == -1) {		/* get information about device */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

drive=(uint8_t) (*atadevice.filename-'A');	/* get drive number */

if(getblockdevice(drive,&bd) == -1) return(-1);		/* get device information */

switch(request) {
	case IOCTL_ATA_IDENTIFY: 		/* get information */
		ata_ident(bd.physicaldrive,buffer);
		return;
	  
	default:
		setlasterror(INVALID_PARAMETER);
		return(-1);
}

return(-1);
}

