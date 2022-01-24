/*  CCP Version 0.0.1
    (C) Matthew Boote 2020

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

/* CCP floppy driver */
/* based on floppy tutorial from http://bos.asmhackers.net/docs/floppy/docs/floppy_tutorial.txt */

#include <stdint.h>
#include <stddef.h>
#include "../../../header/errors.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "floppy.h"

#define MODULE_INIT floppy_init

size_t checkstatus(void);
unsigned int delay_loop(size_t delaycount);
void floppy_writecommand(size_t drive,uint8_t c);
size_t sector_io(unsigned int op,uint8_t drive,uint16_t head,uint16_t cyl,uint16_t sector,size_t blocksize,char *buf);
void waitforirq6(void);
extern tickcount;
volatile unsigned int irq6done=FALSE;
void wait_for_irq6(void);
extern setirqhandler();

size_t floppybuf;

uint8_t st0;
uint8_t cyl;
uint8_t st[7];

//#define FLOPPY_DEBUG

#define KERNEL_HIGH (1 << (sizeof(size_t)*8)-1)

struct {
 uint8_t steprate_headunload;
 uint8_t headload_ndma;
 uint8_t motor_delay_off;
 uint8_t bytes_per_sector;
 uint8_t sectors_per_track;
 uint8_t gap_length;
 uint8_t data_length; 
 uint8_t format_gap_length;
 uint8_t filler;
 uint8_t motor_start_time; 
 uint8_t head_settle_time;
} *floppy_parameters=KERNEL_HIGH+0x000fefc7;

char *floppytype[] = { "None","5.25\" 360k","5.25\" 1.2M","3.5\" 720K","3.5\" 1.44M","3.5\" 2.88M" };
struct {
 unsigned int sectorspertrack;
 unsigned int numberofheads;
 unsigned int numberofsectors;
} fdparams[] = { {0,0,0},\
		 { 8,2,720 },\
       { 15,2,2400 },\
       { 9,2,1400 },\
       { 18,2,2880 },\
       { 15,2,2400 },\
       { 36,2,5760 } };

/* switch motor on */
void motor_on(size_t drive) {

#ifdef FLOPPY_DEBUG
 kprintf("fd debug: enable motor\n");
#endif

if(drive == 0) {
 outb(DIGITAL_OUTPUT_REGISTER,MOTA | IRQ | NOT_RESET | DSEL0);			/* floppy 0 motor on */ 
 return;
}
else
{
 outb(DIGITAL_OUTPUT_REGISTER,MOTB | IRQ | NOT_RESET | DSEL1);			/* floppy 1 motor on */ 
 return;
}

////delay_loop(500);								/* wait for floppy head to settle */

}

/* switch motor off */

void motor_off(size_t drive) {
#ifdef FLOPPY_DEBUG
 kprintf("fd debug: disable motor\n");
#endif

 switch(drive) {
 
  case 0:				/* floppy 0*/
   outb(DIGITAL_OUTPUT_REGISTER,(IRQ | NOT_RESET) & DSEL0);			/* floppy 0 motor off */ 
   return;
   
  case 1:				/* floppy 1*/ 
   outb(DIGITAL_OUTPUT_REGISTER,(IRQ | NOT_RESET) & DSEL1);			/* floppy 1 motor off */  
   return;
  }
}
 
