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

#include <stdint.h>
#include <stddef.h>
#include "../../../header/errors.h"
#include "serialio.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "../../../filemanager/vfs.h"

#define MODULE_INIT serialio_init

extern setirqhandler();

uint8_t comport(unsigned int op,uint16_t port,char *buf,size_t size);
void com1(unsigned int op,char *buf,size_t size);
void com2(unsigned int op,char *buf,size_t size);
void com3(unsigned int op,char *buf,size_t size);
void com3(unsigned int op,char *buf,size_t size);
void com1_irq_handler(void *regs);
void com2_irq_handler(void *regs);
void com3_irq_handler(void *regs);
void com4_irq_handler(void *regs);
void serialio_init(char *init);
size_t serial_ioctl(size_t handle,unsigned long request,char *buffer);

port ports[]={ { "com1",COM1,115200,8,0,0,0,NULL,NULL,NULL,0,1024 },\
	       { "com2",COM2,115200,8,0,0,0,NULL,NULL,NULL,0,1024 },\
	       { "com3",COM3,115200,8,0,0,0,NULL,NULL,NULL,0,1024 },\
	       { "com4",COM4,115200,8,0,0,0,NULL,NULL,NULL,0,1024 }};

parity parityvals[] = { { "none", 0 },\
			{ "odd", 1 },\
			{ "even", 2 },\
			{ "mark", 3 },\
			{ "space", 4 },\
			{ "",-1 } };

uint8_t comport(unsigned int op,uint16_t port,char *buf,size_t size) {
unsigned int count;
char *b;
unsigned int comportrcount;
unsigned int whichport;
char c;

if(op == _WRITE) {				/* write com port */
 while((inb(port+5) & 0x20) == 0) ;;		/* not ready */

 b=buf;

 kprintf("com1=%X\n",port);
 kprintf("buf=%X\n",buf);

 for(count=0;count<size;count++) {		/* for each */
  c=*b++;
  kprintf("%c",c);

  outb((uint16_t) port,c);
 }

 kprintf("com done\n");
 return(size); 
}

for(whichport=0;whichport<5;whichport++) {
 if(ports[whichport].port == port) break;		/* found */
}

/*  the data arriving at the COM port triggers an irq interrupt which reads the data to a buffer and increments comportrcount */

 if((inb(port+5) & 0x20) != 0) {		/* not ready */
  setlasterror(DEVICE_IO_ERROR);
  return(-1);
 }

 count=0;

 while(count < size) {			/* wait until all data received */
 
/* read data in PAGE_SIZE chunks */

  while(ports[whichport].portrcount < ports[2].buffersize || ports[whichport].portrcount < size) ;;	/* wait for buffer to fill up */
  
  memcpy(buf,b,ports[whichport].portrcount);				
  buf=buf+ports[whichport].portrcount;					/* next */
 }

 ports[whichport].portrcount=0; 
 return(size);   
}


void com1(unsigned int op,char *buf,size_t size) {
 comport(op,COM1,buf,size);
}

void com2(unsigned int op,char *buf,size_t size) {
 comport(op,COM2,buf,size);
}

void com3(unsigned int op,char *buf,size_t size) {
 comport(op,COM3,buf,size);
}

void com4(unsigned int op,char *buf,size_t size) {
 comport(op,COM4,buf,size);
}

void com1_irq_handler(void *regs) {
 if(ports[0].bufptr == ports[0].buffer+ports[0].buffersize) ports[0].bufptr=ports[0].buffer;

 *ports[0].bufptr++=inb(COM1);
 ports[0].portrcount++;
 return;
}

void com2_irq_handler(void *regs) {
 if(ports[1].bufptr == ports[1].buffer+ports[1].buffersize) ports[1].bufptr=ports[1].buffer;

 *ports[1].bufptr++=inb(COM1);
 ports[1].portrcount++;
 return;
}

void com3_irq_handler(void *regs) {
 if(ports[2].bufptr == ports[2].buffer+ports[2].buffersize) ports[2].bufptr=ports[2].buffer;

 *ports[2].bufptr++=inb(COM1);
 ports[2].portrcount++;
 return;
}

void com4_irq_handler(void *regs) {
 if(ports[1].bufptr == ports[1].buffer+ports[3].buffersize) ports[1].bufptr=ports[1].buffer;

 *ports[1].bufptr++=inb(COM1);
 ports[1].portrcount++;
 return;
}


 /* intialize devices */


