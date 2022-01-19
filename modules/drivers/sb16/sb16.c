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

#include "../../../devicemanager/device.h"
#include "sb16.h"

#define KERNEL_HIGH (1 << (sizeof(size_t)*8)-1)

#define MODULE_INIT sb16_init

void sb16_init(char *initstring);
unsigned int sb16_io(unsigned int op,char *buf,size_t len);
size_t sb16_ioctlsize_t handle,unsigned long request,char *buffer);
void sb_irq_handler(void);

uint8_t *sb_dma_buffer;
size_t sb_dma_buffer_size=SB_DMA_BUFFER_DEFAULT_SIZE;
unsigned int sb_irq_number=SB_DEFAULT_IRQ;
unsigned int sb_channel=SB_DEFAULT_CHANNEL;
unsigned int sb_sample_rate=SB_DEFAULT_SAMPLE_RATE;
unsigned int sb_time_constant=SB_DEFAULT_TIME_CONSTANT;
unsigned int sb_data_type=SB_DEFAULT_DATA_TYPE;
char *sb_bufptr;
size_t sb_transfer_length;

 struct {
  uint16_t address_register;
  uint16_t count_register;
  uint16_t page_register;
  uint8_t transfermode;
  uint16_t mask_register;
  uint16_t flipflop_register;
  uint16_t mode_register;
 } sb_ports[7] = { { 0,0,0,0,0,0,0 },\  
		   { 0x2,0x3,0x83,0xc0,0x0a,0x0c,0x0b },\
	           { 0x4,0x5,0x81,0xc0,0x0a,0x0c,0x0b },\
		   { 0x6,0x7,0x82,0xc0,0x0a,0x0c,0x0b }, \
		   { 0,0,0,0,0,0,0 },\
                   { 0xc4,0xc6,0x8b,0xb0,0xD4,0xD8,0xd6 },\
		   { 0xc8,0xca,0x89,0xb0,0xD4,0xD8,0xd6 },\
	           { 0xc4,0xc6,0x8b,0xb0,0xD4,0xD8,0xd6 } };

void sb16_init(char *init) {
CHARACTERDEVICE bd;
size_t tc;
char *tokens[10][255];
char *op[2][255];
int count;

if(init != NULL) {			/* args found */
 tc=tokenize_line(init,tokens," ");	/* tokenize line */

 for(count=0;count<tc;count++) {
  tokenize_line(tokens[count],op,"=");	/* tokenize line */
  if(strcmp(op[0],"buffersize") == 0) {		/* set buffer size */
   sb_dma_buffer_size=atoi(op[1]);  
  }

  if(strcmp(op[0],"irq") == 0) {		/* set irq */
   sb_irq_number=atoi(op[1]);

   if(sb_irq_number > 15) {
    kprintf("sb16: Invalid IRQ\n");
    return;
   }
  }

  if(strcmp(op[0],"samplerate") == 0) {		/* set sample rate */
   sb_sample_rate=atoi(op[1]);
  }

  if(strcmp(op[0],"channel") == 0) {		/* set channel */
   sb_channel=atoi(op[1]);

   if((sb_channel == 0) || (sb_channel > 3)) {
    kprintf("sb16: Invalid channel\n");
    return;
   }
  }

 if(strcmp(op[0],"volume") == 0) {		/* set volume */

   if(atoi(op[1]) > 0xff) {
    kprintf("sb16: Invalid volume value\n");
    return;

   outb(DSP_MIXER_PORT,SB_SET_VOLUME);
   outb(DSP_MIXER_DATA_PORT,(uint8_t) atoi(op[1]));
  }

 }
 }

}

sb_dma_buffer=dma_alloc(sb_dma_buffer_size);		/* allocate dma buffer */
if(sb_dma_buffer == -1) {					/* can't alloc */
 kprintf("sb16: can't allocate dma buffer\n");
 return(-1);
}

strcpy(bd.dname,"AUDIO");		/* add char device */
bd.chario=&sb16_io;
bd.flags=0;
bd.data=NULL;
bd.next=NULL;
add_char_device(&bd); 

setirqhandler(sb_irq_number,&sb_irq_handler);		/* set irq handler */

#ifdef SB16_DEBUG
 kprintf("sb irq number=%d\n",sb_irq_number);
#endif

outb(DSP_MIXER_PORT,0x80);		/* set irq */

switch(sb_irq_number) {
 case 2:
	outb(DSP_MIXER_DATA_PORT,1);
 	break;

 case 5:
	outb(DSP_MIXER_DATA_PORT,2);
 	break;

 case 7:
	outb(DSP_MIXER_DATA_PORT,4);
 	break;

 case 10:
	outb(DSP_MIXER_DATA_PORT,8);
 	break;

 }

return;
}

