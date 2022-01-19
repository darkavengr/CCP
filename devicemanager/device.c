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

/*
 * device manager
 */

#include <stdint.h>
#include "device.h"
#include "../filemanager/vfs.h"
#include "../header/errors.h"
#include <elf.h>
#include "../processmanager/mutex.h"

size_t add_block_device(BLOCKDEVICE *driver);
size_t add_char_device(CHARACTERDEVICE *device);
size_t blockio(unsigned int op,size_t drive,size_t block,void *buf);
size_t load_kernel_module(char *filename,char *argsx);
size_t getnameofsymbol(Elf32_Shdr *strtab,size_t which,char *name);
size_t findcharacterdevice(char *name,CHARACTERDEVICE *buf);
size_t getblockdevice(size_t drive,BLOCKDEVICE *buf);
size_t getdevicebyname(char *name,BLOCKDEVICE *buf);
size_t update_block_device(size_t drive,BLOCKDEVICE *driver);
size_t remove_block_device(char *name);
size_t remove_char_device(char *name);
size_t allocatedrive(void);
void devicemanager_init(void);

BLOCKDEVICE *blockdevices=NULL;
size_t lastdrive=2;
CHARACTERDEVICE *characterdevs=NULL;
MUTEX blockdevice_mutex;
MUTEX characterdevice_mutex;

#define DO_386_32(S, A)	((S) + (A))
#define DO_386_PC32(S, A, P)	((S) + (A) - (P))
 
kernelsymbol *kernelsyms=NULL;

size_t add_block_device(BLOCKDEVICE *driver) {
 BLOCKDEVICE *next;
 BLOCKDEVICE *last;

 lock_mutex(&blockdevice_mutex);			/* lock mutex */

/* find device */
 next=blockdevices;
 
 while(next != NULL) {
  last=next;
  next=next->next;
 }

if(blockdevices == NULL) {				/* if empty allocate struct */
 blockdevices=kernelalloc(sizeof(BLOCKDEVICE));
 if(blockdevices == NULL) return(-1);

 memset(blockdevices,0,sizeof(BLOCKDEVICE));
 last=blockdevices;
}
else
{
 last->next=kernelalloc(sizeof(blockdevices)); /* add to struct */
 last=last->next;
 memset(last,0,sizeof(BLOCKDEVICE));
}

 /* at end here */
 memcpy(last,driver,sizeof(BLOCKDEVICE));
 last->next=NULL;
 last->superblock=NULL;

 unlock_mutex(&blockdevice_mutex);		/* unlock mutex */

 setlasterror(NO_ERROR);
 return(NO_ERROR);
}


size_t add_char_device(CHARACTERDEVICE *device) {
CHARACTERDEVICE *charnext;
CHARACTERDEVICE *charlast;

charnext=characterdevs;

/* find device */

if(characterdevs == NULL) {				/* if empty allocate struct */
 characterdevs=kernelalloc(sizeof(CHARACTERDEVICE));
 if(characterdevs == NULL) return(-1);

 memset(characterdevs,0,sizeof(CHARACTERDEVICE));
 charnext=characterdevs;
}
else
{
 charnext=characterdevs;

 do {
  if(strcmpi(device->dname,charnext->dname) == 0) {		 		/* already loaded */
   setlasterror(DRIVER_ALREADY_LOADED);
   return(-1);
  }

  charlast=charnext;
  charnext=charnext->next;
 } while(charnext != NULL);

 charlast->next=kernelalloc(sizeof(characterdevs)); /* add to struct */

 charnext=charlast->next;
 memset(charnext,0,sizeof(CHARACTERDEVICE));
 
}

 /* at end here */
 memcpy(charnext,device,sizeof(CHARACTERDEVICE));
 charnext->next=NULL;
 setlasterror(NO_ERROR);
 return(NO_ERROR);

}

/*
 * block i/o handler
 *
 * calls a hardware-specific routine
 *
 */


