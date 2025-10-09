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
#include "mouse.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "string.h"

#define MODULE_INIT mouse_init

struct {
	size_t mousex;
	size_t mousey;
	size_t mousebuttons;
} mouseinfo;

size_t mreadcount;
uint8_t mousepacket[3];
size_t mouse_click_timestamp=0;

/*
 * Initialize mouse
 *
 * In:  char *init	Initialization string
 *
 * Returns: nothing
 *
 */

size_t mouse_init(char *init) {
CHARACTERDEVICE device;
uint8_t mousestatus;
char *tokens[10][MAX_PATH];
char *op[10][MAX_PATH];
uint32_t mr;
size_t tc;
size_t count;

mouseinfo.mousex=0;
mouseinfo.mousey=0;
mouseinfo.mousebuttons=0;
mreadcount=0;

setirqhandler(12,'MSE$',&mouse_handler);		/* set irq handler */

wait_for_mouse_write();
outb(MOUSE_STATUS,MOUSE_ENABLE_AUXILARY_DEVICE);	/* get status */

wait_for_mouse_write();
outb(MOUSE_STATUS,MOUSE_GET_COMPAQ_STATUS);	/* get status */

wait_for_mouse_read();
mousestatus=inb(MOUSE_PORT);

mousestatus |= MOUSE_ENABLE_IRQ12;

wait_for_mouse_write();
outb(MOUSE_STATUS,MOUSE_SET_COMPAQ_STATUS);			/* set status */

wait_for_mouse_write();
outb(MOUSE_PORT,mousestatus);

mouse_send_command(MOUSE_SET_DEFAULTS);		/* configure mouse */
readmouse();

mouse_send_command(MOUSE_ENABLE_PACKET_STREAMING);		/* configure mouse */
readmouse();

/* no ack */

strncpy(&device.dname,"MOUSE",MAX_PATH);			/* add character device */

device.charioread=&mouseio_read;
device.chariowrite=NULL;
device.flags=0;
device.data=NULL;
device.next=NULL;

if(add_character_device(&device) == -1) {	/* add character device */
	kprintf_direct("mouse: Can't register character device %s: %s\n",device.name,kstrerr(getlasterror()));
	return(-1);
}

mouse_click_timestamp=get_timer_count();		/* get first timestamp for doubleclick */

if(init != NULL) {			/* args found */
	tc=tokenize_line(init,tokens," ");	/* tokenize line */

	for(count=0;count<tc;count++) {
		tokenize_line(tokens[count],op,"=");	/* tokenize line */

	  	if(strncmp(op[0],"resolution",MAX_PATH) == 0) {		/* mouse resolution */
			mr=atoi(op[1]);

			if(mr < 3) {
				kprintf_direct("mouse: Invalid resolution. Must be 0,1,2 or 3\n");
				return;
			}
	
			mouse_set_resolution(mr);
		}

		if(strncmp(op[0],"samplerate",MAX_PATH) == 0) {
			mr=atoi(op[1]);	/* sample rate */
			mouse_set_sample_rate(mr);
		}
	
		if(strncmp(op[0],"enablescrollwheel",MAX_PATH) == 0) {
			mouse_enable_scrollwheel();
		}
	}
}

return;
}

/*
 * Mouse IRQ handler
 *
 * In:  nothing
 *
 * Returns: nothing
 *
 */

void mouse_handler(void) {
uint8_t mousestatus;
uint32_t oldpos;
uint8_t oldcolour;
uint8_t mx;
uint8_t my;

wait_for_mouse_read();
mousepacket[mreadcount]=inb(MOUSE_PORT);
mreadcount++;

if(mreadcount == 3 && (mousestatus & MOUSE_DATA_READY_READ) == 0) {
	 mreadcount=0;

	 mouseinfo.mousebuttons=mousepacket[0];
	 mx=mousepacket[1];
	 my=mousepacket[2];

	 if(mx <= 2) {			/* small movement */
	 	mx *= 1;
	 }
	 else
	 {
	 	mx *=10;
	 }

	 if((mouseinfo.mousebuttons & MOUSE_X_SIGN_BIT_MASK)) mx |= 0xffffff00;		/* wrap over */
	 if((mouseinfo.mousebuttons & MOUSE_Y_SIGN_BIT_MASK)) my |= 0xffffff00;

	 mouseinfo.mousex += mx;
	 mouseinfo.mousey += my;

	 if(mousepacket[0] & MOUSE_LEFT_BUTTON_MASK) {
	 	if(get_timer_count() < (mouse_click_timestamp+MOUSE_DOUBLECLICK_INTERVAL)) {		/* double click */    

		mouseinfo.mousebuttons |= MOUSE_LEFT_BUTTON_DOUBLECLICK;
	}
	else
	{
		mouse_click_timestamp=get_timer_count()+MOUSE_DOUBLECLICK_INTERVAL;
		mouseinfo.mousebuttons |= MOUSE_LEFT_BUTTON_MASK;
	}
}

if(mousepacket[0] & MOUSE_RIGHT_BUTTON_MASK) {
	mouseinfo.mousebuttons |= MOUSE_RIGHT_BUTTON_MASK;
}

if(mousepacket[0] & MOUSE_MIDDLE_BUTTON_MASK) {
	mouseinfo.mousebuttons |= MOUSE_MIDDLE_BUTTON_MASK;
}

}

return;
}