void initialize_floppy(size_t drive) {
uint8_t floppytype;
char *z[10];
uint8_t dir;

#ifdef FLOPPY_DEBUG
kprintf("fd debug: initialize floppy: reset\n");
#endif

   reset_controller(drive);	

#ifdef FLOPPY_DEBUG
kprintf("fd debug: initialize floppy: configure\n");
#endif

  floppy_writecommand(drive,CONFIGURE);			/* configure drive */
  floppy_writecommand(drive,0);
  floppy_writecommand(drive,(IMPLIED_SEEK << 6) | (FIFO_DISABLED << 5) | (DRIVE_POLLING_DISABLE << 4) | (THRESHOLD-1));
  floppy_writecommand(drive,0);

#ifdef FLOPPY_DEBUG
 kprintf("fd debug: initialize floppy: specify\n");
#endif
 
  floppy_writecommand(drive,SPECIFY);			/* config/specify command */
  floppy_writecommand(drive,8 << 4 | floppy_parameters->steprate_headunload);
  floppy_writecommand(drive,floppy_parameters->headload_ndma << 1 | 0 );

 outb(0x70,0x10);					/* get floppy disk type */
floppytype=inb(0x71);

 if(drive == 0) {					/* drive a */
  floppytype=floppytype >> 4;
 }
 else
 {							/* drive b */
  floppytype=floppytype & 0x0f; 
 }

 switch(floppytype) {					/* select type */

   case 1:						/* 5.25" 360k */
    outb(CONFIGURATION_CONTROL_REGISTER,0);
    outb(DATARATE_SELECT_REGISTER,0);
    break;

   case 2:						/* 5.25" 1.2m */
    outb(CONFIGURATION_CONTROL_REGISTER,0);
    outb(DATARATE_SELECT_REGISTER,0);
    break;

   case 3:						/* 3.5" 720k */
    outb(CONFIGURATION_CONTROL_REGISTER,0);
    outb(DATARATE_SELECT_REGISTER,0);
    break;

   case 4:						/* 3.5" 1.44m */
    outb(CONFIGURATION_CONTROL_REGISTER,0);
    outb(DATARATE_SELECT_REGISTER,0);
    break;

   case 5:						/* 3.5" 2.88m */
    outb(CONFIGURATION_CONTROL_REGISTER,0);
    outb(DATARATE_SELECT_REGISTER,0);
    break;

  default:
    kprintf("kernel: Unknown floppy type\n");
    return(-1);
  }
 
 motor_on(drive);					/* enable motor */ 
 
#ifdef FLOPPY_DEBUG
 kprintf("fd debug: initialize floppy: recalibrate\n");
#endif

  floppy_writecommand(drive,RECALIBRATE);			/* recalibrate */
  floppy_writecommand(drive,drive);

  if(checkstatus() == -1) return(-1);

 irq6done=FALSE;
 waitforirq6();	/* until ready */

  motor_off(drive);					/* disable motor */ 

  return;
}


unsigned int getstatus(size_t drive) {
 while(inb(MAIN_STATUS_REG) & RQM == RQM) ;;

 floppy_writecommand(drive,SENSE_INTERRUPT);		/* get status */

 st0=inb(DATA_REGISTER);
 cyl=inb(DATA_REGISTER);
 return;
}
/* read/write sector */

size_t sector_io(unsigned int op,uint8_t drive,uint16_t head,uint16_t cyl,uint16_t sector,size_t blocksize,char *buf) {
unsigned int count;
unsigned int result;
unsigned int retrycount;
unsigned int dir;

initialize_floppy(drive);

 motor_on(drive);					/* enable motor */ 

// dir=inb(DIGITAL_INPUT_REGISTER);		/* check if disk in drive */

// if((dir & 0x80)) {
//  dir &= 0x80;
  
//  outb(DIGITAL_INPUT_REGISTER,dir);
//  setlasterror(DRIVE_NOT_READY);
//  return(-1);
// }

#ifdef FLOPPY_DEBUG
 kprintf("fd debug: sector io: seek\n");
#endif

 floppy_writecommand(drive,SEEK);				/* seek track */
 floppy_writecommand(drive,head << 2 | drive);
 floppy_writecommand(drive,cyl);

irq6done=FALSE;
waitforirq6();	/* until ready */

#ifdef FLOPPY_DEBUG
 kprintf("fd debug: sector io: seek sense interrupt\n"); 
#endif

 floppy_writecommand(drive,SENSE_INTERRUPT);		/* get status but don't get result bytes */

 st[0]=inb(DATA_REGISTER);		/* get result  */


 #ifdef FLOPPY_DEBUG
  kprintf("fd debug: sector io: copy to buffer for write\n"); 
 #endif

 if(op == _WRITE) memcpy((char *) floppybuf+KERNEL_HIGH,buf,blocksize); 		/* copy to from sector buffer to buffer */

/* taken from http://wiki.osdev.org/ISA_DMA#Floppy_Disk_DMA_Initialization
   initialize floppy DMA */

for(retrycount=0;retrycount < RETRY_COUNT;retrycount++) {
 #ifdef FLOPPY_DEBUG
  kprintf("fd debug: sector io: enable dma\n"); 
 #endif

 outb(0xa,6);					/* mask DMA channel 2 and 0 (assuming 0 is already masked) */
 outb(0x0c, 0xFF);					/* reset the master flip-flop */
 outb(0x04,(uint8_t) (floppybuf & 0x0000FF)); 	/* address low byte */
 outb(0x04,(uint8_t) (floppybuf >> 8));		/* address to high byte */
 outb(0x0c,0xFF); 					/* reset the master flip-flop */
 outb(0x05,(uint8_t) (511 & 0x00FF));			/* count low byte */
 outb(0x05,(uint8_t) (511 >> 8));			/* count high byte */
 outb(0x81,(uint8_t) (floppybuf >> 16));		/* external page register to 0 for total address of SECTOR_BUF */
 outb(0x0a,0x02); 					/* unmask DMA channel 2 */

/*******************/
 
 //delay_loop((unsigned int) floppy_parameters->head_settle_time);			/* wait for floppy head to settle */

 outb(0x0a,0x06);					/* mask DMA channel 2 and 0 (assuming 0 is already masked) */
 if(op == _READ) outb(0x0b,0x56);		        /* 01011010 single transfer, address increment, autoinit, read, channel 2 */
 if(op == _WRITE) outb(0x0b,0x5A);		        /* 01010110 single transfer, address increment, autoinit, write, channel 2 */
 outb(0x0a,0x02);				      	/* unmask DMA channel 2 */


#ifdef FLOPPY_DEBUG
 kprintf("fd debug: sector io: send command for i/o\n"); 
#endif

 irq6done=FALSE;
 if(op == _READ) floppy_writecommand(drive, BIT_MF | READ_DATA);	/* read or write */
 if(op == _WRITE) floppy_writecommand(drive,BIT_MF | WRITE_DATA);

 floppy_writecommand(drive,(head << 2) | drive);
 floppy_writecommand(drive,cyl);
 floppy_writecommand(drive,head);
 floppy_writecommand(drive,sector);
 floppy_writecommand(drive,2); 				/* sector size = 128*2^size */
 floppy_writecommand(drive,1);				 /* number of sectors to transfer */
 floppy_writecommand(drive,27);			        /* default gap value */
 floppy_writecommand(drive,0xff);		      /* default value for data length */

#ifdef FLOPPY_DEBUG
 kprintf("fd debug: sector io: wait for io irq6\n"); 
#endif

irq6done=FALSE;
waitforirq6();	/* until ready */

#ifdef FLOPPY_DEBUG
 kprintf("fd debug: sector io: read io status\n"); 
#endif

break;
}

 motor_off(drive);

#ifdef FLOPPY_DEBUG
 kprintf("fd debug: sector io: copy from buffer for read\n"); 
#endif

#ifdef FLOPPY_DEBUG
 kprintf("fd debug: sector io: copied ok\n"); 
#endif

if(op == _READ) memcpy((void *) buf,(char *) floppybuf+KERNEL_HIGH,blocksize); 		/* copy to from sector buffer to buffer */

 setlasterror(NO_ERROR);
 return(NO_ERROR);
}

