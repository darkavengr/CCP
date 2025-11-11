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

extern end(void);
extern kernel_begin(void);

size_t paging_type=1;

struct ppt {
	uint32_t pagedir[1024];
	uint32_t pagedirphys;
	size_t process; 
	struct ppt *next;
} *processpaging=PAGE_DIRECTORY_LOCATION+KERNEL_HIGH;

struct ppt *processpaging_end;
		
/* add page */

/*
* Add page
*
* In: uint32_t mode	Allocation mode(ALLOC_KERNEL or ALLOC_NORMAL)
	     uint32_t process Process ID
	     uint32_t page	Virtual address
	     void *physaddr	Physical address
*
* Returns NULL or -1 on error
* 
*/
size_t addpage_int(size_t mode,size_t process,uint32_t page,void *physaddr) { 
struct ppt *next;
uint32_t pagedir_entry_number;
uint32_t pagetable_entry_number;
uint32_t *pagetableptr;
uint32_t newpagedirentry;
struct ppt *remap;

next=processpaging;

while(next != NULL) {					/* find process */	
	if(next->process == process) break;		

	next=next->next;
}

if(next == NULL) return(-1);

pagedir_entry_number=(page >> 22) & 0x3ff;				/* page directory offset */
pagetable_entry_number=(page >> 12) & 0x3ff;				/* page table offset */

if(next->pagedir[pagedir_entry_number] == 0) {				/* if page directory empty */	
	newpagedirentry=kernelalloc_nopaging(PAGE_SIZE);
	if(newpagedirentry == NULL) return(-1);

	next->pagedir[pagedir_entry_number]=(uint32_t) newpagedirentry | PAGE_USER | PAGE_RW | PAGE_PRESENT;	/* page directory entry */

	pagetableptr=(uint32_t *) 0xffc00000 + (pagedir_entry_number*1024);

	memset(pagetableptr,0,PAGE_SIZE);
}

/* page tables are mapped at 0xffc00000 via recursive paging */

pagetableptr=(uint32_t *) 0xffc00000 + (pagedir_entry_number*1024);
pagetableptr[pagetable_entry_number]=((uint32_t) physaddr & 0xfffff000)+mode;			/* page table */

/* map page into all address spaces if it is a kernel page */

if(page >= KERNEL_HIGH) {
	remap=processpaging;

	while(remap != NULL) {
		if(remap->process != process) {
			if(remap->pagedir[pagedir_entry_number] == 0) {				/* if page directory empty */	
				remap->pagedir[pagedir_entry_number]=(uint32_t) newpagedirentry | PAGE_USER | PAGE_RW | PAGE_PRESENT;	/* page directory entry */
			}

			next->pagedir[1022]=remap->pagedir[1023];
			pagetableptr=(uint32_t *) 0xFF800000 + (pagedir_entry_number*1024);

		
			pagetableptr[pagetable_entry_number]=((uint32_t) physaddr & 0xfffff000)+mode;			/* page table */
		}

		remap=remap->next;
	}

}

return(0);
}

/*
* Initialize process address space
*
* In: size_t process	Process ID
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
	addpage_system(virtaddress,0,(void *) pagingentryptr_phys);

	pagingentryptr_phys += PAGE_SIZE;
	virtaddress += PAGE_SIZE;
}

/* Important: make sure that is processpaging_end == processpaging. This most be done in the paging intialization code */

if(processpaging_end == NULL) processpaging_end=processpaging; /* do it anyway */

processpaging_end->next=oldvirtaddress;					/* add link to list */
processpaging_end=processpaging_end->next;		/* point to end */
processpaging_end->pagedirphys=old_pagingentryptr_phys;						/* physical address of page directory */
processpaging_end->process=process;

memcpy(&processpaging_end->pagedir[512],&processpaging->pagedir[512],PAGE_SIZE/2);		/* copy higher half of page directory */

processpaging_end->pagedir[1023]=processpaging_end->pagedirphys+(PAGE_RW+PAGE_PRESENT);

processpaging_end->next=NULL;
return(0);
}

/*
* Remove page
*
* In: uint32_t page	Page to remove
	     size_t process	Process ID
*
* Returns NULL
* 
*/
size_t removepage(uint32_t page,size_t process) {
return(addpage_int(0,process,page,0));						/* clear page */
}

