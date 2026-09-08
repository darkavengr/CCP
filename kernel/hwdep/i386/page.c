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

*/

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

PAGING *processpaging=PAGE_DIRECTORY_LOCATION+KERNEL_HIGH;
PAGING *processpaging_end=PAGE_DIRECTORY_LOCATION+KERNEL_HIGH;

/*
* Map virtual address to physical address
*
* In: page_flags	Allocation mode(ALLOC_KERNEL or ALLOC_NORMAL)
      virtual_address	Virtual address
      physical_address	Physical address
*
* Returns 0 on success or -1 on error
* 
*/
size_t map_page_internal(size_t page_flags,size_t virtual_address,void *physical_address) { 
PAGING *next;
uint32_t pagedir_entry_number;
uint32_t pagetable_entry_number;
uint32_t *pagetableptr;
uint32_t newpagedirentry;
uint32_t *pagedirptr;

pagedir_entry_number=(virtual_address >> 22) & 0x3ff;				/* page directory offset */
pagetable_entry_number=(virtual_address >> 12) & 0x3ff;				/* page table offset */

pagedirptr=PAGEDIR_ADDRESS;			/* point to page directory */

if(pagedirptr[pagedir_entry_number] == 0) {				/* if page directory entry is empty */
	newpagedirentry=kernelalloc_nopaging(PAGE_SIZE);
	if(newpagedirentry == NULL) return(-1);

	pagedirptr[pagedir_entry_number]=(uint32_t) newpagedirentry | PAGE_USER | PAGE_RW | PAGE_PRESENT;	/* page directory entry */
}

/* page tables are mapped at PAGETABLE_ADDRESS via recursive paging */

pagetableptr=(uint32_t *) PAGETABLE_ADDRESS + (pagedir_entry_number*1024);
pagetableptr[pagetable_entry_number]=((uint32_t) physical_address & 0xfffff000) | page_flags;	/* map page */

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

size=round_up(sizeof(PAGING),PAGE_SIZE);	/* round up to PAGE _SIZE */

pagingentryptr_phys=kernelalloc_nopaging(size);					/* get the physical address of the new paging entry */
if(pagingentryptr_phys == NULL) return(-1);

virtaddress=findfreevirtualpage(size,ALLOC_KERNEL);		/* find free virtual address */

oldvirtaddress=virtaddress;
old_pagingentryptr_phys=pagingentryptr_phys;

for(count=0;count < (size / PAGE_SIZE) + 1;count++) {
	map_system_page(virtaddress,(void *) pagingentryptr_phys);

	pagingentryptr_phys += PAGE_SIZE;
	virtaddress += PAGE_SIZE;
}

/* Important: make sure that is processpaging_end == processpaging. */

if(processpaging_end == NULL) processpaging_end=processpaging;

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
size_t unmap_page(size_t page) {
return(map_page_internal(0,page,0));						/* clear page */
}

/*
* Free address space
*
* In: process	Process ID
*
*/
size_t freepages(size_t process) {
PAGING *next;
PAGING *previous;
PAGING *savenext;
size_t pagedircount;
size_t pagetablecount;
size_t *pagetableptr;

next=processpaging;
previous=next;

while(next != NULL) {					/* find process */	
	if(next->process == process) {
		/* only free lower half */

		for(pagedircount=0;pagedircount < 511;pagedircount++) {
			if(next->pagedir[pagedircount] != 0) {
				pagetableptr=(uint32_t *) PAGETABLE_ADDRESS + (pagedircount*1024);
	
				for(pagetablecount=0;pagetablecount < 1023;pagetablecount++) {    

					if(pagetableptr[pagetablecount] != 0) free_physical(pagetableptr[pagetablecount]);
	 			}
			}
		}


		/* remove processpaging entry */

		if(next == processpaging) {		/* first */
			processpaging=next->next;

			remove_processpaging_entry(next);
		}
		else if(next->next == NULL) {		/* last */
			savenext=next;

			previous->next=NULL;

			remove_processpaging_entry(savenext);
		}
		else					/* middle */
		{
			savenext=next;
			previous->next=next->next;

			remove_processpaging_entry(savenext);
		}

		return(0);
	}

	previous=next;
	next=next->next;
}

setlasterror(INVALID_PROCESS);
return(-1);
}

void remove_processpaging_entry(size_t virtaddress) {
size_t size;
size_t count;

size=round_up(sizeof(PAGING),PAGE_SIZE);	/* round up to PAGE _SIZE */

for(count=0;count < (size / PAGE_SIZE) + 1;count++) {
	unmap_page(virtaddress);

	virtaddress += PAGE_SIZE;
}

return;
}

/*
* find free virtual address
*
* In: size	Number of free bytes to find
      alloc	Allocation flags (ALLOC_KERNEL to find in top half of memory, ALLOC_NORMAL to find in bottom half)

* Returns first free virtual address of size on success or -1 on error
* 
*/
size_t findfreevirtualpage(size_t size,size_t alloc) {
size_t FoundSize;
size_t start;
size_t end;
size_t pagedircount;
size_t pagetablecount;
uint32_t *pagetableptr;
uint32_t *pagedirptr;
size_t last;

size=round_up(size,PAGE_SIZE);	/* round up to PAGE _SIZE */

/* walk through page directories and tables to find free pages in directories already used */

if(alloc == ALLOC_NORMAL) {
	start=0;	
	end=511;
}
else
{
	start=512;
	end=1022;
}

pagedirptr=PAGEDIR_ADDRESS;

for(pagedircount=start;pagedircount < end;pagedircount++) {			/* page directories */
	if(pagedirptr[pagedircount] != 0) {						/* in use */

		FoundSize=0;

		pagetableptr=(uint32_t *) PAGETABLE_ADDRESS + (pagedircount*1024);

		for(pagetablecount=0;pagetablecount < 1023;pagetablecount++) {      

			if(pagetableptr[pagetablecount] == 0) {
	        		if(FoundSize == 0) last=pagetablecount;

				if(FoundSize >= size) return((pagedircount << 22)+(last << 12)); /* found enough */	
				
	 	  		FoundSize += PAGE_SIZE;		
	     		}
	    		else
	    		{
	     			last=pagetablecount;
	     			FoundSize=0;			/* must be contiguous */
	    		}
	  	}

	}
}

FoundSize=0;

/* Search through page directories for free entries */

for(pagedircount=start;pagedircount < 1022;pagedircount++) {
	if(pagedirptr[pagedircount] == 0) {
		FoundSize += (1024*1024)*4;	/* found 4Mb */

		if(FoundSize >= size)  return((pagedircount << 22));	
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
PAGING *next; 

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
* In: virtual_address	Virtual address
*
* Returns physical address or -1 on error
* 
*/
size_t get_pagetable_entry(size_t virtual_address) {
uint32_t pagedir_entry_number;
uint32_t pagetable_entry_number;
uint32_t *pagetableptr;
size_t retval;

pagedir_entry_number=(virtual_address >> 22) & 0x3ff;				/* page directory offset */
pagetable_entry_number=(virtual_address >> 12) & 0x3ff;				/* page table offset */

pagetableptr=(uint32_t *) PAGETABLE_ADDRESS + (pagedir_entry_number*1024);

return(pagetableptr[pagetable_entry_number]);
}

/*
* Get physical address from virtual address
*
* In: virtual_address	Virtual address
*
* Returns physical address or -1 on error
* 
*/
size_t getphysicaladdress(size_t virtual_address) {
size_t entry=get_pagetable_entry(virtual_address);

if(entry == -1) return(-1);

return(entry & 0xfffff000);				/* return physical address */
}

/*
* Get page flags from virtual address
*
* In: virtual_address	Virtual address
*
* Returns physical address or -1 on error
* 
*/
size_t get_page_flags(size_t virtual_address) {
size_t entry=get_pagetable_entry(virtual_address);

if(entry == -1) return(-1);

return(entry & 0xfff);				/* return page flags */
}

/*
* Add user page
*
* In: page		Virtual address       
      physical_address	Physical address

* Returns 0 on success or -1 on error
* 
*/
size_t map_user_page(size_t virtual_address,void *physical_address) {
return(map_page_internal(PAGE_USER+PAGE_RW+PAGE_PRESENT,virtual_address,physical_address));
}

/*
* Add system page
*
* In: virtual_address  Virtual address       
      physical_address Physical address

* Returns 0 on success or -1 on error
* 
*/
size_t map_system_page(size_t virtual_address,void *physical_address) { 
return(map_page_internal(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,virtual_address,physical_address));
}

size_t get_paging_type(void) {
return(PAGING_TYPE_LEGACY);
}

PAGING *GetProcessPagingPointer(void) {
return(processpaging);
}

