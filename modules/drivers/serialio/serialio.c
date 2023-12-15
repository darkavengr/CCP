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
#include "serialio.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "../../../filemanager/vfs.h"

#define MODULE_INIT serialio_init

extern setirqhandler();

uint8_t comport(size_t op,uint16_t port,char *buf,size_t size);
void read_com1(char *buf,size_t size);
void read_com2(char *buf,size_t size);
void read_com3(char *buf,size_t size);
void read_com3(char *buf,size_t size);
void write_com1(char *buf,size_t size);
void write_com2(char *buf,size_t size);
void write_com3(char *buf,size_t size);
void write_com3(char *buf,size_t size);

void com1_irq_handler(void *regs);
void com2_irq_handler(void *regs);
void com3_irq_handler(void *regs);
void com4_irq_handler(void *regs);
void serialio_init(char *init);
size_t serial_ioctl(size_t handle,unsigned long request,char *buffer);

/* Serial and parallel ports default settings */

port ports[]={ { "com1",COM1,115200,8,0,0,0,NULL,NULL,NULL,NULL,0,1024 },\
	       { "com2",COM2,115200,8,0,0,0,NULL,NULL,NULL,NULL,0,1024 },\
	       { "com3",COM3,115200,8,0,0,0,NULL,NULL,NULL,NULL,0,1024 },\
	       { "com4",COM4,115200,8,0,0,0,NULL,NULL,NULL,NULL,0,1024 }};

/* parity values */

parity parityvals[] = { { "none", 0 },\
			{ "odd", 1 },\
			{ "even", 2 },\
			{ "mark", 3 },\
			{ "space", 4 },\
			{ "",-1 } };

/*
 * Initialize serial and parallel ports
 *
 * In: char *init	Initialization string
 *
 *  Returns nothing
 *
 */