/*
* Free address space
*
* In: size_t process	Process ID
*
*/
size_t freepages(size_t process) {
struct ppt *next;
struct ppt *last;
size_t pagecount;
size_t entrycount;
size_t *pagetableptr;
size_t physaddress=0;

next=processpaging;
last=processpaging;

while(next != NULL) {					/* find process */
	if(next->process == process) break;		

	last=next;
	next=next->next;
}

if(next == NULL) return(-1);

/* remove physical addresses from page tables and remove them from the memory map */

for(pagecount=0;pagecount<512;pagecount++) {

	if(next->pagedir[pagecount] != 0) {
		pagetableptr=(uint32_t *) 0xffc00000 + (pagecount*1024);		/* point to location of page directory */

		for(entrycount=0;entrycount<1023;entrycount++) {    

		//      asm("xchg %bx,%bx");
		//      if(pagetableptr[entrycount] != 0) free_physical(pagetableptr[entrycount]);
	 	}

	}

	pagetableptr++;

}


/* remove page directory entry */

last->next=next->next;	

// kernelfree(next);						/* and free it */

return(0);
}

/*
* find free virtual address
*
* In: size_t size	Number of free bytes to find
	     size_t alloc	Allocation flags (ALLOC_KERNEL to find in top half of memory, ALLOC_NORMAL to find in bottom half)
	     size_t process		Process ID

* Returns first free virtual address of size on success or -1 on error
* 
*/
size_t findfreevirtualpage(size_t size,size_t alloc,size_t process) {
uint32_t s;
size_t start;
size_t end;
size_t count;
size_t countx;
uint32_t *pagetableptr;
size_t sz;
size_t last;
struct ppt *next;
struct ppt *current;

next=processpaging;
	
while(next != NULL) {	
	if(next->process == getpid()) current=next;					/* find process struct */ 
	if(next->process == process) break;
	next=next->next;
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
	end=1022;
}

for(count=start;count<end;count++) {			/* page directories */

	if(next->pagedir[count] != 0) {						/* in use */

		s=0;

		pagetableptr=(uint32_t *) 0xffc00000 + (count*1024);

		for(countx=0;countx<1022;countx++) {      
			if(pagetableptr[countx] == 0) {
	        		if(s == 0) last=countx;

				if(s > size) return((count << 22)+(last << 12)); /* found enough */
	 	  		s += PAGE_SIZE;		
	     		}
	    		else
	    		{
	     			last=countx;
	     			s=0;				/* must be contiguous */
	    		}
	  	}

	}
}

s=0;

for(count=start;count<1022;count++) {			/* page directories */
	if(next->pagedir[count] == 0) {
		s=s+(1024*1024)*4;						/* found 4Mb */
		if(s >= size)  return( (count << 22));	
	}
}

setlasterror(NO_MEM);
return(-1);
}

/*
* Load page tables for process
*
* In: size_t process		Process ID
*
* Returns NULL
* 
*/
size_t loadpagetable(size_t process) {
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
* Get physical address from virtual address
*
* In: size_t process		Process ID
	     uint32_t virtaddr	Virtual address
*
* Returns physical address or -1 on error
* 
*/
size_t getphysicaladdress(size_t process,uint32_t virtaddr) {
struct ppt *next;
uint32_t pagedir_entry_number;
uint32_t pagetable_entry_number;
uint32_t *pagetableptr;

next=processpaging;					/* find process page directory */

do {
	if(next->process == process) break;
		
	next=next->next;
} while(next != NULL);

pagedir_entry_number=(virtaddr >> 22) & 0x3ff;				/* page directory offset */
pagetable_entry_number=(virtaddr >> 12) & 0x3ff;				/* page table offset */

pagetableptr=(uint32_t *) 0xffc00000 + (pagedir_entry_number*1024);

if((pagetableptr[pagetable_entry_number] & PAGE_PRESENT) == 0) return(-1);	/* page not present */

return(pagetableptr[pagetable_entry_number] & 0xfffff000);				/* return physical address */
}
/*
* Add user page
*
* In: uint32_t page		Virtual address       
	     size_t process		Process ID
	     void *physaddr		Physical address

* Returns NULL
* 
*/
size_t addpage_user(uint32_t page,size_t process,void *physaddr) { 
return(addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}

/*
* Add system page
*
* In: uint32_t page		Virtual address       
	     size_t process		Process ID
	     void *physaddr		Physical address

* Returns NULL
* 
*/
size_t addpage_system(uint32_t page,size_t process,void *physaddr) { 
return(addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}


