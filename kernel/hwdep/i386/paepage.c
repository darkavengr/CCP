/*  CCP Version 0.0.1
	  (newpagedirentry) Matthew Boote 2020-2022

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
/* PAE paging

XXAAAAAAAAABBBBBBBBBCCCCCCCCCCCC

/* XX     AAAAAAAAA          BBBBBBBBB	    	    CCCCCCCCCCCC
   PDPT	| page 	directory| | page table|	    | 4k page  |  */
/* 1GB	       2MB		4KB */

//PDPT=11		0x3
//pagedir_entry_number=000000000		0x0
//pagetable_entry_number= 110011110		0x19E
//PAGE= 110011110100	0xCF4

//33222222222211111111110000000000
//10987654321098765432109876543210
//11000000011000000000000000000000

#include <stdint.h>
#include <stddef.h>
#include "kernelhigh.h"
#include "pagesize.h"
#include "paepage.h"
#include "hwdefs.h"
#include "errors.h"
#include "debug.h"
#include "memorymanager.h"
#include "string.h"

size_t paging_type=PAGING_TYPE_PAE;

/*
* Add page
*
* In: 	page_flags	Page flags
	page		Virtual address
	physaddr	Physical address
*
* Returns 0 on success or -1 on error
* 
*/

size_t map_page_internal(uint32_t page_flag,uint32_t virtual_address,void *physical_address) { 
PAEPAGING *next;
size_t pdpt_entry_number;
size_t pagedir_entry_number;
size_t pagetable_entry_number;
uint64_t *pagetableptr;
uint64_t newpagedirentry;
uint32_t addr;
uint64_t *pdptptr;

//33222222222211111111110000000000
//10987654321098765432109876543210
//11000000011000000010000000000000

pdpt_entry_number=virtual_address >> 30;			/*  directory pointer table */
pagedir_entry_number=(virtual_address & 0x3FE00000) >> 21;
pagetable_entry_number=(virtual_address & 0x1FF000) >> 12;

pdptptr=PDPT_ADDRESS;

/* if pdpt empty, add page directory to it, the page tables in the page directory will appear at PDPT_LOCATION+(pagedir_entry_number*512) */

if(pdptptr[pdpt_entry_number] == 0) {			/* if pdpt empty */
	newpagedirentry=kernelalloc_nopaging(PAGE_SIZE);
	if(newpagedirentry == NULL) {					/* allocation error */
		setlasterror(NO_MEM);
		return(-1);
	}


	pdptptr[pdpt_entry_number]=(newpagedirentry & 0xfffff000) | PAGE_PRESENT;
}

pagetableptr=KERNEL_HIGH+(pdptptr[pdpt_entry_number] & 0xfffff000);

if(pagetableptr[pagedir_entry_number] == 0) {				/* if page directory empty */
	newpagedirentry=kernelalloc_nopaging(PAGE_SIZE);

	if(newpagedirentry == NULL) {					/* allocation error */
		setlasterror(NO_MEM);
		return(-1);
	}

	pagetableptr[pagedir_entry_number]=newpagedirentry | PAGE_USER | PAGE_RW | PAGE_PRESENT;
}
	
/* page tables are mapped at PDPT_LOCATION via recursive paging */

pagetableptr=PDPT_LOCATION+(pdpt_entry_number << 21) | (pagedir_entry_number << 12);

addr=physical_address;

pagetableptr[pagetable_entry_number]=(addr & 0xfffff000)+page_flags;			/* page table */

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
PAEPAGING *p;
PAEPAGING *last;
uint32_t oldvirtaddress;
uint32_t virtaddress;
uint32_t oldpp;
uint32_t pp;
size_t count;
size_t size;

if(process == 0) return;		/* don't need to intialize if process 0 */

/* Get a physical address and manually map it to a virtual address. The physical addres of p->pagedir is needed to fill cr3 */

size=round_up(size,PAGE_SIZE);	/* round up to PAGE _SIZE */

pp=kernelalloc_nopaging(size);					/* add link */
if(pp == NULL) return(-1);

virtaddress=findfreevirtualpage(size,ALLOC_KERNEL,0);		/* find free virtual address */

oldvirtaddress=virtaddress;
oldpp=pp;

for(count=0;count<(size/PAGE_SIZE)+1;count++) {
	map_system_page(virtaddress,0,(void *) pp);

	pp += PAGE_SIZE;
	virtaddress += PAGE_SIZE;
}

/* Important: make sure that is processpaging_end == processpaging. This most be done in the paging intialization code */

if(processpaging_end == NULL) processpaging_end=processpaging; /* do it anyway */

processpaging_end->next=oldvirtaddress;					/* add link to list */
processpaging_end=processpaging_end->next;		/* point to end */
processpaging_end->pagedirphys=oldpp;						/* physical address of page directory */
processpaging_end->process=process;

/* add bottom half PDPTs */

for(count=0;count != 2;count++) {
	pp=kernelalloc_nopaging(PAGE_SIZE);
	if(pp == NULL) return(-1);

	last->pdpt[count]=pp | PAGE_PRESENT;

	map_system_page(pp+KERNEL_HIGH,0,(void *) pp);
}

last->pdpt[2]=processpaging->pdpt[2];		/* copy top-half PDPT */
last->pdpt[3]=oldpp | PAGE_PRESENT;		/* map last PDPT to PDPT */

last->next=NULL;
return;
}
/*
* Remove page
*
* In: uint32_t page	Page to remove
*
* Returns NULL
* 
*/