unsigned int sb16_io(unsigned int op,char *buf,size_t len) {
 char *b;

 outb(DSP_RESET,1);			/* reset sound card */
 kwait(6);
 outb(DSP_RESET,0);

 #ifdef SB16_DEBUG
  kprintf("len=%d\n",len);
  kprintf("reset=%X\n",inb(DSP_READ));
 #endif

 outb(DSP_WRITE,SB_SPEAKER_ON);		/* enable speaker */

 sb_bufptr=buf;
 sb_transfer_length=len;

 #ifdef SB16_DEBUG
  kprintf("%X %X\n",sb_dma_buffer,sb_dma_buffer+KERNEL_HIGH);
 #endif

 memcpy(sb_dma_buffer+KERNEL_HIGH,sb_bufptr,sb_dma_buffer_size);		/* copy first chunk of data to dma buffer */

 #ifdef SB16_DEBUG
  kprintf("mask register=%X\n",sb_ports[sb_channel].mask_register);
  kprintf("flipflop register=%X\n",sb_ports[sb_channel].flipflop_register);
  kprintf("mode register=%X\n",sb_ports[sb_channel].mode_register);
  kprintf("page register=%X\n",sb_ports[sb_channel].page_register);
  kprintf("address register=%X\n",sb_ports[sb_channel].address_register);
  kprintf("count register=%X\n",sb_ports[sb_channel].count_register);
 #endif

  /* configure dma */

  outb(sb_ports[sb_channel].mask_register,0x4+sb_channel);		/* select channel */
  outb(sb_ports[sb_channel].flipflop_register,1);				/* flip-flop */
  outb(sb_ports[sb_channel].mode_register,0x58+sb_channel);		/* auto mode */
  outb(sb_ports[sb_channel].page_register,(uint8_t) ((unsigned int) sb_dma_buffer >> 16));		/* byte #3 of 24-bit address */
  outb(sb_ports[sb_channel].address_register,(uint8_t) (((unsigned int) sb_dma_buffer & 0x0000ff)));	/* byte #1 of 24-bit address */
  outb(sb_ports[sb_channel].address_register,(uint8_t) (((unsigned int) sb_dma_buffer & 0x00ff00) >> 8)); /* byte #2 of 24-bit address */
  outb(sb_ports[sb_channel].count_register,(uint8_t) ((sb_dma_buffer_size-1) & 0xff));	/* size low byte */
  outb(sb_ports[sb_channel].count_register,(uint8_t) ((sb_dma_buffer_size-1) >> 8));	/* size low byte */
  outb(sb_ports[sb_channel].mask_register,sb_channel);		/* enable channel */

 outb(DSP_WRITE,SB_SET_TIME_CONSTANT);
 outb(DSP_WRITE,sb_time_constant);

 //outb(DSP_WRITE,SB_SET_SAMPLE_RATE);	/* set sample rate */
// outb(DSP_WRITE,sb_sample_rate);

 outb(DSP_WRITE,sb_ports[sb_channel].transfermode);

 outb(DSP_WRITE,sb_data_type);
 outb(DSP_WRITE,(uint8_t) ((sb_dma_buffer_size-1) & 0xff));	/* size low byte */
 outb(DSP_WRITE,(uint8_t) ((sb_dma_buffer_size-1) >> 8));	/* size low byte */
 
 while(sb_transfer_length > 0) ;;

 outb(DSP_WRITE,SB_SPEAKER_OFF);		/* disable speaker */
}


size_t sb16_ioctlsize_t handle,unsigned long request,char *buffer) {
unsigned int param;
char *b;
size_t count;

 switch(request) {
  case IOCTL_SB16_IRQ: 		/* set irq */
   param=*buffer;

   if(param > 15) return(-1);

   sb_irq_number=param;

   outb(DSP_MIXER_PORT,0x80);		/* set irq */
   outb(DSP_MIXER_DATA_PORT,sb_irq_number);

   setirqhandler(sb_irq_number,&sb_irq_handler);		/* set irq handler */
   break;

  case IOCTL_SB16_SET_SAMPLE_RATE: 	/* set sample rate */
   sb_sample_rate=*buffer;
   return;

  case IOCTL_SB16_SET_CHANNEL:	 	/* set channel */
   param=*buffer;

   if((param == 0) || (param > 3)) return(-1); /* invalid channel */
   return;

  case IOCTL_SB16_SET_VOLUME:	 	/* set volume */
   param=*buffer;

   if(param > 0xff) return(-1); /* invalid volume level */

   outb(DSP_MIXER_PORT,SB_SET_VOLUME);
   outb(DSP_MIXER_DATA_PORT,param);
   return;

  case IOCTL_SB16_SET_TIME_CONSTANT:	/* set time constant */
   param=*buffer;

   if(param > 0xff) return(-1); /* invalid time constant */

   sb_time_constant=param;

   outb(DSP_MIXER_PORT,SB_SET_TIME_CONSTANT);
   outb(DSP_MIXER_DATA_PORT,sb_time_constant);
   return;
   
  case IOCTL_SB16_SET_DATATYPE:	/* set data type */
   sb_data_type=*buffer;

    return;
 
  default:
    return(-1);
 }
}


void sb_irq_handler(void) {
 #ifdef SB16_DEBUG
  kprintf("sb16: IRQ %d received\n",sb_irq_number);
 #endif

  memcpy(sb_dma_buffer+KERNEL_HIGH,sb_bufptr,sb_dma_buffer_size);		/* copy next chunk of data to dma buffer */
  sb_bufptr += sb_dma_buffer_size;
  sb_transfer_length -= sb_dma_buffer_size;

 #ifdef SB16_DEBUG
  kprintf("Transfer length=%d\n",sb_transfer_length);
 #endif
}