size_t blockio(unsigned int op,size_t drive,size_t block,void *buf) {
 BLOCKDEVICE *next;
 char *b;
 unsigned int count;
 unsigned int error;
 unsigned int lasterr;

 lock_mutex(&blockdevice_mutex);			/* lock mutex */

 next=blockdevices;					/* find drive entry */
 
 while(next != NULL) {
  if(next->drive == drive) break;
  next=next->next;
 }

 if(next == NULL) {					/* bad drive */
  unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

  setlasterror(INVALID_DRIVE);
  return(-1);
 }

 while((next->flags & DEVICE_LOCKED) == DEVICE_LOCKED) ;;	/* wait for drive to become free */

 next->flags |= DEVICE_LOCKED;				/* lock drive */
 

b=buf;

if(next->sectorsperblock == 0) next->sectorsperblock=1;

for(count=0;count<next->sectorsperblock;count++) {

 if(next->blockio(op,next->physicaldrive,(next->startblock+block)+count,b) == -1) {
  lasterr=getlasterror();

  if(lasterr == WRITE_PROTECT_ERROR || lasterr == DRIVE_NOT_READY || lasterr == BAD_CRC \
		|| lasterr == GENERAL_FAILURE || lasterr == DEVICE_IO_ERROR) { 
 
   error=call_critical_error_handler(next->dname,drive,(op & 0x80000000),lasterr);	/* call exception handler */

   if(error == CRITICAL_ERROR_ABORT) {
    next->flags ^= DEVICE_LOCKED;				/* unlock drive */

    unlock_mutex(&blockdevice_mutex);			/* unlock mutex */
    exit(0);	/* abort */
   }

   if(error == CRITICAL_ERROR_RETRY) {		/* RETRY */
    count--;
    continue;
   }

   if(error == CRITICAL_ERROR_FAIL) {
    next->flags ^= DEVICE_LOCKED;				/* unlock drive */

    unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

    return(-1);	/* FAIL */
   }
  }
 }

 b=b+next->sectorsize;
}

 next->flags ^= DEVICE_LOCKED;				/* unlock drive */

 unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

 setlasterror(NO_ERROR);
 return(NO_ERROR);
}	

