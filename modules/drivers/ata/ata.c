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
#include "../../../header/kernelhigh.h"
#include "../../../header/errors.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "../../../filemanager/vfs.h"
#include "ata.h"
#include "../pci/pci.h"

#define ATA_DEBUG 1

//#define MODULE_INIT ata_init

prdt_struct *prdt;

size_t ata_io(size_t op,size_t physdrive,size_t block,uint16_t *buf);
size_t ata_io_chs (size_t op,size_t physdrive,size_t blocksize,size_t head,size_t cylinder,size_t sector,uint16_t *buf);
size_t ata_ident(size_t physdrive,ATA_IDENTIFY *buf);
size_t ata_io_dma(size_t op,size_t physdrive,size_t block,uint16_t *buf);
size_t irq14_handler(void);
size_t irq15_handler(void);
size_t ata_init(char *initstring);

char *hddmastruct;
size_t ata_irq14done=FALSE;
size_t ata_irq15done=FALSE;

/*
 * Initialize ATA
 *
 * In:  char *init	Initialization string
 *
 * Returns: nothing
 *
 */
size_t ata_init(char *initstring) {
ATA_IDENTIFY ident;
size_t physdiskcount;
prdt_struct *prdtptr;
size_t p;

prdt=dma_alloc(sizeof(prdt_struct));	/* allocate dma buffer */
if(prdt == -1) {
	kprintf_direct("\nhd: Error allocating memory for hd initialization\n");
	return(-1);
}

p=prdt;
p += KERNEL_HIGH;
prdtptr=p;

prdtptr->address=dma_alloc(ATA_DMA_BUFFER_SIZE);	/* allocate dma buffer */

if(prdtptr->address == -1) {
	kprintf_direct("\nhardddrive: Error allocating memory for initialization\n");
	return(-1);
}


	/* Add hard disk partitions */

for(physdiskcount=0x80;physdiskcount<0x82;physdiskcount++) {  /* for each disk */

	 if(ata_ident(physdiskcount,&ident) == 0) {	/* get ata identify */

	 	if(scan_partitions(physdiskcount,&ata_io) == -1) {	/* can't initalize partitions */
	 		kprintf_direct("harddrive: Unable to intialize partitions for drive %X\n",physdiskcount);
	 		continue;
	  	}
	 
		if((physdiskcount == 0x80) || (physdiskcount == 0x81)) {
			setirqhandler(14,&irq14_handler);		/* set irq handler for master */
	  	}
	  	else
	  	{
	       		setirqhandler(15,&irq15_handler);		/* set irq handler for slave */
	  	}

	 }
}

return(0);
}

/*
 * ATA I/O function
 *
 * In:  op	Operation (0=read,1=write)
	       buf	Buffer
	len	Number of bytes to read/write
 *
 *  Returns: 0 on success, -1 on error
 *
 */	
size_t ata_io(size_t op,size_t physdrive,size_t block,uint16_t *buf) {
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
ATA_IDENTIFY result;
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

if(ata_ident(physdrive,&result) == -1) return(-1);	/* get information */

islba=0;

if(result.lba28_size != 0) islba=28;	/* is 28bit lba */
if(result.commands_and_feature_sets_supported2 & 1024) islba=48; /* is 48bit lba */

if(islba == 28) {						/* use lba28 */
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

	if(op == _READ)  outb(controller+ATA_COMMAND_PORT,READ_SECTORS);			/* operation */
	if(op == _WRITE) outb(controller+ATA_COMMAND_PORT,WRITE_SECTORS);
}

if(islba == 48) {				/* use lba48 */
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


	if(op == _READ) outb(controller+ATA_COMMAND_PORT,READ_SECTORS_EXT);
	if(op == _WRITE) outb(controller+ATA_COMMAND_PORT,WRITE_SECTORS_EXT);
}

do {
	 atastatus=inb(controller+ATA_COMMAND_PORT);						/* get status */

	 if((atastatus & ATA_DRIVE_FAULT) || (atastatus & ATA_ERROR)) {			/* controller returned error */
	  kprintf_direct("kernel: drive returned error\n");
	  setlasterror(DEVICE_IO_ERROR);
	  return(-1);
	 }
} while((atastatus & ATA_DATA_READY) == 0 || (atastatus & ATA_BUSY));

count=0;
bb=buf;

while(count++ < 512/2) { 
	if(op == _READ) *bb++=inw(controller+ATA_DATA_PORT);			/* read data from harddisk */
	if(op == _WRITE) outw(controller+ATA_DATA_PORT,*bb++);			/* write data to harddisk */
}

return(NO_ERROR);
}

/*
 * ATA I/O function using Cylinder, Head, Sector addressing
 *
 * In:  op		Operation (0=read,1=write)
	physdrive	Physical drive number
	blocksize	Block size
	Head		Head number
	Cylinder	Cylinder number
	Sector		Sector number
	buf		Buffer
 *
 *  Returns: nothing
 *
 */
