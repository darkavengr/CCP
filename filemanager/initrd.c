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
#include "vfs.h"
#include "../devicemanager/device.h"
#include "initrd.h"
#include "../header/errors.h"
#include "../header/bootinfo.h"

size_t initrd_findfirst(char *filename,FILERECORD *buf);
size_t initrd_findnext(char *filename,FILERECORD *buf);
size_t initrd_find(unsigned int op,char *filename,FILERECORD *buf);
initrd_read(int ignore,size_t size,void *buf);
size_t initrd_init(void);
unsigned int initrd_io(unsigned int op,size_t drive,size_t block,char *buf);

TAR_HEADER *tarptr;

size_t initrd_findfirst(char *filename,FILERECORD *buf) {
 return(initrd_find(FALSE,filename,buf));
}

size_t initrd_findnext(char *filename,FILERECORD *buf) {
 return(initrd_find(TRUE,filename,buf));
}

size_t initrd_find(unsigned int op,char *filename,FILERECORD *buf) {
size_t size;

if(op == FALSE) tarptr=BOOT_INFO_INITRD_START;		/* first */

if(*tarptr->filename == 0) {			/* last file */
 setlasterror(FILE_NOT_FOUND);
 return(-1);
}

kprintf("%X\n",tarptr);
asm("xchg %bx,%bx");

memset(buf,0,sizeof(FILERECORD));

strcpy(buf->filename,tarptr->filename);	/* copy information */

atoi(tarptr->size,buf->filesize);
buf->drive='Z';

tarptr += (size+sizeof(TAR_HEADER));	/* point to next */
}

initrd_read(int ignore,size_t size,void *buf) {
void *dataptr;

size=atoi(tarptr->size,8);	/* get size */
dataptr=tarptr+sizeof(TAR_HEADER);	/* data follows header */

memcpy(buf,dataptr,size);

return(size);
}

size_t initrd_init(void) {
FILESYSTEM fs;
FILERECORD filerecord;
size_t retval;
BLOCKDEVICE bd;

/* create block device */
strcpy(bd.dname,"INITRD");
bd.blockio=&initrd_io;		/*setup struct */
bd.ioctl=NULL;
bd.physicaldrive=0;
bd.drive=0;
bd.flags=0;
bd.startblock=0;
bd.sectorsperblock=1;
addblockdevice(&bd);

/* load filesystem driver */
strcpy(fs.name,"INITRDFS");	/* name */
strcpy(fs.magicnumber,"Ustar\0");
fs.size=6;
fs.location=257;
fs.findfirst=&initrd_findfirst;
fs.findnext=&initrd_findnext;
fs.read=&initrd_read;
fs.write=NULL;			/* not used */
fs.rename=NULL;
fs.delete=NULL;
fs.mkdir=NULL;
fs.rmdir=NULL;
fs.create=NULL;
fs.chmod=NULL;
fs.setfiletd=NULL;
fs.write=NULL;
fs.write=NULL;
fs.getstartblock=NULL;
fs.getnextblock=NULL;
fs.seek=NULL;

register_filesystem(&fs);
}

unsigned int initrd_io(unsigned int op,size_t drive,size_t block,char *buf) { 
 size_t *size_ptr=BOOT_INFO_INITRD_START;

 if((block*512) > *size_ptr) {			/* out of range */
  set_last_error(BAD_BLOCK);
  return(-1);
}

 memcpy(buf,tarptr,(block*512));		/* copy data */
 
 set_last_error(NO_ERROR);
 return(0);
}
