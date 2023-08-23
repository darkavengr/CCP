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
#include "../../../header/errors.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "clock.h"

#define MODULE_INIT clock_init


size_t gettime(TIMEBUF *timebuf);
void settime(TIMEBUF *timebuf);
size_t delay_loop(size_t delaycount);
size_t clockio(size_t op,void *buf,size_t size);
void clock_init(char *init);

/*
 * Get time and date
 *
 * In:  timebuf 	struct to store time and date
 *
 *  Returns: nothing
 *
 */
size_t gettime(TIMEBUF *timebuf) {
 outb(0x70,4); 
 timebuf->hours=inb(0x71); 			/* read hour */
 outb(0x70,2); 
 timebuf->minutes=inb(0x71); 		/* read minutes */
 outb(0x70,1); 
 timebuf->seconds=inb(0x71); 		/* read seconds */
  
 outb(0x70,9); 
 timebuf->year=inb(0x71); 			/* read year */
 outb(0x70,8); 
 timebuf->month=inb(0x71); 			/* read month */
 outb(0x70,7); 
 timebuf->day=inb(0x71);			 /* read day */
}

/*
 * Set time and date
 *
 * In:  timebuf 	struct with time and date
 *
 *  Returns: nothing
 *
 */
void settime(TIMEBUF *timebuf) {
 outb(0x70,4); 
 outb(0x71,timebuf->hours);			 /* set hours */
 outb(0x70,2); 
 outb(0x71,timebuf->minutes);		/* set minutes */
 outb(0x70,0); 
 outb(0x71,timebuf->seconds);		/* set hours */
 
 outb(0x70,9); 
 outb(0x71,(timebuf->year & 0xff));	 	/* set year */
 outb(0x70,8); 
 outb(0x71,timebuf->month);	 		/* set month */
 outb(0x70,7); 
 outb(0x71,timebuf->day);	 		/* set day */
 return;
}

/*
 * Delay loop
 *
 * In:  delaycount	Number of seconds to wait
 *
 *  Returns: nothing
 *
 */
size_t delay_loop(size_t delaycount) {
 size_t oldcount;
 size_t count;

 outb(0x70,1);				/* get seconds */ 
 oldcount=inb(0x71);

 while(count < oldcount+delaycount) {		/* loop until end */
  outb(0x70,1); 
  count=inb(0x71);
 }
}

/*
 * Clock  I/O function
 *
 * In:  op	Operation (0=read,1=write)
        buf	Buffer
	len	Number of bytes to read/write
 *
 *  Returns: nothing
 *
 */
size_t clockio(size_t op,void *buf,size_t size) {
 TIMEBUF time;
 char *z[10];

 if(op == _READ) {		/* read time */
  gettime(&time);	

  memcpy(buf,&time,size); 

  setlasterror(NO_ERROR);
  return(NO_ERROR);
 }

 if(op == _WRITE) {		/* write block */
  memcpy(&time,buf,size);  
  settime(&time);
  setlasterror(NO_ERROR);
  return(NO_ERROR);
 }

 setlasterror(DEVICE_IO_ERROR);
 return(-1);
}

/*
 * Initialize RTC
 *
 * In:  char *init	Initialization string
 *
 * Returns: nothing
 *
 */

void clock_init(char *init) {
CHARACTERDEVICE device;

 strcpy(&device.dname,"CLOCK$");
 device.charioread=&clockio;
 device.ioctl=NULL;
 device.flags=0;
 device.data=NULL;
 device.next=NULL;

 if(add_char_device(&device) == -1) { /* can't intialize */
  kprintf_direct("kernel: can't intialize clock device\n");
  return(-1);
 }

 setlasterror(NO_ERROR);
 return(NO_ERROR);
}