void serialio_init(char *init) {
CHARACTERDEVICE device;
char *tokens[10][255];
char *op[2][255];
int count;
int tc;
int subtc;
int whichp;
int countx;

ports[0].handler=&com1;		/* intialize function pointers */
ports[1].handler=&com2;
ports[2].handler=&com3;
ports[3].handler=&com4;

//port=com1 baud=115200 databits=8 stopbits=1 parity=even interrupts=data buffersize=1024

if(init != NULL) {			/* args found */
 tc=tokenize_line(init,tokens," ");	/* tokenize line */

 for(count=0;count<tc;count++) {
  tokenize_line(tokens[count],op,"=");	/* tokenize line */

  if(strcmp(op[0],"port") == 0) {		/* set port */
   for(whichp=0;whichp<5;whichp++) {
    if(strcmp(ports[whichp],op[1]) == 0) break;
   }

   if(whichp == 5) {		/* bad serial */
    kprintf("serial: Unknown port %s\n",op[1]);
    return;
   }
  }

  if(strcmp(op[0],"buffersize") == 0) {
   ports[whichp].buffersize=atoi(op[1]);
   return;
  }

  if(strcmp(op[0],"baud") == 0) {		/* set baud */
   ports[whichp].baud=atoi(op[0]);
  }

  if(strcmp(op[0],"databits") == 0) {		/* set databits */
   ports[whichp].databits=atoi(op[1]);
  }


  if(strcmp(op[0],"stopbits") == 0) {		/* set stop bits */
   ports[whichp].stopbits=atoi(op[1]);
  }

  if(strcmp(tokens[count],"parity") == 0) {		/* set baud */
   countx=0;
   while(countx != -1) {
    if(strcmp(parityvals[countx].name,op[1]) == 0) ports[whichp].parity=parityvals[countx].val;
    countx++;
   }
  
   if(countx == -1) {		/* bad value */
    kprintf("serial: parity must be odd or even\n");
    return;
   }
  }
 }
}

setirqhandler(3,&com2_irq_handler);		/* set irq handler */
setirqhandler(4,&com1_irq_handler);		/* set irq handler */
setirqhandler(4,&com3_irq_handler);		/* set irq handler */
setirqhandler(3,&com4_irq_handler);		/* set irq handler */

/* configure ports */

for(count=0;count<4;count++) {
 ports[count].buffer=kernelalloc(ports[count].buffersize);		/* allocate buffer */
 if(ports[count].buffer == NULL) return(-1);

 ports[count].bufptr= ports[count].buffer;

 strcpy(&device.dname,ports[count].name);		/* add device */
 device.chario=ports[count].handler;
 device.flags=0;
 device.data=NULL;
 device.next=NULL;
 add_char_device(&device);			

 outb(ports[count].port+1, 0x00);    			/* Disable all interrupts */
 outb(ports[count].port+3, 0x80);    			/* Enable DLAB (set baud rate divisor) */
 outb(ports[count].port+0, (115200/ports[count].baud)); /* set divisor  */
 outb(ports[count].port+1, 0x00);    			/* hi byte */
 outb(ports[count].port+3, ports[count].parity);    	/* Parity  */
 outb(ports[count].port+2, 0xC7);    		/* Enable FIFO, clear them, with 14-byte threshold  */
 outb(ports[count].port+4, 0x0B);    				/* IRQs enabled, RTS/DSR set  */
}

return;
}	

size_t serial_ioctl(size_t handle,unsigned long request,char *buffer) {
unsigned int param;
FILERECORD serialdevice;
char *b;
int count;

gethandle(handle,&serialdevice);		/* get information about device */

/* find device name */

for(count=0;count<4;count++) {
 if(strcmp(ports[count].name,serialdevice.filename)) break;	/* found device */
}

if(count > 4) return(-1);			/* not a serial device */

b=buffer;

 switch(request) {

  case IOCTL_SERIAL_BAUD: 		/* set baud */
   ports[count].baud=*buffer;

   outb(ports[count].port+1, 0x00);    			/* Disable all interrupts */
   outb(ports[count].port+3, 0x80);    			/* Enable DLAB (set baud rate divisor) */
   outb(ports[count].port+0, (115200/ports[count].baud)); /* set divisor  */
   outb(ports[count].port+4, 0x0B);    				/* IRQs enabled, RTS/DSR set  */
   return;

  case IOCTL_SERIAL_DATABITS:
   ports[count].databits=*buffer;
   return;

  case IOCTL_SERIAL_STOPBITS:
   ports[count].stopbits=*buffer;
   return;

  case IOCTL_SERIAL_PARITY:
   ports[count].parity=*buffer;

   outb(ports[count].port+1, 0x00);    			/* Disable all interrupts */
   outb(ports[count].port+3, ports[count].parity);    	/* Parity  */
   outb(ports[count].port+4, 0x0B);    				/* IRQs enabled, RTS/DSR set  */
   return;

  default:
    return(-1);
 }
}