unsigned int fd_io(unsigned int op,size_t drive,size_t block,char *buf) {
BLOCKDEVICE blockdevice;
BLOCKDEVICE *old;
unsigned int count;
unsigned int retrycount;
uint16_t cylinder;
uint16_t temp;
uint16_t head;
uint16_t sector;
char *b;
uint8_t floppytype;
void *bootbuf;
b=buf;
unsigned int rv;


/* if this is the first time the floppy drive has been used, there will
 be no bios paramter block read in, so it must be read into the drive struct */

bootbuf=kernelalloc(1024);			/* allocate temporary buffer */
if(bootbuf == NULL) return(-1);

getblockdevice(drive,&blockdevice);

//if(block > (blockdevice.numberofsectors/blockdevice.sectorsperblock)) return(-1); /* bad block */
	
count=0;
while(count++ < blockdevice.sectorsperblock) {
 if(block == 0) {
  cylinder=0;
  head=0;
  sector=1;
 }
 else
 {
  cylinder=block / (blockdevice.numberofheads  * blockdevice.sectorspertrack);			/* convert block number to CHS */
  temp=block % (blockdevice.numberofheads * blockdevice.sectorspertrack);
  head=temp / blockdevice.sectorspertrack;
  sector=temp % blockdevice.sectorspertrack+1;
}
 
#ifdef FLOPPY_DEBUG
 kprintf("fd debug: sector i/o\n");
#endif

if(sector_io(op,(uint8_t) drive,head,cylinder,sector,512,b) == -1) {
   if(floppytype == 2 || floppytype == 4) {		/* double density disk in high density drive */
    outb(CONFIGURATION_CONTROL_REGISTER,2);
    outb(DATARATE_SELECT_REGISTER,2);
   }

   if(floppytype == 5) {					/* high density disk in extra high density drive */
    outb(CONFIGURATION_CONTROL_REGISTER,3);
    outb(DATARATE_SELECT_REGISTER,3);
    
    floppy_writecommand(drive,PERPENDICULAR_MODE);				/* enable perpendicular mode */

    if(drive == 0)  {
     floppy_writecommand(drive,1 << 2);
    }
     else
    {
     floppy_writecommand(drive,1 << 3);
    }
   }

if(sector_io(op,(uint8_t) drive,head,cylinder,sector,512,b) == -1) return(-1);
}
  block++;
  b=b+blockdevice.sectorsize;
}

 setlasterror(NO_ERROR);
 return(NO_ERROR);
}



void irq6_handler(void) {
//#ifdef FLOPPY_DEBUG
// kprintf("fd debug: IRQ6 received\n");
//#endif

 irq6done=TRUE;
 return;
}