size_t load_kernel_module(char *filename,char *argsx) {
 size_t handle;
 char *fullname[MAX_PATH];
 char *name[MAX_PATH];
 char c;
 size_t count;
 void *buf;
 void *bufptr;
 Elf32_Ehdr *elf_header;
 Elf32_Shdr *shptr;
 Elf32_Rel *relbuf;
 Elf32_Rela *relbufa;
 size_t whichsym;
 size_t symtype;
 Elf32_Sym *symtab;
 Elf32_Sym *symptr;
 char *strtab;
 char *entryptr;
 size_t *addr;
 size_t *ref;
 void *(*entry)(char *);
 unsigned int numberofrelocentries;
 unsigned int symval;
 uint32_t codestart;

disablemultitasking();

enable_interrupts();

getfullpath(filename,fullname);

handle=open(fullname,_O_RDONLY);		/* open file */
if(handle == -1) return(-1);		/* can't open */

buf=kernelalloc(getfilesize(handle));
if(buf == -1) return(-1);

if(read(handle,buf,getfilesize(handle)) == -1) return(-1); /* read error */

elf_header=buf;

if(elf_header->e_ident[0] != 0x7F && elf_header->e_ident[1] != 0x45 && elf_header->e_ident[2] != 0x4C && elf_header->e_ident[3] != 0x46) {	/* not elf */
 setlasterror(BAD_DRIVER);
 kernelfree(buf);
 return(-1);
}

if(elf_header->e_type != ET_REL) {
 setlasterror(BAD_DRIVER);
 kernelfree(buf);
 return(-1);
}

if(elf_header->e_shnum == 0) {	
 setlasterror(BAD_DRIVER);
 kernelfree(buf);
 return(-1);
}

 shptr=buf+elf_header->e_shoff;

for(count=0;count < elf_header->e_shnum;count++) {
 if(shptr->sh_type == SHT_SYMTAB && symtab != NULL) symtab=buf+shptr->sh_offset;
 if(shptr->sh_type == SHT_STRTAB && symtab != NULL) strtab=buf+shptr->sh_offset;

 shptr++;
}

/* check if symbol and string table present */

if(symtab == NULL) {
 setlasterror(BAD_DRIVER);
 kernelfree(buf);
 return(-1); 
}

if(shptr == NULL) {
setlasterror(BAD_DRIVER);
 kernelfree(buf);
 return(-1); 
}

shptr=buf+elf_header->e_shoff+(elf_header->e_shstrndx*elf_header->e_shentsize);	/* point to string table */
entryptr=(buf+shptr->sh_offset)+1;

/* find the start of the code */

count=1;

while(*entryptr != 0) {
 if(strcmpi(entryptr,".text") == 0) {	/* found name of section */
   shptr=buf+(elf_header->e_shoff+(count*elf_header->e_shentsize));	/* point to section header */
   
   codestart=buf+shptr->sh_offset;	/* point to address */
   break;
  }

  symptr=symptr+(strlen(symptr))+1;
  count++;
}

 if(count == elf_header->e_shnum) {
  kprintf("Missing symbol .text in %s\n",filename);

  setlasterror(BAD_DRIVER);

  kernelfree(buf);
  return(-1);
 }

shptr=buf+elf_header->e_shoff;

/* relocate elf references */

for(count=0;count < elf_header->e_shnum;count++) {

 if(shptr->sh_type == SHT_REL) {
//   kprintf("Found SHT_REL section\n");
 
   numberofrelocentries=shptr->sh_size/sizeof(Elf32_Rel);
   relbuf=buf+shptr->sh_offset;

//   kprintf("Number of reloc entries=%d\n",numberofrelocentries);
//   kprintf("relbuf=%X\n",relbuf);

   while(numberofrelocentries-- > 0) {
		/* 	Get the location where the symbol value is to be placed  */

	     whichsym=relbuf->r_info >> 8;			/* which symbol */
             symtype=relbuf->r_info & 0xff;		/* symbol type */

	     ref=codestart+relbuf->r_offset;	/* location of symbol */


	      /*	Get the value to place at the location ref into symval */

             if((symptr->st_info >> 4) == STB_GLOBAL) {		/* it's an extern */
                      getnameofsymbol(strtab,whichsym,name);	/* get name of symbol */

		      symval=getkernelsymbol(name);
	              if(symval == -1) {
		       kprintf("Unknown external symbol %s in %s\n",filename,name);

                       setlasterror(BAD_DRIVER);
		       kernelfree(buf);
		       return(-1); 
		      }

            }
	    else 
            {   
                          symval=symptr->st_value;
	    }

//	     kprintf("ref=%X\n",ref);
//     	     kprintf("symval=%X\n\n",symval);
 

/* update reference in section */

	     switch(symtype) {
	      case R_386_NONE:
	       break;
		
	      case R_386_32:
	       *ref=DO_386_32(codestart,*ref);
	       break;

	      case R_386_PC32:
	       *ref=DO_386_PC32(codestart,*ref,codestart);
	       break;

     	      default:
	       kprintf("kernel: unknown relocation type %d in %s\n",symtype,filename);

	       enablemultitasking();
	       setlasterror(BAD_DRIVER);
	       return(-1);
             }
       }

        	relbuf++;
   }


 if(shptr ->sh_type == SHT_RELA) {
  
    numberofrelocentries=shptr->sh_size/sizeof(Elf32_Rela);
    relbufa=buf+shptr->sh_offset;

   while(numberofrelocentries-- > 0) {
	     whichsym=relbufa->r_info >> 8;			/* which symbol */
             symtype=relbufa->r_info & 0xff;		/* symbol type */

	     addr=buf+shptr->sh_info;		/* find location to do relocation */
	     ref=codestart+relbufa->r_offset;	/* location of symbol */

	      /*	Get the value to place at the location ref into symval */

             symptr=symtab+(whichsym*sizeof(Elf32_Sym));
	
             if((symptr->st_info >> 4) == STB_GLOBAL) {		/* it's an extern */
                      getnameofsymbol(strtab,whichsym,name);	/* get name of symbol */

		      symval=getkernelsymbol(name);
	              if(symval == -1) {
		       kprintf("Unknown external symbol %s in %s\n",filename,name);

		       enablemultitasking();
                       setlasterror(BAD_DRIVER);
		       kernelfree(buf);
		       return(-1); 
		      }
            }
	    else 
            {   
	       symval=symptr->st_value;
	    }

             symval += relbufa->r_addend;
/* update reference in section */

	     switch(symtype) {
	      case R_386_NONE:
	       break;
		
	      case R_386_32:
	       *ref=DO_386_32(symval,*ref);
	       break;


	      case R_386_PC32:
		*ref = DO_386_PC32(symval,*ref,symval);
		break;


     	      default:
	       enablemultitasking();
	       setlasterror(BAD_DRIVER);
	       return(-1);
             }

	       relbufa++;	
       }
   }
      	
	
shptr++;				
}

close(handle);

entry=codestart;
entry(argsx);
return;
}

size_t getnameofsymbol(Elf32_Shdr *strtab,size_t which,char *name) {
 size_t count=0;
 Elf32_Shdr *strptr=strtab;

 for(count=0;count<which;count++) {
  strptr += strlen(strptr);
 }
  
 strcpy(name,strptr);
}

int getkernelsymbol(char *name) {
kernelsymbol *symptr;

if(kernelsyms == NULL) return(-1);		/* no symbols */

symptr=kernelsyms;

while(symptr != NULL) {			/* search symbols for name */
 if(strcmpi(symptr->name,name) == 0) return(symptr->val);

 symptr++;
 }

 return(-1);
}

