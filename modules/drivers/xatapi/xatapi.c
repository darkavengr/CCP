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
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "../ata/ata.h"
#include "atapi.h"
#include "debug.h"

#define MODULE_INIT xatapi_init

size_t atapi_irq14_done=FALSE;
size_t atapi_irq15_done=FALSE;

/*
 * Initialize ATAPI
 *
 * In:  char *init	Initialization string
 *
 * Returns: nothing
 *
 */

size_t xatapi_init(char *initstring) {
int physdiskcount;
ATA_IDENTIFY ident;
BLOCKDEVICE blockdevice;
size_t logicaldrive;

/* Add atapi partitions */

for(physdiskcount=0x80;physdiskcount<0x82;physdiskcount++) {  /* for each disk */

	if(atapi_ident(physdiskcount,&ident) == 0) {		/* get atapi drive information */
		logicaldrive=allocatedrive();	/* get drive number */

		ksnprintf(blockdevice.name,"ATAPI%d",MAX_PATH,logicaldrive);
		blockdevice.blockio=&atapi_io_pio;
		blockdevice.ioctl=&atapi_ioctl;
		blockdevice.physicaldrive=physdiskcount;
		blockdevice.drive=logicaldrive;
		blockdevice.sectorsperblock=1;
		blockdevice.sectorsize=ATAPI_SECTOR_SIZE;

		if(add_block_device(&blockdevice) == -1) {	/* add block device */
			kprintf_direct("atapi: Can't register block device %s: %s\n",blockdevice.name,kstrerr(getlasterror()));
			return(-1);
		}
	}
}

return(0);	
}

/*
 * ATAPI PIO function
 *
 * In:  op	Operation (0=read,1=write)
        buf	Buffer
	len	Number of bytes to read/write
 *
 *  Returns: 0 on success, -1 on error
 *
 */	

size_t atapi_io_pio(size_t op,size_t physdrive,uint64_t block,uint16_t *buf) {
uint16_t controller;
uint8_t slavebit;
uint16_t *bb;
size_t count;
ATAPI_PACKET packet;
ATA_IDENTIFY ident;

DEBUG_PRINT_HEX(buf);

if(atapi_ident(physdrive,&ident) == -1) {		/* get atapi drive information */
	kprintf_direct("atapi: Can't get physical drive %X information\n",physdrive);
	return(-1);
}

switch(physdrive) {
	case 0x80:
		controller=0x1f0;					/* primary controller, master */
		slavebit=0;

		break;

	case 0x81:
		controller=0x1f0;					/* secondary controller, slave */
		slavebit=1;
		break;

	case 0x82:
		controller=0x370;					/* secondary controller, master */
		slavebit=0;
		break;

	case 0x83:
		controller=0x370;					/* secondary controller, slave */
		slavebit=1;
		break;
}

outb(controller+ATA_DRIVE_HEAD_PORT,0xA0 | (slavebit << 4));	/* select master or slave */

if(atapi_wait_for_controller_ready(controller) == -1) {	/* wait for controller */
	setlasterror(READ_FAULT);
	return(-1);
}

outb(controller+ATA_FEATURES_PORT,0);		/* use PIO */
		
outb(controller+ATA_CYLINDER_LOW_PORT,(ATAPI_SECTOR_SIZE & 0xff));
outb(controller+ATA_CYLINDER_HIGH_PORT,(ATAPI_SECTOR_SIZE >> 8));

outb(controller+ATA_COMMAND_PORT,ATAPI_PACKET_COMMAND);	/* send ATAPI PACKET command */

if(atapi_wait_for_controller_ready(controller) == -1) {	/* wait for controller */
	setlasterror(READ_FAULT);
	return(-1);
}

/* send packet */

if(op == DEVICE_READ) {				/* read or write */
	packet.opcode=ATAPI_READ12;
}
else
{
	packet.opcode=ATAPI_WRITE12;
}

packet.lba=((uint64_t) block & 0x000000000ffffffff);	/* block number */

bb=&packet;
count=0;

while(count < (sizeof(ATAPI_PACKET)/2)-2) {
	outw(controller+ATA_DATA_PORT,*bb++);			/* send packet data */

	count++;
}

atapi_clear_irq_flag(controller);		/* clear IRQ flag */
atapi_wait_for_irq(controller);		/* wait for IRQ even though it's a PIO command. An IRQ will be received */

count=((inb(controller+ATA_CYLINDER_HIGH_PORT) << 8) | inb(controller+ATA_CYLINDER_LOW_PORT))/2;	/* number of words to read */

bb=buf;

while(count > 0) { 
	if(atapi_wait_for_controller_ready(controller) == -1) {	/* wait for controller */
		if(op == DEVICE_READ) setlasterror(READ_FAULT);
		if(op == DEVICE_WRITE) setlasterror(WRITE_FAULT);

		return(-1);
	}

	if(op == DEVICE_READ) *bb++=inw(controller+ATA_DATA_PORT);			/* read data */
	if(op == DEVICE_WRITE) outw(controller+ATA_DATA_PORT,*bb++);			/* write data */

	count--;
}

setlasterror(NO_ERROR);
return(0);
}

