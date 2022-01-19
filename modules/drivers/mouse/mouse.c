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

#include "mouse.h"
#include "../../../devicemanager/device.h"

#define MODULE_INIT mouse_init

int mouse_init(char *init);
uint8_t readmouse(void);
void mouse_handler(void);
void wait_for_mouse_read(void);
void wait_for_mouse_write(void);
size_t mouseio(unsigned int op,void *buf,size_t size);
unsigned int wait_for_ack(void);
void mouse_send_command(uint8_t command);
void wait_for_mouse_ack(void);
unsigned int mouse_set_sample_rate(unsigned int resolution);
unsigned int mouse_enable_scrollwheel(void);
unsigned int mouse_set_resolution(unsigned int resolution);

struct {
 size_t mousex;
 size_t mousey;
 size_t mousebuttons;
} mouseinfo;

size_t mreadcount;
uint8_t mousepacket[3];
size_t mouse_click_timestamp=0;

int mouse_init(char *init) {
CHARACTERDEVICE device;
uint8_t mousestatus;
char *tokens[10][MAX_PATH];
char *op[10][MAX_PATH];
uint32_t mr;
int tc;
int count;

mouseinfo.mousex=0;
mouseinfo.mousey=0;
mouseinfo.mousebuttons=0;
mreadcount=0;

setirqhandler(12,&mouse_handler);		/* set irq handler */

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

strcpy(&device.dname,"MOUSE");			/* add character device */
device.chario=&mouseio;
device.flags=0;
device.data=NULL;
device.next=NULL;

add_char_device(&device);

mouse_click_timestamp=gettickcount();		/* get first timestamp for doubleclick */

if(init != NULL) {			/* args found */
 tc=tokenize_line(init,tokens," ");	/* tokenize line */

 for(count=0;count<tc;count++) {
 	 tokenize_line(tokens[count],op,"=");	/* tokenize line */

	  if(strcmp(op[0],"resolution") == 0) {		/* mouse resolution */
		mr=atoi(op[1]);

		if(mr < 3) {
		 kprintf("mouse: Invalid resolution. Must be 0,1,2 or 3\n");
		 return;
		}
	
		mouse_set_resolution(mr);
	 }

	 if(strcmp(op[0],"samplerate") == 0) {
		mr=atoi(op[1]);	/* sample rate */
		mouse_set_sample_rate(mr);
	 }
	
	 if(strcmp(op[0],"enablescrollwheel") == 0) {
  		mouse_enable_scrollwheel();
	 }
 }
}

return;
}

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

  if((mouseinfo.mousebuttons & MOUSE_X_SIGN_BIT_MASK)) mx |= 0xffffff00;
  if((mouseinfo.mousebuttons & MOUSE_Y_SIGN_BIT_MASK)) my |= 0xffffff00;

  mouseinfo.mousex += mx;
  mouseinfo.mousey += my;

  if(mousepacket[0] & MOUSE_LEFT_BUTTON_MASK) {
  //  kprintf("%X %X\n",mouse_click_timestamp,gettickcount());


    if(gettickcount() < (mouse_click_timestamp+MOUSE_DOUBLECLICK_INTERVAL)) {		/* double click */    

   //     kprintf("Mouse double clicked\n");
 	mouseinfo.mousebuttons |= MOUSE_LEFT_BUTTON_DOUBLECLICK;
    }
    else
    {
	mouse_click_timestamp=gettickcount()+MOUSE_DOUBLECLICK_INTERVAL;

//        kprintf("Mouse left clicked\n");
        mouseinfo.mousebuttons |= MOUSE_LEFT_BUTTON_MASK;
    }
   }

  if(mousepacket[0] & MOUSE_RIGHT_BUTTON_MASK) {
//        kprintf("Mouse right clicked\n");

	mouseinfo.mousebuttons |= MOUSE_RIGHT_BUTTON_MASK;
  }

  if(mousepacket[0] & MOUSE_MIDDLE_BUTTON_MASK) {
        kprintf("Mouse left clicked\n");
	mouseinfo.mousebuttons |= MOUSE_MIDDLE_BUTTON_MASK;
 }

  kprintf("x=%X y=%X\n",mouseinfo.mousex,mouseinfo.mousey);
 }

 return;
}

uint8_t readmouse(void) {
 wait_for_mouse_read();		/*wait for ready to read */

 return(inb(MOUSE_PORT));
}

void wait_for_mouse_read(void) {
//kprintf("mouse read\n");

while((inb(MOUSE_STATUS) & MOUSE_DATA_READY_READ) != 1) ;;

return;
}

void wait_for_mouse_write(void) {
uint8_t readmouse=0;

//kprin|tf("mouse write\n");

do {

 readmouse=inb(MOUSE_STATUS);
// kprintf("mouse status=%X\n",readmouse);

} while((readmouse & MOUSE_DATA_READY_WRITE) != 0);

return;
}

size_t mouseio(unsigned int op,void *buf,size_t size) {
char *b;
size_t count;

if(op == _MOUSE_READ) {
 while(mreadcount < 3) ;;		/* wait for buffer to fill up */

 memcpy(buf,&mouseinfo,sizeof(mouseinfo));
 return(size);
}

}

void wait_for_mouse_ack(void) {
 while(inb(MOUSE_PORT) != 0xFA) ;;
}

void mouse_send_command(uint8_t command) {
 wait_for_mouse_write();		/*wait for ready to write */
 outb(MOUSE_STATUS,0xD4);
 
 wait_for_mouse_write();		/*wait for ready to write */
 outb(MOUSE_PORT,command);
}

unsigned int mouse_set_resolution(unsigned int resolution) {
 mouse_send_command(MOUSE_SET_RESOLUTION);

 wait_for_mouse_write();		/*wait for ready to write */
 outb(MOUSE_PORT,resolution);

/* check if accepted */
 wait_for_mouse_read();		/*wait for ready to write */
 return(readmouse());
}

unsigned int mouse_set_sample_rate(unsigned int resolution) {
 mouse_send_command(MOUSE_SET_SAMPLE_RATE);

 wait_for_mouse_write();		/*wait for ready to write */
 outb(MOUSE_PORT,resolution);

/* check if accepted */
 wait_for_mouse_read();		/*wait for ready to write */
 return(readmouse());
}

unsigned int mouse_enable_scrollwheel(void) {
 mouse_set_sample_rate(200);
 mouse_set_sample_rate(100);
 mouse_set_sample_rate(80);

/* check if accepted */
 wait_for_mouse_read();		/*wait for ready to write */
 return(readmouse());
}

unsigned int mouse_enable_extra_buttons(void) {
 mouse_set_sample_rate(200);
 mouse_set_sample_rate(200);
 mouse_set_sample_rate(80);

/* check if accepted */
 wait_for_mouse_read();		/*wait for ready to write */
 return(readmouse());
}

unsigned int mouse_ioctl(size_t handle,unsigned long request,char *buffer) {
unsigned int param;
char *b;

b=buffer;

 switch(request) {			/* ioctl request */

  case MOUSE_IOCTL_SET_RESOLUTION:	/* set pointer resolution */
    param=*b++;

    mouse_set_resolution(param);
    return;

  case MOUSE_IOCTL_SET_SAMPLE_RATE:	/* set sample rate */
    param=*b++;

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