size_t findcharacterdevice(char *name,CHARACTERDEVICE *buf) {
CHARACTERDEVICE *next;

lock_mutex(&characterdevice_mutex);			/* lock mutex */

next=characterdevs;

while(next != NULL) {
 if(strcmpi(next->dname,name) == 0) {		/* found */

  memcpy(buf,next,sizeof(CHARACTERDEVICE));

  unlock_mutex(&characterdevice_mutex);			/* unlock mutex */

  setlasterror(NO_ERROR);
  return(NO_ERROR);
 }

 next=next->next;
}

setlasterror(FILE_NOT_FOUND);
return(-1);
}

size_t getblockdevice(size_t drive,BLOCKDEVICE *buf) {
BLOCKDEVICE *next;

lock_mutex(&blockdevice_mutex);			/* lock mutex */

next=blockdevices;
while(next != NULL) {
 if(next->drive == drive) {		/* found device */

  memcpy(buf,next,sizeof(BLOCKDEVICE));

  unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

  setlasterror(NO_ERROR);
  return(NO_ERROR);
 }

 next=next->next;
}

unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

setlasterror(INVALID_DRIVE);
return(-1);
}

size_t getdevicebyname(char *name,BLOCKDEVICE *buf) {
BLOCKDEVICE *next;
BLOCKDEVICE *savenext;

SPLITBUF splitbuf;

lock_mutex(&blockdevice_mutex);			/* lock mutex */

next=blockdevices;
while(next != NULL) {
 if(strcmpi(next->dname,name) == 0) {		/* found device */
  savenext=next->next;

  memcpy(buf,next,sizeof(BLOCKDEVICE));
  next->next=savenext;

  unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

  setlasterror(NO_ERROR);
  return(NO_ERROR);
 }

 next=next->next;
}

setlasterror(INVALID_DRIVE);
return(-1);
}

size_t update_block_device(size_t drive,BLOCKDEVICE *driver) {
 BLOCKDEVICE *next;
 BLOCKDEVICE *last;

/* find device */
 lock_mutex(&blockdevice_mutex);			/* lock mutex */

 next=blockdevices;

 while(next != NULL) {
  if(next->drive == drive) {		 		/* already loaded */
   last=next->next;

   memcpy(next,driver,sizeof(BLOCKDEVICE));		/* update struct */

   next->next=last;

   unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

   setlasterror(NO_ERROR);
   return(0);
  }

 next=next->next;
 }
	
  setlasterror(INVALID_DRIVE);
  return(-1);
}

size_t remove_block_device(char *name) {
BLOCKDEVICE *next;
BLOCKDEVICE *last;

/* find device */
 lock_mutex(&blockdevice_mutex);			/* lock mutex */

 next=blockdevices;
 last=blockdevices;

 while(next != NULL) {
  if(strcmpi(next->dname,name) == 0) {		 		/* already loaded */

   if(last == blockdevices) {			/* start */
    blockdevices=next->next;
   }
   else if(next->next == NULL) {		/* end */
	last->next=NULL;
   }
   else						/* middle */
   {
    last->next=next->next;	
   }
   
   kernelfree(next);

   unlock_mutex(&blockdevice_mutex);			/* unlock mutex */

   setlasterror(NO_ERROR);
   return(0);
  }

 last=next;
 next=next->next;
 }

 setlasterror(INVALID_DRIVE);
 return(-1);
}


size_t remove_char_device(char *name) {
CHARACTERDEVICE *next;
CHARACTERDEVICE *last;

/* find device */

 lock_mutex(&characterdevice_mutex);			/* lock mutex */

 next=characterdevs;
 last=next;

 while(next != NULL) {
  if(strcmpi(next->dname,name) == 0) {		 		/* already loaded */
   if(last == characterdevs) {			/* start */
    characterdevs=next->next;
   }
   else if(next->next == NULL) {		/* end */
	last->next=NULL;
   }
   else						/* middle */
   {
    last->next=next->next;	
   }

   kernelfree(next);

   unlock_mutex(&characterdevice_mutex);	/* unlock mutex */

   setlasterror(NO_ERROR);
   return(0);
  }

 last=next;
 next=next->next;
 }

 setlasterror(BAD_DEVICE);
 return(-1);
}

size_t allocatedrive(void) {
 return(lastdrive++);
}


void devicemanager_init(void) {
 blockdevices=NULL;
 characterdevs=NULL;

 initialize_mutex(&blockdevice_mutex);		/* intialize mutex */
 initialize_mutex(&characterdevice_mutex);	/* intialize mutex */
}