/*
 * ATAPI identify function
 *
 * In:  physdrive	Physical drive
        buf		Buffer
 *
 *
 *  Returns: 0 on success, -1 on error
 *
 */	
size_t atapi_ident(size_t physdrive,ATA_IDENTIFY *buf) {
uint16_t controller;
size_t count;
size_t zerocount=0;
uint8_t atapibytes[4];
uint16_t *b=buf;

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

outb(controller+ATA_COMMAND_PORT,ATAPI_IDENTIFY_PACKET_DEVICE);

while((inb(controller+ATA_COMMAND_PORT) & 0x80) != 0) ;;		/* wait until ready */

/* check if not ATAPI drive */
for(count=2;count<6;count++) {
	atapibytes[count]=inb(controller+count);
}

if(zerocount == 4) {		/* drive doesn't exist */
	setlasterror(INVALID_DRIVE);
	return(-1);
}

if((atapibytes[5] == 0xEB && atapibytes[4] == 0x14) || (atapibytes[5] == 0x96 && atapibytes[4] == 0x69)) {
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
 * Wait for IRQ
 *
 * In: controller	Controller base address  (master=0x1f0, slave=0x370)
 *
 * Returns: Nothing
 *
 */
void atapi_wait_for_irq(size_t controller) {
if(controller == 0x1f0) {			/* wait for IRQ 14 */
	kprintf_direct("atapi: waiting for irq 14\n");
	while(atapi_irq14_done == FALSE) ;;
}

if(controller == 0x370) {			/* wait for IRQ 15 */
	kprintf_direct("atapi: waiting for irq 15\n");

	while(atapi_irq15_done == FALSE) ;;
}

return;
}

/*
 * Wait for ATAPI drive 
 *
 * In:  controller	Base IO port (0x1f0=master, 0x370=slave)
 *
 *
 *
 *  Returns: 0 on success, -1 on error
 *
 */	
size_t atapi_wait_for_controller_ready(uint16_t controller) {
uint8_t atastatus=0;

while(1) {
	atastatus=inb(controller+ATA_STATUS_PORT);

	if(atastatus & ATA_ERROR) return(-1);

	if(((atastatus & ATA_BUSY) == 0) && (atastatus & ATA_DATA_READY)) break;		/* controller is ready */

	atapi_wait(controller);
}

return(0);
}


/*
 * Clear IRQ done flag
 *
 * In:  controller	Base IO port (0x1f0=master, 0x370=slave)
 *
 *  Returns: Nothing
 *
 */
void atapi_clear_irq_flag(uint16_t controller) {
if(controller == 0x1f0) {			/* wait for IRQ 14 */
	atapi_irq14_done=TRUE;
}

if(controller == 0x370) {			/* wait for IRQ 15 */
	atapi_irq15_done=TRUE;
}


}

/*
 * ATAPI delay
 *
 * In:  controller	Base IO port (0x1f0=master, 0x370=slave)
 *
 *  Returns: Nothing
 *
 */
void atapi_wait(uint16_t controller) {
inb(controller+ATA_CONTROL_PORT+ATA_ALT_STATUS_PORT);
inb(controller+ATA_CONTROL_PORT+ATA_ALT_STATUS_PORT);
inb(controller+ATA_CONTROL_PORT+ATA_ALT_STATUS_PORT);
inb(controller+ATA_CONTROL_PORT+ATA_ALT_STATUS_PORT);
return;
}

/*
 * ATAPI ioctl handler
 *
 * In:  handle	Handle created by open() to reference device
	       request Request number
	       buffer  Buffer
 *
 *  Returns: -1 on error, 0 on success
 *
 */
size_t atapi_ioctl(size_t handle,unsigned long request,char *buffer) {
FILERECORD atapidevice;
size_t drive;
BLOCKDEVICE bd;

if(gethandle(handle,&atapidevice) == -1) {		/* get information about device */
	setlasterror(INVALID_HANDLE);
	return(-1);
}

drive=(uint8_t) (*atapidevice.filename-'A');	/* get drive number */

if(getblockdevice(drive,&bd) == -1) return(-1);		/* get device information */

switch(request) {
	case IOCTL_ATAPI_IDENTIFY: 		/* get information */
		return(atapi_ident(bd.physicaldrive,buffer));

	case IOCTL_ATAPI_EJECT:			/* eject drive */
		return(0);

	//	return(atapi_eject(bd.physdrive));
}

setlasterror(INVALID_PARAMETER);
return(-1);
}