void reset_controller(size_t drive) {
#ifdef FLOPPY_DEBUG
 kprintf("fd_debug: controller reset\n");
#endif

  irq6done=FALSE;
 outb(DIGITAL_OUTPUT_REGISTER,0);			/* reset controller */
 outb(DIGITAL_OUTPUT_REGISTER,0x0c);

irq6done=FALSE;
waitforirq6();	/* until ready */

  getstatus(drive);						/* get return values */
}

void floppy_writecommand(size_t drive,uint8_t c) {
 if((inb(MAIN_STATUS_REG) & 0xc0) != 0x80) {	/* need to reset */
  reset_controller(drive);	
  motor_on(drive);					/* enable motor */ 
 }

 outb(DATA_REGISTER,c);				/* write data */
} 	



size_t floppy_init(char *init) {
BLOCKDEVICE fdstruct;
unsigned int floppycount=0;
uint8_t ftype;
unsigned int count;				 

floppybuf=dma_alloc(512);					/* allocate dma buffer */
if(floppybuf == -1) {					/* can't alloc */
 kprintf("floppy: can't allocate dma buffer\n");
 return(-1);
}

#ifdef FLOPPY_DEBUG
 kprintf("dma buf=%X\n",floppybuf);
#endif

outb(0x70,0x10);					/* check floppy disk presense */

ftype=inb(0x71);

if(ftype == 0) {				/* no floppy drives */
 kprintf("kernel: No floppy drives found\n");
 return(-1);
} 

if((ftype >> 4) != 0) floppycount++;		/* number of floppy drives */
if((ftype & 0x0f) != 0) floppycount++;

outb(0x70,0x10);					/* check floppy disk presence */
ftype=inb(0x71);

for(count=0;count<floppycount;count++) {
 if(count == 0) {
  ftype=ftype >> 4;
 }
 else
 {
  ftype=ftype & 0x0f;
 }

 fdstruct.sectorspertrack=fdparams[ftype].sectorspertrack;
 fdstruct.numberofheads=fdparams[ftype].numberofheads;
 fdstruct.numberofsectors=fdparams[ftype].numberofsectors;
 
  if(count == 0) {
   strcpy(fdstruct.dname,"FD0");
   fdstruct.blockio=&fd_io;		/*setup struct */
   fdstruct.ioctl=NULL;
   fdstruct.physicaldrive=0;
   fdstruct.drive=0;
   fdstruct.flags=0;
   fdstruct.startblock=0;
   fdstruct.sectorsperblock=1;
  }
  else
  {
   strcpy(fdstruct.dname,"FD1");
   fdstruct.blockio=&fd_io;		/*setup struct */
   fdstruct.ioctl=NULL;
   fdstruct.physicaldrive=1;
   fdstruct.drive=1;
   fdstruct.flags=0;
   fdstruct.startblock=0;
   fdstruct.sectorsize=512;
   fdstruct.sectorsperblock=1;
  }

  add_block_device(&fdstruct);	 /* drive a */
 }

setirqhandler(6,&irq6_handler);		/* set irq handler */
}	

void waitforirq6(void) { 
while(irq6done == FALSE) ;;

return;	/* wait for interrupt */
 
}

size_t checkstatus(void) {
uint8_t st[8];

st[0]=inb(MAIN_STATUS_REG);		/* get result  */
st[1]=inb(MAIN_STATUS_REG);		/* get result  */
st[2]=inb(MAIN_STATUS_REG);		/* get result  */

if((st[0] & 0x80) || (st[0] & 0xc0) || (st[0] & 0x40)) {
 if((st[0] & 8)) {
  setlasterror(DRIVE_NOT_READY);
  return(-1);
 }

 if((st[1] & 0x80) || (st[1] & 0x40) || (st[1] & 0x10) || (st[1] & 0x8) || (st[1] & 0x4) || (st[1] & 0x2) || (st[1] & 0x1)) {
  setlasterror(GENERAL_FAILURE);
  return(-1);
 }

 if((st[2] & 0x40) || (st[2] & 0x20) || (st[2] & 0x10) || (st[2] & 0x8) || (st[2] & 0x4) || (st[2] & 0x2) || (st[2] & 0x1)) {
  setlasterror(GENERAL_FAILURE);
  return(-1);
 }

 if((st[3] & 0x40)) {
  setlasterror(WRITE_PROTECT_ERROR);
  return(-1);
 }

 if((st[3] & 0x20) || (st[1] & 0x10) || (st[1] & 0x8) || (st[1] & 0x4) || (st[1] & 0x2) || (st[1] & 0x1)) {
  setlasterror(GENERAL_FAILURE);
  return(-1);
 }
}

return(0);
}
