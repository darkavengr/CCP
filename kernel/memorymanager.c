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

/* memory manager */
	
#define ALLOC_NORMAL	0
#define ALLOC_KERNEL	1
#define ALLOC_NOPAGING 	2
#define ALLOC_GLOBAL	4

#include <stdint.h>
#include <stddef.h>
#include "kernelhigh.h"
#include "errors.h"
#include "bootinfo.h"
#include "memorymanager.h"
#include "mutex.h"
#include "debug.h"
#include "page.h"

extern kernel_begin(void);
extern size_t PAGE_SIZE;

extern size_t MEMBUF_START;

void *dmabuf=NULL;
void *dmaptr=NULL;
size_t dmabufsize=0;
MUTEX memmanager_mutex;

/*
* Internal allocator function
*
* In: flags		Allocation flags(ALLOC_NORMAL,ALLOC_KERNEL or ALLOC_GLOBAL)
      process		Process ID of process to allocate memory to
      size		Number of bytes to allocate
      overrideaddress	Address to force allocation to. If -1, use first available address
*
* Returns start address of allocated memory on success,NULL otherwise
* 
*/
void *alloc_int(size_t flags,size_t process,size_t size,size_t overrideaddress) {
size_t count;
size_t r;
size_t *next;
size_t *mb=(size_t *) -1;
size_t pcount;
size_t startpage;
size_t physpage;
size_t firstpage;
size_t *m;
size_t countx;
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;		/* point to boot information */
size_t oldsize=size;

if(size >= bootinfo->memorysize) {							/* sanity check */
	setlasterror(NO_MEM);
	return(NULL);
}

if(size < PAGE_SIZE) size=PAGE_SIZE;		/* if size is less than a page, round up to page size */

if((size % PAGE_SIZE) != 0) {
	size=(size & ((0-1)-(PAGE_SIZE-1)))+PAGE_SIZE;		/* round */
}

lock_mutex(&memmanager_mutex);

m=(size_t *) MEMBUF_START;

pcount=0;
mb=m; 
startpage=0;

/* check if enough free memory */

for(count=0;count<(bootinfo->memorysize/PAGE_SIZE);count++) {
	if(*mb == 0) {
		if(startpage == 0) startpage=count;

		if(pcount == (size / PAGE_SIZE)) break;			/* found enough */

		pcount++;
	}

	 mb++;
}

if((pcount < (size/PAGE_SIZE)) || (count == (bootinfo->memorysize/PAGE_SIZE)-1)) {					/* out of memory */
	unlock_mutex(&memmanager_mutex);

	 setlasterror(NO_MEM);
	 return(NULL);
}

/* first start virtual address */

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
	
for(count=0;count != (bootinfo->memorysize/PAGE_SIZE)+1;count++) {

	if(*mb == 0) {
		pcount++;

	/* find next in memory map */

		next=mb;
		next++;

		for(countx=count+1;countx<(bootinfo->memorysize/PAGE_SIZE)+1;countx++) {
	  		if(*next == 0) break;
	  		next++;
	     	}
	  
	     	*mb=(size_t *) next;
	  
/* link physical addresses to virtual addresses */
	 
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

		if(pcount == (size/PAGE_SIZE)+1) break;			/* found enough */
	}

	mb++;
	physpage=physpage+PAGE_SIZE;
}

*mb=(size_t *) -1;							/* mark end of chain */

//if(flags != ALLOC_NOPAGING) memset(firstpage,0,size-1);

unlock_mutex(&memmanager_mutex);

setlasterror(NO_ERROR);
return(firstpage);
}

/*
* Internal free memory function
*
* In: 	process		Process to free memory from
*	b		Start address of memory area
*	flags		Flags (1=free physical address)
*
* Returns 0 on success, -1 otherwise
* 
*/
size_t free_internal(size_t process,void *b,size_t flags) {
size_t pc;
size_t count;
size_t p;
size_t c;
size_t *z;

lock_mutex(&memmanager_mutex);

/* Get start of memory chain */

if((flags & FREE_PHYSICAL)) {			/* freeing physical address */
	c=(size_t) b;
}
else
{
	c=getphysicaladdress(process,b);
	if(c == -1) {
		unlock_mutex(&memmanager_mutex);
		return(-1);					/* bad address */
	 }
}

c=(c / PAGE_SIZE) * sizeof(size_t);
z=MEMBUF_START+c;

pc=b;
pc &= ((size_t) 0-1)-(PAGE_SIZE-1);

/* follow chain, removing entries */

do {
	p=*z;				/* get next */

	if(p == 0) break;

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

/*
* Allocate to memory to user process
*
* In: size	Number of bytes to allocate
*
* Returns start address on success, NULL otherwise
* 
*/
void *alloc(size_t size) {
	return(alloc_int(ALLOC_NORMAL,getpid(),size,-1));
}

/*
* Free memory from user process
*
* In: b	Start address of memory area
*
* Returns 0 on success, -1 otherwise
* 
*/
size_t free(void *b) {					/* free memory (0x00000000 - 0x7fffffff) */
return(free_internal(getpid(),b,0));
}

/*
* Free memory from physical address

*
* In: b	Start address of memory area
*
* Returns 0 on success, -1 otherwise
* 
*/
size_t free_physical(void *b) {					/* free memory (0x00000000 - 0x7fffffff) */
	return(free_internal(getpid(),b,FREE_PHYSICAL));
}

/*
* Allocate to memory to kernel
*
* In: size	Number of bytes to allocate
*
* Returns start address on success, NULL otherwise
* 
*/
void *kernelalloc(size_t size) {				/* allocate kernel memory (0 - 0x80000000) */
return(alloc_int(ALLOC_KERNEL,getpid(),size,-1));
}

/*
* Free memory from kernel
*
* In: b	Start address of memory area
*
* Returns 0 on success, -1 otherwise
* 
*/
size_t kernelfree(void *b) {
return(free_internal(getpid(),b,0));
}

/*
* Allocate to memory and return physical address
*
* In: size	Number of bytes to allocate
*
* Returns start address on success, NULL otherwise
* 
*/
void *kernelalloc_nopaging(size_t size) {
return(alloc_int(ALLOC_NOPAGING,getpid(),size,-1));
}

/*
* Allocate memory from DMA buffer
*
* In: size	Number of bytes to allocate
*
* Returns start address on success, -1 otherwise
* 
*/
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

/*
* Initialize memory manager
*
* In: dmasize	Number of bytes to allocate for DMA buffers
*
* Returns start address on success, NULL otherwise
* 
*/

size_t memorymanager_init(size_t dmasize) {
size_t count;
size_t dmap;

/* allocate physical memory and map it to identical virtual addresses */

dmabuf=kernelalloc_nopaging(dmasize);		/* allocate memory and return physical address */
if(dmabuf == NULL) {
	 kprintf_direct("kernel: Unable to allocate DMA buffer\n");
	 return(-1);
}

/* map physical addresses to virtual addresses */

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

