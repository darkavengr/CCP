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
#include "kernelhigh.h"
#include "errors.h"	
#include "vfs.h"
#include "debug.h"
#include "memorymanager.h"
#include "string.h"
#include "signal.h"
#include "tty.h"
#include "ttycharacters.h"

size_t tty_mode;
TTY_HANDLER *tty_read_handlers=NULL;
TTY_HANDLER *tty_read_handlers_end=NULL;
TTY_HANDLER *tty_write_handlers=NULL;
TTY_HANDLER *tty_write_handlers_end=NULL;
TTY_HANDLER *tty_default_read_handler=NULL;
TTY_HANDLER *tty_default_write_handler=NULL;

/*
 * Initialize TTY
 *
 * In:  Nothing
 *
 * Returns: -1 on error, 0 on success
 *
 */

/* NOTE: This function MUST be called before opening any files.
	  stdin, stdout and stderror must file file handles 0,1 and 2 */

size_t tty_init(void) {
CHARACTERDEVICE device;
size_t handle;

strncpy(&device.name,TTY_DEVICE,MAX_PATH);
device.charioread=&tty_read;
device.chariowrite=&tty_write;
device.ioctl=&tty_ioctl;
device.flags=0;
device.data=NULL;
device.next=NULL;

if(add_character_device(&device) == -1) {	/* add character device */
	kprintf_direct("tty_init: Can't register character device %s: %s\n",device.name,kstrerr(getlasterror()));
	return(-1);
}

if(open(TTY_DEVICE,O_RDWR) == -1) return(-1);			/* open stdin */

dup(stdin);			/* duplicate to stdout */
dup(stdin);			/* duplicate to stderr */

tty_mode=TTY_MODE_ECHO;		/* set TTY flags */

return(0);
}

/*
 * Read TTY
 *
 * In:  data	Address to read to
	size	Number of bytes to read
 *
 *  Returns: -1 on error, 0 on success
 *
 */

size_t tty_read(char *data,size_t size) {
size_t bytesread;
size_t count;
char *dataptr=data;
char *controlout[3];
TTY_HANDLER *ttyptr;

/* get TTY handler */
if(tty_default_read_handler == NULL) {
	ttyptr=tty_read_handlers;
}
else
{
	ttyptr=tty_default_read_handler;
}

if(ttyptr == NULL) {		/* no handler */
	kprintf_direct("tty: No TTY read handler found\n");

	setlasterror(INVALID_PARAMETER);
	return(-1);
}

/* read from input device on character at a time to allow for special character processing */

for(count=0;count < size;count++) {
	bytesread=ttyptr->ttyread(dataptr,1);		/* read data */
	if(bytesread == -1) return(-1);

	/* handle special characters */

	if((tty_mode & TTY_MODE_RAW) == 0) {		/* process characters */

		if((char) *dataptr == TTY_CHAR_RETURN) return(count);		/* newline */

		if((char) *dataptr <= 31) {		/* display ^X control character */
			/* output ^X to console */

			if(tty_mode & TTY_MODE_ECHO) {
				ksnprintf(controlout,"^%c",3,(unsigned char) *dataptr+'@');
	
				if(tty_write(controlout,2) == -1) return(-1);	/* echo character */
			}

			if((char) *dataptr == TTY_CHAR_CANCEL) {		/* terminate process */
				sendsignal(getpid(),SIGINT);		/* send SIGINT signal */
			}
			else if((char) *dataptr == TTY_CHAR_INPUT_END) {		/* end of input */
				return(count);
			}
		}
	}
	
	if(tty_mode & TTY_MODE_ECHO) {
		if(tty_write(dataptr,1) == -1);		/* echo character */
	}

	dataptr++;
}

return(bytesread);
}

/*
 * Write TTY
 *
 * In:  data	Address to write from
	size	Number of bytes to write
 *
 *  Returns: -1 on error, 0 on success
 *
 */
size_t tty_write(char *data,size_t size) {
TTY_HANDLER *ttyptr;

/* get TTY handler */
if(tty_default_write_handler == NULL) {
	ttyptr=tty_write_handlers;
}
else
{
	ttyptr=tty_default_write_handler;
}

if(ttyptr == NULL) {		/* no handler */
	kprintf_direct("tty: No TTY write handler found\n");

	setlasterror(INVALID_PARAMETER);
	return(-1);
}

return(ttyptr->ttywrite(data,size));		/* write data */
}

/*
 * TTY ioctl handler
 *
 * In:  handle	Handle created by open() to reference device
	       request Request number
	       buffer  Buffer
 *
 *  Returns: -1 on error, 0 on success
 *
 */
size_t tty_ioctl(size_t handle,unsigned long request,char *buffer) {
	if(request == IOCTL_SET_TTY_MODE_RAW) {		/* set TTY raw mode */
		tty_mode |= TTY_MODE_RAW;

		setlasterror(NO_ERROR);
		return(0);
	}
	else if(request == IOCTL_CLEAR_TTY_MODE_RAW) {		/* clear TTY raw mode */
		tty_mode |= ~TTY_MODE_RAW;

		setlasterror(NO_ERROR);
		return(0);
	}
	else if(request == IOCTL_SET_TTY_MODE_ECHO) {		/* set TTY echo mode */
		tty_mode |= ~TTY_MODE_ECHO;

		setlasterror(NO_ERROR);
		return(0);
	}
	else if(request == IOCTL_CLEAR_TTY_MODE_ECHO) {	/* clear TTY echo mode */
		tty_mode |= ~TTY_MODE_ECHO;

		setlasterror(NO_ERROR);
		return(0);
	}

setlasterror(INVALID_PARAMETER);
return(-1);
}

/*
 * Register TTY handler internal function
 *
 * In:  handler	TTY handler data
 *
 *  Returns: -1 on error, 0 on success
 *
 */
size_t tty_register_read_handler(TTY_HANDLER *read_handler) {
/* Add entry to end */

if(tty_read_handlers == NULL) {			/* first read_handler */
	tty_read_handlers=kernelalloc(sizeof(TTY_HANDLER));
	if(tty_read_handlers == NULL) return(-1);

	tty_read_handlers_end=tty_read_handlers;
}
else
{
	tty_read_handlers_end->next=kernelalloc(sizeof(TTY_HANDLER));
	if(tty_read_handlers_end->next == NULL) return(-1);

	tty_read_handlers_end=tty_read_handlers_end->next;
}

memcpy(tty_read_handlers_end,read_handler,sizeof(TTY_HANDLER));	/* copy entry */

tty_read_handlers_end->next=NULL;
return(0);
}

size_t tty_register_write_handler(TTY_HANDLER *write_handler) {
/* Add entry to end */

if(tty_write_handlers == NULL) {			/* first write_handler */
	tty_write_handlers=kernelalloc(sizeof(TTY_HANDLER));
	if(tty_write_handlers == NULL) return(-1);

	tty_write_handlers_end=tty_write_handlers;
}
else
{
	tty_write_handlers_end->next=kernelalloc(sizeof(TTY_HANDLER));
	if(tty_write_handlers_end->next == NULL) return(-1);

	tty_write_handlers_end=tty_write_handlers_end->next;
}

memcpy(tty_write_handlers_end,write_handler,sizeof(TTY_HANDLER));	/* copy entry */

tty_write_handlers_end->next=NULL;
return(0);
}

void tty_set_default_read_handler(TTY_HANDLER *handler) {
tty_default_read_handler=handler;
}

void tty_set_default_write_handler(TTY_HANDLER *handler) {
tty_default_write_handler=handler;
}

