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
#include "../../../header/errors.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "atapi.h"
#include "../ata/ata.h"

#define MODULE_INIT atapi_init

size_t atapi_send_command(size_t physdrive,atapipacket *atapi_packet);
size_t atapi_io(size_t op,size_t physdrive,size_t block,uint16_t *buf);
size_t atapi_ident(size_t physdrive,ATA_IDENTIFY *buf);
size_t atapi_init(char *initstring);

/*
 * Send ATAPI command
 *
 * In:  physdrive	Physical drive
        atapi_packet	ATAPI packet with information about transfer
 *
 *  Returns: -1 on error, 0 on success
 *
 */
size_t atapi_send_command(size_t physdrive,atapipacket *atapi_packet) {
uint16_t controller;
int slavebit;
uint16_t *bb;
size_t count;
uint8_t atastatus;

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

		break;
}

outb(controller,slavebit);
		
outb(controller+ATA_SECTOR_COUNT_PORT,0);					/* sectorcount high byte */
outb(controller+ATA_SECTOR_COUNT_PORT,1);					/* sectorcount low byte */

outb(controller,ATAPI_PACKET);


do {
	atastatus=inb(controller+ATA_COMMAND_PORT);						/* get status */

	if((atastatus & ATA_DRIVE_FAULT) || (atastatus & ATA_ERROR)) {			/* controller returned error */
		kprintf_direct("kernel: drive returned error\n");

		setlasterror(DEVICE_IO_ERROR);
		return(-1);
	}
} while((atastatus & ATA_DATA_READY) == 0 || (atastatus & ATA_BUSY));

/* send packet */

bb=atapi_packet;
count=0;

while(count++ < sizeof(atapi_packet)/2) { 
	outw(controller+ATA_DATA_PORT,*bb++);			/* send packet data */
}

}

/*
 * ATAPI I/O function
 *
 * In:  op	Operation (0=read,1=write)
        buf	Buffer
	len	Number of bytes to read/write
 *
 *  Returns: 0 on success, -1 on error
 *
 */	
size_t atapi_io(size_t op,size_t physdrive,size_t block,uint16_t *buf) {
}

size_t atapi_ident(size_t physdrive,ATA_IDENTIFY *buf) {
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
outb(controller+ATA_SECTOR_COUNT_PORT,1);
 
outb(controller+ATA_COMMAND_PORT,ATAPI_PACKET);				/* send packet command */	

if(inb(controller+ATA_COMMAND_PORT) == 0) return(-1);		/* drive doesn't exist */

while((inb(controller+ATA_COMMAND_PORT) & 0x80) != 0) ;;		/* wait until ready */

/* check if not atapi */
 if((inb(ATA_CYLINDER_LOW_PORT) != 0) && (inb(ATA_CYLINDER_HIGH_PORT) != 0)) return(-1);

b=buf;

for(count=0;count<127;count++) {			/* read result words */
	*b++=inw(controller+ATA_DATA_PORT);
}

return(0);
}

/*
 * Initialize ATAPI
 *
 * In:  char *init	Initialization string
 *
 * Returns: nothing
 *
 */

size_t atapi_init(char *initstring) {
int physdiskcount;
ATA_IDENTIFY ident;

/* Add atapi partitions */

for(physdiskcount=0x80;physdiskcount<0x82;physdiskcount++) {  /* for each disk */

	if(atapi_ident(physdiskcount,&ident) == -1) continue;		/* get ata identify */

	if(partitions_init(physdiskcount,&atapi_io) == -1) {	/* can't initalize partitions */
		kprintf_direct("atapi: Unable to intialize partitions for drive %X\n",physdiskcount);
		return(-1);
	}
}

return(0);	
}

