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

#include <stdint.h>
#include <stddef.h>
#include "kernelhigh.h"
#include "errors.h"
#include "bootinfo.h"
#include "memorymanager.h"
#include "mutex.h"
#include "debug.h"
#include "page.h"

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
size_t *memory_map_entry_ptr=NULL;
size_t page_count;
size_t virtual_address=0;
size_t physical_address=0;
size_t first_virtual_address=0;
size_t *start_of_chain=NULL;
size_t next_free_entry_count;
size_t *next_memory_map_ptr=NULL;
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;		/* point to boot information */

if(size >= bootinfo->memorysize) {				/* sanity check */
	setlasterror(NO_MEM);
	return(NULL);
}

if(size < PAGE_SIZE) size=PAGE_SIZE;				/* if size is less than a page, round up to page size */

if((size % PAGE_SIZE) != 0) {
	size=(size & ((0-1)-(PAGE_SIZE-1)))+PAGE_SIZE;		/* round up size to PAGE_SIZE */
}

lock_mutex(&memmanager_mutex);

page_count=0;
memory_map_entry_ptr=MEMBUF_START;

/* check if enough free memory, find start of free chain and starting physical address */

for(count=0;count<(bootinfo->memorysize/PAGE_SIZE);count++) {	
	if(*memory_map_entry_ptr == 0) {
		if(start_of_chain == NULL) start_of_chain=memory_map_entry_ptr;		/* save start of chain */

		if(++page_count == (size / PAGE_SIZE)) break;			/* found enough */
	}

	physical_address += PAGE_SIZE;
	memory_map_entry_ptr++;
}

if((page_count < (size/PAGE_SIZE)) || (count == (bootinfo->memorysize/PAGE_SIZE)-1)) {					/* out of memory */
	unlock_mutex(&memmanager_mutex);

	 setlasterror(NO_MEM);
	 return(NULL);
}

/* first start virtual address */

if(flags != ALLOC_NOPAGING) {

	if(overrideaddress == -1) { 					/* find first free virtual address */
		first_virtual_address=findfreevirtualpage(size,flags,process); 

		if(first_virtual_address == -1) {
			unlock_mutex(&memmanager_mutex);

	   		setlasterror(NO_MEM);
	   		return(NULL);
	  	}
	 
	  	virtual_address=first_virtual_address;
	}
	else
	{					/* use override address */
		virtual_address=overrideaddress;
		first_virtual_address=overrideaddress;
	 }

}
else
{
	first_virtual_address=physical_address;		/* use physical address */
}

/* create new chain in buffer */

memory_map_entry_ptr=start_of_chain;			/* point to start of new chain */

page_count=0;
	
while(count != (bootinfo->memorysize/PAGE_SIZE)+1) {

	if(*memory_map_entry_ptr == 0) {
		page_count++;

	/* find next in memory map */

		next_memory_map_ptr=memory_map_entry_ptr;
		next_memory_map_ptr++;

		for(next_free_entry_count=count+1;next_free_entry_count<(bootinfo->memorysize/PAGE_SIZE)+1;next_free_entry_count++) {
	  		if(*next_memory_map_ptr == 0) break;

			next_memory_map_ptr++;
	     	}

	     	*memory_map_entry_ptr=(size_t *) next_memory_map_ptr;
	  
/* link physical addresses to virtual addresses */

	     	if(flags != ALLOC_NOPAGING) {	
	
	     		switch(flags) {
	     			case ALLOC_NORMAL:				/* add user-mode page */
	 				addpage_user(virtual_address,process,physical_address);
	        			if(process == getpid()) loadpagetable(process);
					break;

				case ALLOC_KERNEL:
	 				addpage_system(virtual_address,process,physical_address); /* add kernel page */

	        			if(process == getpid()) loadpagetable(process);
	        			break;

				case ALLOC_GLOBAL:
	 				addpage_user(virtual_address,process,physical_address); /* add global page */

	        			if(process == getpid()) loadpagetable(process);
	       				break;	    
	       		}

			virtual_address += PAGE_SIZE;
	     	}
	}

	memory_map_entry_ptr++;
	physical_address += PAGE_SIZE;
	
	if(page_count > (size/PAGE_SIZE)) break;			/* found enough */

	count++;
}

*memory_map_entry_ptr=(size_t *) -1;							/* mark end of chain */

if(flags != ALLOC_NOPAGING) memset(first_virtual_address,0,size-1);

unlock_mutex(&memmanager_mutex);

setlasterror(NO_ERROR);

