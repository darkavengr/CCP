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
#include <stddef.h>
#include "../../processmanager/mutex.h"
#include "../../devicemanager/device.h"
#include "../../filemanager/vfs.h"

#define NULL 0
#define GPT_BLOCK_SIZE 65536

void partitions_init(size_t physdrive,size_t (*handler)(size_t,size_t,size_t,void *));
size_t gpt_hd_init(size_t physdrive,size_t (*handler)(size_t,size_t,size_t,void *));

struct partitions { 
 uint8_t bootableflag;
 uint8_t starthead;
 uint8_t startsectorcylinder;
 uint8_t startcylinder;
 uint8_t type;
 uint8_t endhead;
 uint8_t endsectorcylinder;
 uint8_t endcylinder;
 uint32_t firstsector;
 uint32_t numberofsectors;
};

/*
 * Initialize partitions; add partitions as drives to block devices
 *
 * In: size_t physdrive								Physical drive
       size_t (*handler)(size_t,size_t,size_t,void *)	I/O handler function to call to read blocks
 *
 * Returns 0
 * 
 */
void scan_partitions(size_t physdrive,size_t (*handler)(size_t,size_t,size_t,void *)) {
size_t partition_count;
size_t head;
size_t cyl;
size_t sector;
void *buf[512];
void *bootbuf[512];
BLOCKDEVICE hdstruct;
struct partitions partition[3];
size_t relstart;
size_t hdcount;

if(handler(_READ,physdrive,0,buf) == -1) return(-1);	/* read boot sector */


memcpy((void *) partition,(void *) buf+0x1be,64);  				/* copy partition table to struct */

/* for each partition */

for(partition_count=0;partition_count<3;partition_count++) {

if(partition[partition_count].type == 0) continue;				/* empty entry */

if(partition[partition_count].type == 0xee) {		 /* guid partition */
 gpt_hd_init(physdrive,handler);
 return(0);
 }

if(partition[partition_count].type == 0x5) {				/* extended partition */
 relstart=partition[0].firstsector;			/* for lba addressing */

 while(partition[1].firstsector != 0) {			/* until end of chain */
  handler(_READ,relstart,partition[partition_count].firstsector,bootbuf); 

	/* the rest is updated later */

  hdstruct.sectorspertrack=(partition[partition_count].endsectorcylinder & 0x3f);
  hdstruct.numberofheads=partition[partition_count].endhead;
  hdstruct.sectorsperblock=1;
  hdstruct.blockio=handler;					/* Add device handlers */			
  hdstruct.startblock=partition[partition_count].firstsector;
  hdstruct.physicaldrive=physdrive;			/* physicaldrive */
  hdstruct.drive=allocatedrive();
  hdstruct.flags=DEVICE_FIXED;
  hdstruct.numberofsectors=partition[partition_count].numberofsectors;

  ksprintf(hdstruct.dname,"HD%d",hdstruct.drive);
  hdcount++;

  add_block_device(&hdstruct);
        
  sector=(partition[1].startsectorcylinder & 0x3f);
  cyl=((partition[1].startsectorcylinder & 0xc0) << 2)+partition[partition_count].startcylinder;
  head=partition[1].starthead;

  /* find next in chain */
  handler(_READ,physdrive,partition[1].firstsector,bootbuf);

  memcpy((void *) partition,(void *) bootbuf+0x1be,64);  				/* copy partition table to struct */
	 	
   hdcount++;

   relstart=relstart+partition[0].firstsector;			/* for lba addressing */
  }

  continue;									/* continue to next disk */
 }

/* primary partitions */

  hdcount++;
  
  sector=(partition[partition_count].startsectorcylinder);
  cyl=((partition[partition_count].startsectorcylinder & 0xc0) << 2)+partition[partition_count].startcylinder;
  head=partition[partition_count].starthead;

  handler(_READ,physdrive,partition[partition_count].firstsector,bootbuf);


  hdstruct.sectorspertrack=(partition[partition_count].endsectorcylinder & 0x3f);
  hdstruct.numberofheads=partition[partition_count].endhead;

  hdstruct.blockio=handler;					/* Add device handlers */			
  hdstruct.startblock=partition[partition_count].firstsector;
  hdstruct.physicaldrive=physdrive;			/* physicaldrive */
  hdstruct.sectorsize=512;
  hdstruct.drive=allocatedrive();
  hdstruct.sectorsperblock=1;
  hdstruct.flags=DEVICE_FIXED;
  hdstruct.numberofsectors=partition[partition_count].numberofsectors;

  ksprintf(hdstruct.dname,"HD%d",hdstruct.drive);

  add_block_device(&hdstruct);

  hdcount++;

}
    

 if(hdcount == 0) return(-1); 	/* no partitions found */
 return(0);	
}

