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

/* address */
/* AAAAAAAAAABBBBBBBBBBCCCCCCCCCCCC */ 
	
/* Paging is divided up into 4MB chunks, which are subdivided into 4k entires which represent a 4k block of *physical* memory */
/*   AAAAAAAAAA				       BBBBBBBBBB	    CCCCCCCCCCCC
* |  4MB Chunk     	         |    

* | Points to 1024 page tables  | ->  | 1024 4kb page entries |-> | 4k page |	

/* add page */

#include <stdint.h>
#include <stddef.h>
#include "kernelhigh.h"
#include "pagesize.h"
#include "page.h"
#include "hwdefs.h"
#include "errors.h"
#include "debug.h"
#include "memorymanager.h"
#include "string.h"

size_t paging_type=1;

struct ppt {
	uint32_t pagedir[1024];
	uint32_t pagedirphys;
	size_t process; 
	struct ppt *next;
} *processpaging=PAGE_DIRECTORY_LOCATION+KERNEL_HIGH;

struct ppt *processpaging_end;  

/*
* Map virtual address to physical address
*
* In: page_flags	Allocation mode(ALLOC_KERNEL or ALLOC_NORMAL)
      process 		Process ID
      virtual_address	Virtual address
      physical_address	Physical address
*
* Returns 0 on success or -1 on error
* 
*/
size_t map_page_internal(size_t page_flags,size_t process,uint32_t virtual_address,void *physical_address) { 
struct ppt *next;
uint32_t pagedir_entry_number;
uint32_t pagetable_entry_number;
uint32_t *pagetableptr;
uint32_t newpagedirentry;
struct ppt *current_page_mapping=NULL;
struct ppt *update=NULL;
size_t savemapping;

next=processpaging;

while(next != NULL) {					/* find process */	
	if(next->process == process) update=next;
	if(next->process == getpid()) current_page_mapping=next;	

	if((update != NULL) && (current_page_mapping != NULL)) break;	/* found mappings */

	next=next->next;
}

if((update == NULL) || (current_page_mapping == NULL)) {
	setlasterror(INVALID_PROCESS);
	return(-1);
}

pagedir_entry_number=(virtual_address >> 22) & 0x3ff;				/* page directory offset */
pagetable_entry_number=(virtual_address >> 12) & 0x3ff;				/* page table offset */

/* if mapping a lower-half page for another process, then update the recursive paging to show the processes' user pages. 
   Kernel pages are the same for every process and don't need this */

if((process != getpid()) && (virtual_address < KERNEL_HIGH)) {
	savemapping=current_page_mapping->pagedir[1023];			/* save current recursive mapping */

	current_page_mapping->pagedir[1023]=update->pagedir[1023];		/* update mapping */
}

if(update->pagedir[pagedir_entry_number] == 0) {				/* if page directory entry is empty */	
	newpagedirentry=kernelalloc_nopaging(PAGE_SIZE);
	if(newpagedirentry == NULL) {
		if(process != getpid()) current_page_mapping->pagedir[1023]=savemapping;	/* restore original mapping */
		return(-1);
	}

	update->pagedir[pagedir_entry_number]=(uint32_t) newpagedirentry | PAGE_USER | PAGE_RW | PAGE_PRESENT;	/* page directory entry */

	pagetableptr=(uint32_t *) 0xffc00000 + (pagedir_entry_number*1024);
	memset(pagetableptr,0,PAGE_SIZE);
}

/* if it's a user page, either map it into the current process page tables or another process page table */

/* page tables are mapped at 0xffc00000 via recursive paging */

pagetableptr=(uint32_t *) 0xffc00000 + (pagedir_entry_number*1024);
pagetableptr[pagetable_entry_number]=((uint32_t) physical_address & 0xfffff000) | page_flags;	/* map page */

if(process != getpid()) current_page_mapping->pagedir[1023]=savemapping;	/* restore original mapping */

/* map page into all address spaces if it is a kernel page */

if(virtual_address >= KERNEL_HIGH) {
	update=processpaging;

	while(update != NULL) {
		
		if(update->process != process) {
			savemapping=current_page_mapping->pagedir[1023];

			current_page_mapping->pagedir[1023]=update->pagedir[1023];
			pagetableptr=(uint32_t *) 0xffc00000 + (pagedir_entry_number*1024);

			pagetableptr[pagetable_entry_number]=((uint32_t) physical_address & 0xfffff000) | page_flags;	/* map page */

			current_page_mapping->pagedir[1023]=savemapping;
		}

		update=update->next;
	}
}

return(0);
}