/* chs routine is seperated for use in hd_init */
size_t ata_io_chs (size_t op,size_t physdrive,size_t blocksize,size_t head,size_t cylinder,size_t sector,uint16_t *buf) {
size_t controller;
size_t count;
uint16_t *bb;
ATA_IDENTIFY result;
uint8_t c;
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


if(op == _READ)  outb(controller+ATA_COMMAND_PORT,READ_SECTORS);		/* operation */
if(op == _WRITE) outb(controller+ATA_COMMAND_PORT,WRITE_SECTORS);


atastatus=(inb(controller+ATA_COMMAND_PORT) & ATA_BUSY);

while(atastatus == ATA_BUSY) {					/* wait for data to be ready */
	 atastatus=inb(controller+ATA_COMMAND_PORT) & ATA_BUSY;						/* get status */

	if((atastatus & ATA_DRIVE_FAULT) || (atastatus & ATA_ERROR) || (atastatus & ATA_RDY)) {			/* controller returned error */
		kprintf_direct("kernel: drive returned error\n");
		setlasterror(DEVICE_IO_ERROR);
		return(-1);
	}
}

count=0;
bb=buf;

while(count++ < 512/2) {
	 if(op == _READ)  *bb++=inw(controller+ATA_DATA_PORT);			/* read data from harddisk */
	 if(op == _WRITE) outw(controller+ATA_DATA_PORT,*bb++);			/* write data to harddisk */
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
if(inb(controller+ATA_COMMAND_PORT) == 0) return(-1);		/* drive doesn't exist */

while((inb(controller+ATA_COMMAND_PORT) & 0x80) != 0) {		/* wait until ready */

/* check if not	ata */

	if((inb(ATA_CYLINDER_LOW_PORT) == 0) || (inb(ATA_CYLINDER_HIGH_PORT) == 0)) return(-1);
}

b=buf;

for(count=0;count<127;count++) {			/* read result words */
	*b++=inw(controller+ATA_DATA_PORT);
}

return(NO_ERROR);
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
size_t ata_io_dma(size_t op,size_t physdrive,size_t block,uint16_t *buf) {
uint32_t prdt_phys;
ATA_IDENTIFY result;
size_t count;
BLOCKDEVICE blockdevice;
uint16_t barfour;
size_t islba;
size_t rw=0;
prdt_struct *prdtptr;
size_t size;
size_t p;
uint32_t status=0;
uint16_t controller;
uint16_t control_base;
uint16_t commandregister;

if(ata_ident(physdrive,&result) == -1) return(-1);	/* bad drive */

barfour=pci_get_bar4(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER);
if(barfour == -1) {
	kprintf_direct("ata: can't get PCI configuration for barfour\n");

	return(-1);
}

barfour &= 0xFFFFFFFC;			/* clear bottom two bits */

kprintf_direct("bar4=%X\n",barfour);

/* create prdt */

kprintf_direct("ata_debug: setup prdt\n");

p=prdt;
p += KERNEL_HIGH;
prdtptr=p;

prdtptr->last=0x8000;
prdtptr->size=512;

/* send information to ata barfour */

if((physdrive == 0x80) || (physdrive == 0x81)) {			/* which barfour */
	kprintf_direct("Primary ATA prdt\n");

	outb(barfour+PRDT_STATUS_PRIMARY,inb(barfour+PRDT_STATUS_PRIMARY) | 0x04 | 0x02);
	outb(barfour+PRDT_COMMAND_PRIMARY,8); 		/* read/write */
	outd(barfour+PRDT_ADDRESS1_PRIMARY,(uint8_t) ((size_t) prdt)); /* prdt address */
}

if((physdrive == 0x82) || (physdrive == 0x83)) {			/* which barfour */
	kprintf_direct("Secondary ATA prdt\n");

	outb(barfour+PRDT_STATUS_SECONDARY,inb(barfour+PRDT_STATUS_SECONDARY) | 0x04 | 0x02);	/* enable bus mastering */

	outd(barfour+PRDT_ADDRESS1_SECONDARY,(uint8_t) ((size_t) prdt)); /* prdt address */
	outb(barfour+PRDT_COMMAND_SECONDARY,8+op); 		/* read/write and start bit*/
}


switch(physdrive) {					/* which controller */
	case 0x80:
		controller=0x1f0;					/* primary controller, master */
		control_base=0x3f6;
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xA0);	
		break;

	case 0x81:
		controller=0x1f0;					/* primary controller, slave */
		control_base=0x3f6;
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xB0);
		break;

	 case 0x82:
		controller=0x170;					/* secondary controller, master */
		control_base=0x376;
		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xA0);	
		break;

	 case 0x83:
		controller=0x170;					/* secondary controller, slave */
		control_base=0x376;

		outb(controller+ATA_DRIVE_HEAD_PORT,(uint8_t) 0xB0);
		break;
	}


islba=0;

if(result.lba28_size != 0) islba=28;	/* is 28bit lba */
if(result.commands_and_feature_sets_supported2 & 1024) islba=48; /* is 48bit lba */