/*
 * Initialize GPT partitions; add partitions as drives to block devices
 *
 * In: size_t physdrive								Physical drive
       size_t (*handler)(size_t,size_t,size_t,void *)	I/O handler function to call to read blocks
 *
 * Returns 0
 * 
 */
size_t gpt_hd_init(size_t physdrive,size_t (*handler)(size_t,size_t,size_t,void *)) {
 size_t partition_count;
 char *z[10];
 BLOCKDEVICE *next;
 BLOCKDEVICE *last;
 BLOCKDEVICE hdstruct;
 size_t size;
 size_t count;
 size_t sector;
 char *buf[MAX_PATH];

 struct guidhead { 
  char sig[8];
  uint32_t revision;
  uint32_t header_size;
  uint32_t crc;
  uint32_t reserved;
  uint64_t currentlba;
  uint64_t backuplba;
  uint64_t first_lba;
  uint64_t last_lba;
  uint64_t guid_high;
  uint64_t guid_low;
  uint64_t partition_arraystart;
  uint32_t partition_count;
  uint32_t pa_crc;
  char *padding[420];
 } guid_header;

 struct guidpart { 
  uint64_t partition_guid;
  uint64_t first_lba;
  uint64_t last_lba;
  uint64_t attributes;
  char *partition_name[72];
 } guid_partition;

  BLOCKDEVICE bpb;
  struct guidpart *guidptr;
  struct guidpart *guidbuf;

  	
  guidbuf=kernelalloc(GPT_BLOCK_SIZE);
  if(guidbuf == NULL) return(-1);			/* no memory */

  handler(_READ,physdrive,1,guidbuf);		/* read guid header */
  memcpy(&guid_header,guidbuf,sizeof(struct guidhead));

 size=(guid_header.partition_count*128)/512;		/* size of partition list */
 if((guid_header.partition_count*128) % 512 > 0) size++;		/* size of partition list */

 guidptr=guidbuf;					/* point to buffer */
 sector=guid_header.partition_arraystart;

for(count=0;count<size;count++) {		/* for each sector */
 handler(_READ,physdrive,sector,guidbuf);
 sector++;

 guidptr=guidbuf;

/* for each partition entry in sector */
 for(partition_count=0;partition_count<512/128;partition_count++) {
   memcpy(&guid_partition,guidptr,sizeof(struct guidpart));

   if(guid_partition.partition_guid == 0) continue;

    /* Read boot sector of partition to logical drives list */
 
    hdstruct.blockio=handler;					/* Add device handlers */			
    hdstruct.startblock=guidptr->first_lba;
    hdstruct.physicaldrive=physdrive;			/* physicaldrive */
    hdstruct.sectorsize=512;
    hdstruct.drive=allocatedrive();
    hdstruct.flags=DEVICE_FIXED;
    hdstruct.numberofsectors=guidptr->last_lba-guidptr->first_lba;
    hdstruct.ioctl=NULL;
    ksprintf(hdstruct.dname,"HD%d",hdstruct.drive);

    hdstruct.startblock=guidptr->first_lba;	
    add_block_device(&hdstruct);

   guidptr +=sizeof(struct guidhead);	/* point to next record */ 
  }
 }

  kernelfree(guidbuf);
  return;
 }