/*
* Initialize process address space
*
* In: process	Process ID
*
* Returns nothing
* 
*/
size_t page_init(size_t process) {
uint32_t oldvirtaddress;
uint32_t virtaddress;
uint32_t old_pagingentryptr_phys;
uint32_t pagingentryptr_phys;
size_t count;
size_t size;

if(process == 0) return(0);

/* get a physical address and manually map it to a virtual address, the physical addres of p->pagedir is needed to fill cr3 */

size=(sizeof(struct ppt) & ((0-1)-(PAGE_SIZE-1)))+PAGE_SIZE;		/* round up size to multiple of PAGE_SIZE */

pagingentryptr_phys=kernelalloc_nopaging(size);					/* get the physical address of the new paging entry */
if(pagingentryptr_phys == NULL) return(-1);

virtaddress=findfreevirtualpage(size,ALLOC_KERNEL,0);		/* find free virtual address */

oldvirtaddress=virtaddress;
old_pagingentryptr_phys=pagingentryptr_phys;

for(count=0;count<(size/PAGE_SIZE)+1;count++) {
	map_system_page(virtaddress,0,(void *) pagingentryptr_phys);

	pagingentryptr_phys += PAGE_SIZE;
	virtaddress += PAGE_SIZE;
}

/* Important: make sure that is processpaging_end == processpaging. This most be done in the paging intialization code */

if(processpaging_end == NULL) processpaging_end=processpaging; /* do it anyway */

processpaging_end->next=oldvirtaddress;					/* add link to list */
processpaging_end=processpaging_end->next;		/* point to end */
processpaging_end->pagedirphys=old_pagingentryptr_phys;						/* physical address of page directory */
processpaging_end->process=process;

memset(processpaging_end->pagedir,0,PAGE_SIZE);

memcpy(&processpaging_end->pagedir[512],&processpaging->pagedir[512],PAGE_SIZE/2);		/* copy higher half of page directory */

processpaging_end->pagedir[1023]=processpaging_end->pagedirphys+(PAGE_RW | PAGE_PRESENT);	/* recursively map process page tables */

processpaging_end->next=NULL;
return(0);
}

/*
* Unmap page
*
* In: page	Page to unmap
*     process	Process ID
*
* Returns NULL
* 
*/
size_t unmap_page(uint32_t page,size_t process) {
return(map_page_internal(0,process,page,0));						/* clear page */
}

/*
* Free address space
*
* In: process	Process ID
*
*/
size_t freepages(size_t process) {
struct ppt *next;
struct ppt *previous;
size_t pagecount;
size_t entrycount;
size_t *pagetableptr;
size_t physaddress=0;
struct ppt *current_page_mapping=NULL;
struct ppt *update=NULL;
size_t savemapping;

next=processpaging;

while(next != NULL) {					/* find process */	
	if(next->next != NULL) {
		if(next->next->process == process) previous=next;
	}

	if(next->process == process) update=next;		
	if(next->process == getpid()) current_page_mapping=next;	

	if((update != NULL) && (current_page_mapping != NULL)) break;	/* found mappings */

	next=next->next;
}


if((update == NULL) || (current_page_mapping == NULL) || ((next->next != NULL) && (previous == NULL))) {
	setlasterror(INVALID_PROCESS);
	return(-1);
}

/* remove physical addresses from page tables and remove them from the memory map */

for(pagecount=0;pagecount<512;pagecount++) {

	if(next->pagedir[pagecount] != 0) {

		if((process != getpid()) && (pagecount < KERNEL_HIGH)) {
			savemapping=current_page_mapping->pagedir[1023];
			current_page_mapping->pagedir[1023]=update->pagedir[1023];
		}

		pagetableptr=(uint32_t *) 0xffc00000 + (pagecount*1024);
		
		for(entrycount=0;entrycount<1023;entrycount++) {    
		//      if(pagetableptr[entrycount] != 0) free_physical(pagetableptr[entrycount]);
	 	}

	}

	pagetableptr++;

}

if((process != getpid()) && (pagecount < KERNEL_HIGH)) current_page_mapping->pagedir[1023]=savemapping;

/* remove page directory entry */

previous->next=next->next;	

// kernelfree(next);						/* and free it */

return(0);
}