kprintf_direct("is_lba=%d\n",islba);

if(islba == 28) {						/* use lba28 */
	switch(blockdevice.physicaldrive) {					/* master or slave */
	
		case 0x80:							/* master */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xe0 | (0 << 4) | ((block >> 24) & 0xf));
			break;

		case 0x81:							/* slave */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xf0 | (1 << 4) | ((block >> 24) & 0xf));
			break;

		case 0x82:							/* master */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xe0 | (0 << 4) | ((block >> 24) & 0xf));
			break;

		case 0x83:							/* slave */
			outb(controller+ATA_DRIVE_HEAD_PORT, 0xf0 | (1 << 4) | ((block >> 24) & 0xf));
			break;
	}

	if((physdrive == 0x80) || (physdrive == 0x81)) {
		ata_irq14done=FALSE;
	}
	else
	{
		ata_irq15done=FALSE;
	}

	outb(controller+ATA_ALT_STATUS_PORT,0);

	outb(controller+ATA_SECTOR_COUNT_PORT,1);					/* sector count */
	outb(controller+ATA_FEATURES_PORT,0);

	outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) (block & 0xff));			/* bits 0-7 */
	outb(controller+ATA_CYLINDER_LOW_PORT,(uint8_t) (block >> 8));				/* lba 2 */
	outb(controller+ATA_CYLINDER_HIGH_PORT,(uint8_t) (block >> 16));				/* lba 3 */
	
	commandregister=pci_get_command(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER);
	commandregister |= 0x04;		/* enable bus mastering */
	commandregister=pci_put_command(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER,commandregister);

	if(op == _READ) outb(controller+ATA_COMMAND_PORT,READ_SECTORS_DMA);			/* operation */
	if(op == _WRITE) outb(controller+ATA_COMMAND_PORT,WRITE_SECTORS_DMA);
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
	 outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) (block >> 24));				/* lba 4 */
	 outb(controller+ATA_CYLINDER_LOW_PORT,0);					/* lba 5 */
	 outb(controller+ATA_CYLINDER_HIGH_PORT,0);					/* lba 6 */
	 outb(controller+ATA_SECTOR_COUNT_PORT,1);					/* sectorcount low byte */
	 outb(controller+ATA_SECTOR_NUMBER_PORT,(uint8_t) block);					/* lba 1 */
	 outb(controller+ATA_CYLINDER_LOW_PORT,(uint8_t) (block >> 8));				/* lba 2 */
	 outb(controller+ATA_CYLINDER_HIGH_PORT,(uint8_t) (block >> 16));				/* lba 3 */
	
	commandregister=pci_get_command(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER);
	commandregister |= 0x04;		/* enable bus mastering */
	commandregister=pci_put_command(0,CLASS_MASS_STORAGE_CONTROLLER,SUBCLASS_MASS_STORAGE_IDE_CONTROLLER,commandregister);

	if(op == _READ)  outb(controller+ATA_COMMAND_PORT,READ_SECTORS_EXT_DMA);			/* operation */
	if(op == _WRITE) outb(controller+ATA_COMMAND_PORT,WRITE_SECTORS_EXT_DMA);
}

if(controller == 0x1f0) {			/* wait for irq */
	kprintf_direct("waiting for irq 14\n");
	while(ata_irq14done == FALSE) ;;
}

if(controller == 0x370) {			/* wait for irq */
	kprintf_direct("waiting for irq 15\n");

	while(ata_irq15done == FALSE) ;;
}

count=0;				/*clear start bit */
if((physdrive == 0x80) || (physdrive == 0x81)) {			/* which controller */
	outb(barfour+PRDT_COMMAND_PRIMARY,count);
}

if((physdrive == 0x82) || (physdrive == 0x83)) {			/* which barfour */
	outb(barfour+PRDT_COMMAND_SECONDARY,count);
}

if((inb(ATA_ERROR_PORT) & 1) == 1) {	/* error occurred */
	setlasterror(DEVICE_IO_ERROR);
	return(-1);
}
	
prdtptr += KERNEL_HIGH;

kprintf_direct("dma buf=%X\n",prdtptr->address+KERNEL_HIGH);	
kprintf_direct("buf=%X\n",buf);

memcpy(buf,prdtptr->address+KERNEL_HIGH,prdtptr->size);
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
kprintf_direct("received irq 14\n");

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
kprintf_direct("received irq 15\n");
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
size_t param;
FILERECORD atadevice;
char *b;
size_t drive;
BLOCKDEVICE bd;

gethandle(handle,&atadevice);		/* get information about device */
drive=(uint8_t) (*atadevice.filename-'A');	/* get drive number */

getblockdevice(drive,&bd);		/* get device information */

switch(request) {
	case IOCTL_ATA_IDENTIFY: 		/* get information */
		ata_ident(bd.physicaldrive,buffer);
		return;
	  
	default:
		return(-1);
}

return(-1);
}

