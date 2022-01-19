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

/* memory manager */
 
#define ALLOC_NORMAL	0
#define ALLOC_KERNEL	1
#define ALLOC_NOPAGING 	2
#define ALLOC_GLOBAL	4

#include <stdint.h>
#include <stddef.h>

#include "../header/errors.h"
#include "../memorymanager/memorymanager.h"
#include "../processmanager/mutex.h"

#define KERNEL_HIGH (1 << (sizeof(unsigned int)*8)-1)

extern end(void);
extern kernel_begin(void);
extern MEM_SIZE;
extern PAGE_SIZE;

extern MEMBUF_START;

void *alloc_int(unsigned int flags,size_t process,size_t size,size_t overrideaddress);
unsigned int free_internal(size_t process,void *b);
void *alloc(size_t size);
void *kernelalloc(size_t size);				/* allocate kernel memory (0 - 0x80000000) */
void *kernelalloc_nopaging(size_t size);
void *dma_alloc(size_t size);

void *dmabuf=NULL;
void *dmaptr=NULL;
size_t dmabufsize=0;
MUTEX *memmanager_mutex;

void *alloc_int(unsigned int flags,size_t process,size_t size,size_t overrideaddress) {
size_t count;
size_t r;
size_t *last=(size_t *) -1;
size_t *mb=(size_t *) -1;
size_t pcount;
size_t startpage;
size_t firstpage;
size_t physpage;
size_t *m;
size_t countx;

if(size >= MEM_SIZE) {							/* sanity check */
 setlasterror(NO_MEM);
 return(NULL);
}

if(size < PAGE_SIZE) size=PAGE_SIZE;

if(size % PAGE_SIZE != 0) {
 size += PAGE_SIZE;				/* round up */
 size -= (size % PAGE_SIZE);
}

lock_mutex(&memmanager_mutex);

m=(size_t *) MEMBUF_START;

pcount=0;
mb=m; 
startpage=0;

/* check if enough free memory */

for(count=0;count<(MEM_SIZE/PAGE_SIZE)+1;count++) {
 if(*mb != 0) {
  startpage++;
  pcount++;
 }
 else
 {
  if(pcount++ >= (size / PAGE_SIZE)) break;			/* found enough */
 }

  mb++;
 }

 if(pcount < (size/PAGE_SIZE)) {					/* out of memory */
  unlock_mutex(&memmanager_mutex);

  setlasterror(NO_MEM);
  return(NULL);
 }

 last=(size_t *) MEMBUF_START;

 for(count=0;count<(MEM_SIZE/PAGE_SIZE)+1;count++) {
  if(*last == 0) break;
  last++;
 }

 if(flags != ALLOC_NOPAGING) {						/* find first page */

  if(overrideaddress == -1) { 
   firstpage=findfreevirtualpage(size,flags,process); 
   if(firstpage == -1) {

    unlock_mutex(&memmanager_mutex);
    setlasterror(NO_MEM);
    return(NULL);
   }
  
   startpage=firstpage;
  }
  else
  {

   startpage=overrideaddress;
   firstpage=overrideaddress;
  }

}
else
{
 firstpage=(startpage*PAGE_SIZE);
}

/* create new chain in buffer */

 physpage=0;
 mb=(size_t *) MEMBUF_START;
 pcount=0;

 for(count=0;count != (MEM_SIZE/PAGE_SIZE)+1;count++) {

  if(*mb == 0) {
      if(pcount == (size/PAGE_SIZE)+1) break;			/* found enough */
      pcount++;

      *last=(unsigned int *) mb;
      last=mb;
          
      if(flags != ALLOC_NOPAGING) {	

       switch(flags) {

        case ALLOC_NORMAL:				/* add normal page */
  	 addpage_user(startpage,process,physpage);
         if(process == getpid()) loadpagetable(process);
	 break;

	case ALLOC_KERNEL:
	 addpage_system(startpage,process,physpage); /* add kernel page */
         if(process == getpid()) loadpagetable(process);
         break;

	case ALLOC_GLOBAL:
	 addpage_user(startpage,process,physpage); /* add global page */
         if(process == getpid()) loadpagetable(process);
         break;

        }
	    
	startpage=startpage+PAGE_SIZE;
      }

    }

   mb++;
   physpage=physpage+PAGE_SIZE;
 }
 
*last=(size_t *) -1;							/* mark end of chain */

if(flags != ALLOC_NOPAGING) memset(firstpage,0,size-1);

unlock_mutex(&memmanager_mutex);

setlasterror(NO_ERROR);
return((void *) firstpage);
}

/*
 * free memory
 *
 */

unsigned int free_internal(size_t process,void *b) {
 size_t pc;
 size_t count;
 size_t p;
 size_t c;
 size_t *z;
 
 lock_mutex(&memmanager_mutex);

 c=getphysicaladdress(process,b);
 if(c == -1) return(-1);	/* bad address */

 c=(c / PAGE_SIZE) * sizeof(size_t);
 z=MEMBUF_START+c;

 pc=b;
 pc &= ((size_t) 0-1)-(PAGE_SIZE-1);

/* follow chain, removing entries */
 p=*z;	

do {
 p=*z;	

 
 *z=0;							/* remove from allocation table */
 z=p;

  removepage(pc,process);				/* remove page from page table */
  pc=pc+PAGE_SIZE;
 }  while(p != -1);


if(c == NULL) {						
 unlock_mutex(&memmanager_mutex);

 setlasterror(NO_MEM);
 return(-1);
 }

 unlock_mutex(&memmanager_mutex);

 setlasterror(NO_ERROR);
 return(NO_ERROR);
}


void *alloc(size_t size) {
 return(alloc_int(ALLOC_NORMAL,getpid(),size,-1));
}

unsigned int free(void *b) {					/* free memory (0x80000000 - 0xfffffffff) */
 free_internal(getpid(),b);
 return(NO_ERROR);
}

void *kernelalloc(size_t size) {				/* allocate kernel memory (0 - 0x80000000) */
 return(alloc_int(ALLOC_KERNEL,getpid(),size,-1));
}

unsigned int kernelfree(void *b) {
 return(free_internal(getpid(),b));
}

void *kernelalloc_nopaging(size_t size) {
 return(alloc_int(ALLOC_NOPAGING,getpid(),size,-1));
}

/* allocate memory from identity dma memory pool  */

void *dma_alloc(size_t size) {
void *newptr=dmaptr;

 if(dmaptr+size > dmabuf+dmabufsize) {		/* out of memory */
  setlasterror(NO_MEM);
  return(-1);
 }

 if(size < PAGE_SIZE) size=PAGE_SIZE;

 if(size % PAGE_SIZE != 0) {
  size += PAGE_SIZE;				/* round up */
  size -= (size % PAGE_SIZE);
 }

dmaptr += size;
return(newptr);
}

int memorymanager_init(size_t dmasize) {
 size_t count;
 size_t dmap;

 dmabuf=kernelalloc_nopaging(dmasize);		/* get physical address */
 if(dmabuf == NULL) {
  kprintf("kernel: Unable to allocate DMA buffer\n");
  return(-1);
 }

 dmaptr=dmabuf;

 dmap=dmabuf;
 for(count=0;count<dmasize/PAGE_SIZE;count++) {
   addpage_system(dmap+KERNEL_HIGH,0,dmap);

   dmap += PAGE_SIZE;
 }

 dmabufsize=dmasize;

 initialize_mutex(&memmanager_mutex);		/* intialize mutex */

 return;
}