/*
* find free virtual address
*
* In: size	Number of free bytes to find
      alloc	Allocation flags (ALLOC_KERNEL to find in top half of memory, ALLOC_NORMAL to find in bottom half)
      process	Process ID

* Returns first free virtual address of size on success or -1 on error
* 
*/
size_t findfreevirtualpage(size_t size,size_t alloc,size_t process) {
uint32_t s;
size_t start;
size_t end;
size_t pagedircount;
size_t pagetablecount;
uint32_t *pagetableptr;
size_t sz;
size_t last;
struct ppt *next=NULL;
struct ppt *current_page_mapping=NULL;
struct ppt *update=NULL;
size_t savemapping;

next=processpaging;

while(next != NULL) {					/* find process */	
	if(next->process == process) update=next;
	if(next->process == getpid()) current_page_mapping=next;	

	if((update != NULL) && (current_page_mapping != NULL)) break;	/* found mappings */

	next=next->next;
}

if((update == NULL) || (current_page_mapping == NULL)) {
	setlasterror(INVALID_PROCESS);
	return(-1);
}

size &= (0-1)-(PAGE_SIZE-1);		/* round */

/* walk through page directories and tables to find free pages in directories already used */

if(alloc == ALLOC_NORMAL) {
	start=0;	
	end=511;
}
else
{
	start=512;
	end=1023;
}

if((process != getpid()) && (alloc == ALLOC_NORMAL)) {
	savemapping=current_page_mapping->pagedir[1023];
	current_page_mapping->pagedir[1023]=update->pagedir[1023];
}

for(pagedircount=start;pagedircount < end;pagedircount++) {			/* page directories */
	if(update->pagedir[pagedircount] != 0) {						/* in use */

		s=0;

		pagetableptr=(uint32_t *) 0xffc00000 + (pagedircount*1024);

		for(pagetablecount=0;pagetablecount < 1022;pagetablecount++) {      

			if(pagetableptr[pagetablecount] == 0) {
	        		if(s == 0) last=pagetablecount;

				if(s > size) {
					if((process != getpid()) && (alloc == ALLOC_NORMAL)) current_page_mapping->pagedir[1023]=savemapping;

					return((pagedircount << 22)+(last << 12)); /* found enough */	
				}

	 	  		s += PAGE_SIZE;		
	     		}
	    		else
	    		{
	     			last=pagetablecount;
	     			s=0;			/* must be contiguous */
	    		}
	  	}

	}
}

if((process != getpid()) && (alloc == ALLOC_NORMAL)) current_page_mapping->pagedir[1023]=savemapping;

s=0;

for(pagedircount=start;pagedircount < 1022;pagedircount++) {			/* page directories */
	if(update->pagedir[pagedircount] == 0) {
		s=s+(1024*1024)*4;						/* found 4Mb */
		if(s >= size)  return((pagedircount << 22));	
	}
}

setlasterror(NO_MEM);
return(-1);
}

/*
* Load page tables for process
*
* In: process		Process ID
*
* Returns NULL
* 
*/
size_t switch_address_space(size_t process) {
struct ppt *next; 

next=processpaging;

while(next != NULL) {
	if(next->process == process) {
		asm volatile("mov %0, %%cr3":: "a"(next->pagedirphys));
		return(0);
	}

	next=next->next;
}


return(-1);
}

/*
* Get page table entry from virtual address
*
* In: process		Process ID
      virtual_address	Virtual address
*
* Returns physical address or -1 on error
* 
*/
size_t get_pagetable_entry(size_t process,uint32_t virtual_address) {
uint32_t pagedir_entry_number;
uint32_t pagetable_entry_number;
uint32_t *pagetableptr;
struct ppt *next;
struct ppt *current_page_mapping;
struct ppt *update;
size_t retval;
size_t savemapping;

next=processpaging;

while(next != NULL) {					/* find process */	
	if(next->process == process) update=next;		
	if(next->process == getpid()) current_page_mapping=next;	

	if((update != NULL) && (current_page_mapping != NULL)) break;	/* found mappings */

	next=next->next;
}

if((update == NULL) || (current_page_mapping == NULL)) {
	setlasterror(INVALID_PROCESS);
	return(-1);
}

pagedir_entry_number=(virtual_address >> 22) & 0x3ff;				/* page directory offset */
pagetable_entry_number=(virtual_address >> 12) & 0x3ff;				/* page table offset */

if((process != getpid()) && (virtual_address < KERNEL_HIGH)) {	/* map process recursively for access */
	savemapping=current_page_mapping->pagedir[1023];
	current_page_mapping->pagedir[1023]=update->pagedir[1023];
}

pagetableptr=(uint32_t *) 0xffc00000 + (pagedir_entry_number*1024);

if((pagetableptr[pagetable_entry_number] & PAGE_PRESENT) == 0) {
	retval=-1;	/* page not present */
}
else
{
	retval=pagetableptr[pagetable_entry_number];
}

if((process != getpid()) && (virtual_address < KERNEL_HIGH)) current_page_mapping->pagedir[1023]=savemapping;	/* Restore mappings */

return(retval);
}

/*
* Get physical address from virtual address
*
* In:  process		Process ID
       virtual_address	Virtual address
*
* Returns physical address or -1 on error
* 
*/
size_t getphysicaladdress(size_t process,uint32_t virtual_address) {
size_t entry=get_pagetable_entry(process,virtual_address);

if(entry == -1) return(-1);

return(entry & 0xfffff000);				/* return physical address */
}

/*
* Add user page
*
* In: page		Virtual address       
      process		Process ID
      physical_address	Physical address

* Returns 0 on success or -1 on error
* 
*/
size_t map_user_page(uint32_t virtual_address,size_t process,void *physical_address) {
return(map_page_internal(PAGE_USER+PAGE_RW+PAGE_PRESENT,process,virtual_address,physical_address));
}

/*
* Add system page
*
* In: virtual_address  Virtual address       
      process	       Process ID
      physical_address Physical address

* Returns 0 on success or -1 on error
* 
*/
size_t map_system_page(uint32_t virtual_address,size_t process,void *physical_address) { 
return(map_page_internal(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,process,virtual_address,physical_address));
}

size_t get_paging_type(void) {
return(PAGING_TYPE_LEGACY);
}