return((void *) first_virtual_address);
}
/*
* Internal free memory function
*
* In: 	process		Process ID
*	b		Start address of memory area
*	flags		Flags (1=free physical address)
*
* Returns 0 on success, -1 otherwise
* 
*/
size_t free_internal(size_t process,void *address,size_t flags) {
size_t pc;
size_t count;
size_t p;
size_t c;
size_t *z;

lock_mutex(&memmanager_mutex);

/* Get start of memory chain */

if((flags & FREE_PHYSICAL)) {			/* freeing physical address */
	c=(size_t) address;
}
else
{
	c=getphysicaladdress(process,address);
	if(c == -1) {
		unlock_mutex(&memmanager_mutex);
		return(-1);					/* bad address */
	 }
}

c=(c / PAGE_SIZE) * sizeof(size_t);
z=MEMBUF_START+c;

pc=address;
pc &= ((size_t) 0-1)-(PAGE_SIZE-1);

/* follow chain, removing entries */

do {
	p=*z;				/* get next */

	if(p == 0) break;

	*z=0;							/* remove from allocation table */
	z=p;

	if((p != -1) || (p == 0)) removepage(pc,process);				/* remove page from page table */
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

size_t memorymanager_init(void) {
initialize_mutex(&memmanager_mutex);		/* intialize mutex */

if(initialize_dma_buffer(DMA_BUFFER_SIZE) == -1) {		/* inititalize DMA buffer */
	 kprintf_direct("kernel: Unable to allocate DMA buffer\n");
	 return(-1);
}
 
return(0);
}

size_t initialize_dma_buffer(size_t dmasize) {
size_t count;
size_t dmap;

/* allocate physical memory and map it to identical virtual addresses */

dmabuf=kernelalloc_nopaging(dmasize);		/* allocate memory and return physical address */
if(dmabuf == NULL) return(-1);

/* map physical addresses to virtual addresses */

dmaptr=dmabuf;

dmap=dmabuf;

for(count=0;count<dmasize/PAGE_SIZE;count++) {
	  addpage_system(dmap+KERNEL_HIGH,0,dmap);

	  dmap += PAGE_SIZE;
}

dmabufsize=dmasize;

return(0);
}

/*
* Resize memory for kernel
*
* In: b	Start address of memory area
*
* Returns 0 on success, -1 otherwise
* 
*/

void *realloc_kernel(void *address,size_t size) {
return(realloc_int(ALLOC_KERNEL,getpid(),address,size));
}

/*
* Resize memory for user process
*
* In: b	Start address of memory area
*
* Returns 0 on success, -1 otherwise
* 
*/

void *realloc_user(void *address,size_t size) {
return(realloc_int(ALLOC_NORMAL,getpid(),address,size));
}


/*
* Resize memory internal function
*
* In: flags		Allocation flags(ALLOC_NORMAL or ALLOC_KERNEL)
      process		Process ID
      address		Address of previously allocated memory
      size		New allocation size
* 
* Returns: address or NULL on error.
*
*/	


void *realloc_int(size_t flags,size_t process,void *address,size_t size) {
char *tempbuf;
char *newbuf;
size_t pc;
size_t count;
size_t p=0;
size_t c;
size_t *z;
size_t *saveptr;
size_t extendcount;

lock_mutex(&memmanager_mutex);

/* Get start of memory chain */

if((flags & ALLOC_NOPAGING) || (size == 0)) {
	setlasterror(INVALID_ADDRESS);
	return(NULL);

}
else
{
	c=getphysicaladdress(process,address);
	if(c == -1) {	
		unlock_mutex(&memmanager_mutex);

		setlasterror(INVALID_ADDRESS);
		return(NULL);
	 }
}

if(size < PAGE_SIZE) size=PAGE_SIZE;		/* if size is less than a page, round up to page size */

if((size % PAGE_SIZE) != 0) {
	size=(size & ((0-1)-(PAGE_SIZE-1)))+PAGE_SIZE;		/* round */
}

c=(c / PAGE_SIZE) * sizeof(size_t);
z=MEMBUF_START+c;

pc=address;
pc &= ((size_t) 0-1)-(PAGE_SIZE-1);

count=0;

do {
	p=*z;				/* get next */

	if((count*PAGE_SIZE) == size) {		 /* shrinking memory */
		if(free_internal(process,(address+size),0) == -1) {
			unlock_mutex(&memmanager_mutex);
			return(NULL);
		}

		z=(address+count);
		*z=-1;			/* put new end of chain marker */
		
		unlock_mutex(&memmanager_mutex);
		return(address);
	}

	if(p == -1) {			/* at end of chain */
		z=p;			/* point to next in chain */

		if(size > (count*PAGE_SIZE)) {		/* extending memory */
			/* check for sufficient contiguous virtual address pages */
			
			for(extendcount=1;extendcount != (size-(count*PAGE_SIZE));extendcount++) {
				if(getphysicaladdress(process,(address+(extendcount*PAGE_SIZE))) == -1) break;
			}

			if((extendcount*PAGE_SIZE) != (size-(count*PAGE_SIZE))) {		/* no contiguous address space found */
				/* kludge */

				newbuf=kernelalloc(size);		/* allocate new memory */
				if(newbuf == NULL) {
					unlock_mutex(&memmanager_mutex);
					return(NULL);
				}

				memcpy(newbuf,address,(count*PAGE_SIZE));		/* copy data */
			
				if(free_internal(process,(address+size),0) == -1) {
					kernelfree(newbuf);
					unlock_mutex(&memmanager_mutex);
					return(NULL);
				}

				setlasterror(NO_ERROR);
				return(newbuf);
			}

			/* contiguous address space found */

			if(alloc_int(flags,process,(count*PAGE_SIZE),address+(count*PAGE_SIZE)) == NULL) {
				unlock_mutex(&memmanager_mutex);
				return(NULL);
			}

			unlock_mutex(&memmanager_mutex);
			return(address);
		}				
	}	

	count++;		
	z=p;				/* get next entry in chain */

}  while(p != -1);

unlock_mutex(&memmanager_mutex);
return(NULL);
}