size_t unmap_page(uint32_t page) {
map_page_internal(0,page,0);
return(NULL);
}

/*
* Free address space
*
* In: process	Process ID
*
*/

size_t freepages(size_t process) {
PAEPAGING *next;
size_t *pagetableptr;
PAEPAGING *last;
uint64_t *pagedir;
size_t pdptcount;
size_t pagedircount;
size_t pagetablecount;
PAEPAGING *savenext;

next=processpaging;
last=next;

while(next != NULL) {					/* find process */

	if(next->next->process == process) {
		previous=next;

		for(pdptcount=0;pdptcount < 4;pdptcount++) {
			pagedir=next->pdpt[pdptcount];

			if(pagedir != 0) {
				for(pagedircount=0;pagedircount != 512;pagedircount++) {				/* page directories */
					pagetableptr=(uint64_t)pagedir[pagedircount] & 0xfffffffffffff000;			/* point to page table */   

						if(pagetableptr != 0) {							/* in use */
							pagetableptr += KERNEL_HIGH;

							for(pagetablecount=0;pagetablecount < 511;pagetablecount++) {
								if(pagetableptr[pagetablecount] != 0) {
								     free_physical(pagetableptr[pagetablecount] & 0xfffffffffffff000);
								}
							}
						}
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

			last->next=NULL;

			remove_processpaging_entry(savenext);
		}
		else					/* middle */
		{
			savenext=next;
			last->next=next->next;

			remove_processpaging_entry(savenext);
		}

		return(0);
	}

	
	next=next->next;
}


return(NULL);
}

/*
* find free virtual address
*
* In:  size	Number of free bytes to find
       alloc	Allocation flags (ALLOC_KERNEL to find in top half of memory, ALLOC_NORMAL to find in bottom half)
       process	Process ID

* Returns NULL
* 
*/

size_t findfreevirtualpage(size_t size,size_t alloc,size_t process) {
size_t s;
size_t start;
size_t end;
PAEPAGING *next=NULL;
uint64_t *pdptptr=NULL;
uint64_t *pagedirptr=NULL;
uint64_t *ptptr=NULL;
size_t pdcount;
size_t ptcount;
size_t last;
size_t pdptcount;

if(alloc == ALLOC_NORMAL) {
	start=0;	
	end=1;
}
else
{
	start=2;
	end=3;
}

/* search through mapped in page tables for free entries */

last=0;

for(pdptcount=start;pdptcount < end;pdptcount++) {
	pagedirptr=PDPT_LOCATION+(pdptcount << 21);

	for(pdcount=0;pdcount < 512;pdcount++) {
		ptptr=pagedirptr[pdcount];

		for(ptcount=0;ptcount < 512;ptcount++) {	
	  		if(ptptr[pdcount] == 0) {
        			if(s == 0) last=pdcount;
        			if(s >= size) return((pdptcount << 30)+(last << 12)); /* found enough */	       		

				s=s+PAGE_SIZE;
		 	}
		 	else
		 	{
		        	s=0;				/* must be contiguous */
    				last=pdcount;
      		 	}
		}
	}
}


/* search through page directories for a free entry */

last=0;
s=0;

for(pdptcount=start;pdptcount < end;pdptcount++) {
	pagedirptr=PDPT_LOCATION+(pdptcount << 21);

	for(pdcount=0;pdcount < 512;pdcount++) {
		if(pagedirptr[pdptcount] != 0) {
			if(s == 0) {
				last=ptcount;
		 		s=s+(1024*2);
			}
			else
			{
		 		s=0;
        	       		last=pdcount;
			}

        		if(s >= size) return((pdcount << 30)+(last << 21)); /* found enough */	         
		}
	}
}

/* search through PDPTs for a free entry */

for(pdcount=start;pdcount != end;pdcount++) {			/* page directories */
	if(update->pdpt[pdcount] == 0) {
		s=s+(1024*1024*1024)*2;						/* found 2gb */

		if(s >= size) {
			if(process != getpid()) current_page_mapping->pdpt[3]=savemapping;	/* restore original mapping */

			return(pdptcount << 30);
		}
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
PAEPAGING *next; 
next=processpaging;
size_t count;

/* Find process */

while(next != NULL) {

	if(next->process == process) {

		/* Clear unecessary bits in the PDPT, the accessed and dirty bits are set
		 * whenever they are accessed via recursive paging. 
		 * This can cause problems when loading cr3 with a PDPT so the bits must be cleared. */

		for(count=0;count < 4;count++) {
			if(next->pdpt[count] != 0) next->pdpt[count]=(next->pdpt[count] & 0xfffffffffffff000) | PAGE_PRESENT;
		}

		asm volatile("mov %0, %%cr3":: "b"(next->pdptphys));		/* load cr3 with PDPT */
		return;
	}

	next=next->next;
}

return(-1);
}	

uint32_t get_page_table_entry(uint32_t virtual_address) {
PAEPAGING *next;
uint64_t pdpt_entry_number;
uint64_t pagedir_entry_number;
uint64_t pagetable_entry_number;
uint64_t *pagetableptr;
uint64_t address;

pdpt_entry_number=(virtual_address & 0xC0000000) >> 30;			/*  directory pointer table */
pagedir_entry_number=(virtual_address & 0x3FE00000) >> 21;

pagetableptr=PDPT_LOCATION | (pdpt_entry_number << 21 | (pdpt_entry_number*PAGE_SIZE)) | (pagedir_entry_number << 12);
return(pagetableptr[pagetable_entry_number]);
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
size_t getphysicaladdress(uint32_t virtual_address) {
size_t entry=get_pagetable_entry(virtual_address);

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
size_t map_user_page(uint32_t virtual_address,void *physical_address) {
return(map_page_internal(PAGE_USER+PAGE_RW+PAGE_PRESENT,virtual_address,physical_address));
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
size_t map_system_page(uint32_t virtual_address,void *physical_address) { 
return(map_page_internal(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,virtual_address,physical_address));
}

PAEPAGING *GetProcessPagingPointer(void) {
return(processpaging);
}

/*
* Get page flags from virtual address
*
* In:  process		Process ID
       virtual_address	Virtual address
*
* Returns physical address or -1 on error
* 
*/
size_t get_page_flags(uint32_t virtual_address) {
size_t entry=get_pagetable_entry(virtual_address);

if(entry == -1) return(-1);

return(entry & 0xfff);				/* return page flags */
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