/*
 * Read mouse data
 *
 * In:  nothing
 *
 * Returns: mouse data byte
 *
 */

uint8_t readmouse(void) {
wait_for_mouse_read();		/*wait for ready to read */

return(inb(MOUSE_PORT));
}

/*
 * Wait for mouse to be ready to read data
 *
 * In:  nothing
 *
 * Returns: nothing
 *
 */

void wait_for_mouse_read(void) {
//kprintf_direct("mouse read\n");

while((inb(MOUSE_STATUS) & MOUSE_DATA_READY_READ) != 1) ;;

return;
}

/*
 * Wait for mouse to be ready to write data
 *
 * In:  nothing
 *
 * Returns: nothing
 *
 */
void wait_for_mouse_write(void) {
uint8_t readmouse=0;

//kprin|tf("mouse write\n");

do {
	readmouse=inb(MOUSE_STATUS);
	// kprintf_direct("mouse status=%X\n",readmouse);

} while((readmouse & MOUSE_DATA_READY_WRITE) != 0);

return;
}

/*
 * Mouse I/O function
 *
 * In:  op	Operation (0=read,1=write)
	       buf	Buffer
	len	Number of bytes to read/write
 *
 *  Returns: number of bytes read
 *
 */
size_t mouseio_read(void *buf,size_t size) {
char *b;
size_t count;

while(mreadcount < 3) ;;		/* wait for buffer to fill up */

memcpy(buf,&mouseinfo,sizeof(mouseinfo));
return(size);
}

/*
 * Wait for mouse to acknowledge command
 *
 * In: nothing
 *
 *  Returns: nothing
 *
 */
void wait_for_mouse_ack(void) {
while(inb(MOUSE_PORT) != 0xFA) ;;
}

/*
 * Send command to mouse
 *
 * In:  command	Command to send
 *
 *  Returns: nothing
 *
 */
void mouse_send_command(uint8_t command) {
wait_for_mouse_write();		/*wait for ready to write */
outb(MOUSE_STATUS,0xD4);
	
wait_for_mouse_write();		/*wait for ready to write */
outb(MOUSE_PORT,command);
}

/*
 * Set mouse move resolution
 *
 * In:  resolution	Resolution (2,4, etc)
 *
 *  Returns: data byte from mouse
 *
 */
size_t mouse_set_resolution(size_t resolution) {
mouse_send_command(MOUSE_SET_RESOLUTION);

wait_for_mouse_write();		/*wait for ready to write */
outb(MOUSE_PORT,resolution);

/* check if accepted */
wait_for_mouse_read();		/*wait for ready to write */

return(readmouse());
}

size_t mouse_set_sample_rate(size_t resolution) {
mouse_send_command(MOUSE_SET_SAMPLE_RATE);

wait_for_mouse_write();		/*wait for ready to write */
outb(MOUSE_PORT,resolution);

/* check if accepted */
wait_for_mouse_read();		/*wait for ready to write */
return(readmouse());
}

/*
 * Enable scroll wheel on mouse
 *
 * In:  nothing
 *
 *  Returns: data byte from mouse
 *
 */
size_t mouse_enable_scrollwheel(void) {
mouse_set_sample_rate(200);
mouse_set_sample_rate(100);
mouse_set_sample_rate(80);

/* check if accepted */
wait_for_mouse_read();		/*wait for ready to write */

return(readmouse());
}

/*
 * Enable extra mouse buttions
 *
 * In:  nothing
 *
 *  Returns: nothing
 *
 */
size_t mouse_enable_extra_buttons(void) {
mouse_set_sample_rate(200);
mouse_set_sample_rate(200);
mouse_set_sample_rate(80);

/* check if accepted */
wait_for_mouse_read();		/*wait for ready to write */

return(readmouse());
}

/*
 * Soundblaster 16 ioctl handler
 *
 * In:  handle	Handle created by open() to reference device
	       request Request number
	       buffer  Buffer
 *
 *  Returns: -1 on error, 0 on success
 *
 */
size_t mouse_ioctl(size_t handle,unsigned long request,char *buffer) {
uint32_t param;
uint32_t *bufptr;

bufptr=buffer;

switch(request) {			/* ioctl request */

	case MOUSE_IOCTL_SET_RESOLUTION:	/* set pointer resolution */
		param=*bufptr++;

		mouse_set_resolution(param);

		return;

	case MOUSE_IOCTL_SET_SAMPLE_RATE:	/* set sample rate */
		param=*bufptr++;

		mouse_set_sample_rate(param);

		return;

	case MOUSE_IOCTL_ENABLE_SCROLL_WHEEL: /* enable scroll wheel */
		mouse_enable_scrollwheel();

		return;

	case MOUSE_IOCTL_ENABLE_EXTRA_BUTTONS: /* enable extra buttons */
		mouse_enable_extra_buttons();
		return;

	default:
		return(-1);
	}
}