void serialio_init(char *init) {
CHARACTERDEVICE device;
char *tokens[10][255];
char *op[2][255];
int count;
int tc;
int subtc;
int whichp;
int countx;

ports[0].readhandler=&read_com1;		/* intialize function pointers */
ports[1].readhandler=&read_com2;
ports[2].readhandler=&read_com3;
ports[3].readhandler=&read_com4;

ports[0].writehandler=&write_com1;		/* intialize function pointers */
ports[1].writehandler=&write_com2;
ports[2].writehandler=&write_com3;
ports[3].writehandler=&write_com4;

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
	   			kprintf_direct("serial: Unknown port %s\n",op[1]);
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
	   			kprintf_direct("serial: parity must be odd or even\n");
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

	strcpy(&device.name,ports[count].name);		/* add device */
	device.charioread=ports[count].readhandler;
	device.chariowrite=ports[count].writehandler;

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


/*
 * Serial/parallel port I/O common function
 *
 * In: size_t op	Operation (0=read, 1=write)
	      uint16_t port	Port number
	      char *buf	Buffer
	      size_t size	Number of bytes to read/write
 *
 *  * Returns number of bytes read/written on success, -1 on error
 *
 */
uint8_t comport(size_t op,uint16_t port,char *buf,size_t size) {
size_t count;
char *b;
size_t comportrcount;
size_t whichport;
char c;

if(op == _WRITE) {				/* write com port */
	while((inb(port+5) & 0x20) == 0) ;;		/* not ready */

	b=buf;

	for(count=0;count<size;count++) {		/* for each */
		c=*b++;

		outb((uint16_t) port,c);
	}

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

/*
 * COM1 I/O functions
 *
 * In: size_t op	Operation (0=read, 1=write)
	      char *buf	Buffer
	      size_t size	Number of bytes to read/write
 *
 *  * Returns number of bytes read/written on success, -1 on error
 *
 */
void read_com1(char *buf,size_t size) {
	return(comport(_READ,COM1,buf,size));
}

void write_com1(char *buf,size_t size) {
	return(comport(_WRITE,COM1,buf,size));
}
/*
 * COM2 I/O function
 *
 * In: size_t op	Operation (0=read, 1=write)
	      char *buf	Buffer
	      size_t size	Number of bytes to read/write
 *
 *  * Returns number of bytes read/written on success, -1 on error
 *
 */

void read_com2(char *buf,size_t size) {
	return(comport(_READ,COM2,buf,size));
}

void write_com2(char *buf,size_t size) {
	return(comport(_WRITE,COM2,buf,size));
}

/*
 * COM3 I/O function
 *
 * In:  op	Operation (0=read, 1=write)
	buf	Buffer
	size	Number of bytes to read/write
 *
 *  * Returns number of bytes read/written on success, -1 on error
 *
 */

void read_com3(char *buf,size_t size) {
	return(comport(_READ,COM3,buf,size));
}

void write_com3(char *buf,size_t size) {
	return(comport(_WRITE,COM3,buf,size));
}

/*
 * COM4 I/O function
 *
 * In: size_t op	Operation (0=read, 1=write)
	      char *buf	Buffer
	      size_t size	Number of bytes to read/write
 *
 *  * Returns number of bytes read/written on success, -1 on error
 *
 */

void read_com4(char *buf,size_t size) {
	return(comport(_READ,COM4,buf,size));
}

void write_com4(char *buf,size_t size) {
	return(comport(_WRITE,COM4,buf,size));
}

/*
 * COM1 irq handler
 *
 * In: void *regs 	Buffer to save CPU registers (not used)
 *
 *  Returns nothing
 *
 */
void com1_irq_handler(void *regs) {
if(ports[0].bufptr == ports[0].buffer+ports[0].buffersize) ports[0].bufptr=ports[0].buffer;


*ports[0].bufptr++=inb(COM1);
ports[0].portrcount++;
return;
}

/*
 * COM2 irq handler
 *
 * In: void *regs 	Buffer to save CPU registers (not used)
 *
 *  Returns nothing
 *
 */
void com2_irq_handler(void *regs) {
if(ports[1].bufptr == ports[1].buffer+ports[1].buffersize) ports[1].bufptr=ports[1].buffer;

*ports[1].bufptr++=inb(COM1);
ports[1].portrcount++;
return;
}

/*
 * COM3 irq handler
 *
 * In: void *regs 	Buffer to save CPU registers (not used)
 *
 *  Returns nothing
 *
 */
void com3_irq_handler(void *regs) {
if(ports[2].bufptr == ports[2].buffer+ports[2].buffersize) ports[2].bufptr=ports[2].buffer;

*ports[2].bufptr++=inb(COM1);

ports[2].portrcount++;
return;
}

/*
 * COM4 irq handler
 *
 * In: void *regs 	Buffer to save CPU registers (not used)
 *
 *  Returns nothing
 *
 */
void com4_irq_handler(void *regs) {
if(ports[1].bufptr == ports[1].buffer+ports[3].buffersize) ports[1].bufptr=ports[1].buffer;

*ports[1].bufptr++=inb(COM1);
ports[1].portrcount++;
return;
}

/*
 * ioctl handler for serial and parallel ports
 *
 * In: size_t handler		Handle used to for port (was opened with open());
	      unsigned long request	Request number
	      char *buffer		Buffer
 *
 *  Returns 0 on success, -1 on error
 *
 */
size_t serial_ioctl(size_t handle,unsigned long request,char *buffer) {
size_t param;
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
	 	return(0);

	 case IOCTL_SERIAL_DATABITS:
	 	ports[count].databits=*buffer;

	 	return(0);

	 case IOCTL_SERIAL_STOPBITS:
	 	ports[count].stopbits=*buffer;

	 	return(0);

	 case IOCTL_SERIAL_PARITY:
	 	ports[count].parity=*buffer;

	 	outb(ports[count].port+1, 0x00);    			/* Disable all interrupts */
	 	outb(ports[count].port+3, ports[count].parity);    	/* Parity  */
	 	outb(ports[count].port+4, 0x0B);    				/* IRQs enabled, RTS/DSR set  */

	 	return(0);

	 default:
	 	return(-1);
	 }
}


