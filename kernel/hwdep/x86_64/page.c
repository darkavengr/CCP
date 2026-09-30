/* CCP Version 0.0.1
  (C) Matthew Boote 2020-2022

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
  along with CCP.  If not, see <https:www.gnu.org/licenses/>.
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

PAGING *processpaging=PML4_PHYS_LOCATION+KERNEL_HIGH;
PAGING *processpaging_end=NULL;


/* 64-bit paging

SSSSSSSSSSSSSSSSZZZZZZZZZXXXXXXXXXAAAAAAAAABBBBBBBBBCCCCCCCCCCCC

/* SSSSSSSSSSSSSSSS     ZZZZZZZZZ    XXXXXXXXX    AAAAAAAAA         BBBBBBBBB     CCCCCCCCCCCC
   Set to bit 48 of |   PML4 |    |  PDPT |    | page directory| | page table| |  4k page | 
   virtual address
*/

/*
* Map virtual address to physical address
*
* In: 	page_flags	Page flags
	virtual_address	Virtual address
	physical_address Physical address
*
* Returns 0 on success or -1 on error
* 
*/

size_t map_page_internal(size_t page_flags,size_t virtual_address,void *physical_address) { 
PAGING *next=NULL;
size_t pml4_number;
size_t pdpt_number;
size_t page_directory_number;
size_t page_table_number;
uint64_t *pml4ptr;
uint64_t *pdptptr;
uint64_t *pagedirptr;
uint64_t *pagetableptr;
uint64_t newpagedirentry;
uint64_t addr;
uint64_t *pageptr;

//kprintf_direct("***\n");
//kprintf_direct("paging=%X %X\n",virtual_address,physical_address);

pml4_number=(virtual_address >> 39) & 0x1FF;
pdpt_number=(virtual_address >> 30) & 0x1FF;
page_directory_number=(virtual_address >> 21) & 0x1FF;
page_table_number=(virtual_address >> 12) & 0x1FF;

pml4ptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) 0x1ff << 21) | ((uint64_t) 0x1ff << 12),47);
pdptptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) 0x1ff << 21) | ((uint64_t) pml4_number << 12),47);
pagedirptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) pml4_number << 21) | ((uint64_t) pdpt_number << 12),47);
pagetableptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) pml4_number << 30) | ((uint64_t) pdpt_number << 21) | ((uint64_t) page_directory_number << 12),47);

if(pml4ptr[pml4_number] == 0) {			/* if PML4 empty */
	newpagedirentry=kernelalloc_nopaging(PAGE_SIZE);
	if(newpagedirentry == NULL) {					/* allocation error */
		setlasterror(NO_MEM);
		return(-1);
	}
	
	pml4ptr[pml4_number]=(newpagedirentry & 0xfffffffffffff000) | PAGE_RW | PAGE_USER | PAGE_PRESENT;
}

if(pdptptr[pdpt_number] == 0) {					/* if PDPT empty */
	newpagedirentry=kernelalloc_nopaging(PAGE_SIZE);

	if(newpagedirentry == NULL) {					/* allocation error */
		setlasterror(NO_MEM);
		return(-1);
	}

	pdptptr[pdpt_number]=newpagedirentry | PAGE_USER | PAGE_RW | PAGE_PRESENT;
}

if(pagedirptr[page_directory_number] == 0) {				/* if page directory empty */
	newpagedirentry=kernelalloc_nopaging(PAGE_SIZE);

	if(newpagedirentry == NULL) {					/* allocation error */
		setlasterror(NO_MEM);
		return(-1);
	}
	
	pagedirptr[page_directory_number]=newpagedirentry | PAGE_USER | PAGE_RW | PAGE_PRESENT;
}
	
/* page tables are mapped at PAGETABLE_LOCATION via recursive paging */

addr=physical_address;

pageptr=((size_t) pagetableptr | (pml4_number << 30) | (pdpt_number << 21) | (page_directory_number << 12));
pageptr[page_table_number]=(size_t) (addr & 0xfffffffffffff000) | page_flags; /* map entry */

return(0);
}

/*
* Initialize process address space
*
* In: process	Process ID
*
* Returns 0 on success or -1 on failure
* 
*/

size_t page_init(size_t process) {
uint64_t oldvirtual_address;
uint64_t virtual_address;
uint64_t oldpp;
uint64_t pp;
size_t count;
size_t size;

if(process == 0) return(0);		/* don't need to intialize if process 0 */

/* Get a physical address and manually map it to a virtual address. The physical addres of p->pagedir is needed to fill cr3 */

size=round_up(sizeof(PAGING),PAGE_SIZE);	/* round up to PAGE _SIZE */

virtual_address=kernelalloc_page_aligned(size);
if(virtual_address == NULL) return(-1);

pp=getphysicaladdress(virtual_address);
if(pp == NULL) return(-1);


oldvirtual_address=virtual_address;
oldpp=pp;

for(count=0;count < (size/PAGE_SIZE)+1;count++) {
	map_system_page(virtual_address,(void *) pp);

	pp += PAGE_SIZE;
	virtual_address += PAGE_SIZE;
}

/* Important: make sure that is processpaging_end == processpaging. This most be done in the paging intialization code */
if(processpaging_end == NULL) processpaging_end=processpaging; /* do it anyway */

processpaging_end->next=oldvirtual_address;					/* add link to list */
processpaging_end=processpaging_end->next;		/* point to end */

processpaging_end->pml4phys=oldpp;			/* physical address of PML4 */
processpaging_end->process=process;

/* add bottom half PML4s */

for(count=0;count != 256;count++) {
	pp=kernelalloc_nopaging(PAGE_SIZE);
	if(pp == NULL) return(-1);

	processpaging_end->pml4[count]=pp | PAGE_PRESENT;

	map_system_page(pp+KERNEL_HIGH,(void *) pp);
}

processpaging_end->pml4[510]=processpaging->pml4[510];		/* copy top-half PML4 */
processpaging_end->pml4[511]=oldpp | PAGE_PRESENT;		/* map last PML4 to PML4 */
processpaging_end->next=NULL;

return(0);
}

/*
* Remove page
*
* In: page	Page to remove
*
* Returns NULL
* 
*/

size_t unmap_page(size_t page) {
map_page_internal(0,page,0);						/* clear page */
return(NULL);
}

/*
* Free address space
*
* In: process	Process ID
*
*/

size_t freepages(size_t process) {
PAGING *next;
size_t *pageptr;
PAGING *previous;
uint64_t *pagedir;
size_t count;
size_t pml4count;
size_t pdptcount;
size_t pdcount;
size_t ptcount;
uint64_t *pagedirptr=NULL;
uint64_t *pagetableptr=NULL;
PAGING *savenext;
PAGING *last;

next=processpaging;

while(next != NULL) {
	if(next->process == process) {
		for(pml4count=0;pml4count < 511;pml4count++) {

			for(pdptcount=0;pdptcount < 511;pdptcount++) {				/* page directories */
				for(pdcount=0;pdcount < 511;pdcount++) {				/* page directories */
					pagedirptr=signextend((0x1ff << 39) | (0x1ff << 30) | (pml4count << 21) | (pdptcount << 12),47);

					if(pagedirptr[pdcount] != 0) {							/* in use */
						pageptr=signextend((0x1ff << 39) | (pml4count << 30) | (pdptcount << 21) | (pdcount << 12),47);

						for(ptcount=0;ptcount < 511;ptcount++) {
							if(pageptr[ptcount] != 0) {					/* free page table  */
							     kernelfree(pageptr[ptcount] & 0xfffffffffffff000);
								     unmap_page(pageptr[ptcount] & 0xfffffffffffff000);
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

setlasterror(INVALID_PROCESS);
return(-1);
}

/*
* find free virtual address
*
* In:  size	Number of free bytes to find
       alloc	Allocation flags (ALLOC_KERNEL to find in top half of memory, ALLOC_NORMAL to find in bottom half)

* Returns address on success or -1 on failure
* 
*/

size_t findfreevirtualpage(size_t size,size_t alloc) {
size_t s;
size_t start;
size_t end;
PAGING *next=NULL;
uint64_t *pagedirptr=NULL;
uint64_t *pagetableptr=NULL;
uint64_t *pdptptr=NULL;
uint64_t *pml4ptr=NULL;
size_t pdcount;
size_t ptcount;
size_t last;
size_t pdptcount;
size_t pml4count;
size_t address;

/* walk through page directories and tables to find free pages in directories already used */

if(alloc == ALLOC_NORMAL) {
	start=0;	
	end=255;	
}
else
{
	start=510;
	end=511;
}
last=0;

for(pml4count=start;pml4count != end;pml4count++) {
	pdptptr=signextend((uint64_t) (((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) 0x1ff << 21) | ((uint64_t) pml4count << 12)),47);

	for(pdptcount=0;pdptcount < 511;pdptcount++) {
		if(pdptptr[pdptcount] != 0) {
			pagedirptr=signextend(((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) pml4count << 21) | ((uint64_t) pdptcount << 12)),47);

			for(pdcount=0;pdcount < 511;pdcount++) {
		  		if(pagedirptr[pdcount] != 0) {
					pagetableptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) pml4count << 30) | ((uint64_t) pdptcount << 21) | ((uint64_t) pdcount << 12),47);

					s=0;

					for(ptcount=0;ptcount < 511;ptcount++) {   
						if(pagetableptr[ptcount] == 0) {

        						if(s == 0) last=ptcount;

							if(s >= size) {
								address=signextend(((uint64_t) ((uint64_t) pml4count << 39)+((uint64_t) pdptcount << 30)+((uint64_t) pdcount << 21)+((uint64_t) last << 12)),47);
								return((uint64_t) address);
							}
	 	  					
							s += PAGE_SIZE;		
						}
						else
						{
							last=ptcount;
							s=0;				/* must be contiguous */
	    					}
	  				}
				}				
		 	}
		}
	}
}

/* search through PDPTs */

for(pml4count=start;pml4count<end;pml4count++) {
	pdptptr=signextend(((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) 0x1ff << 21) | ((uint64_t) pml4count << 12)),47);

	for(pdptcount=0;pdptcount<511;pdptcount++) {
		if(pdptptr[pdptcount] == 0) {
			s += 0x8000000000;		/* + 512GB */
	
			address=signextend((uint64_t) ((uint64_t) pml4count << 39) | ((uint64_t) pdptcount << 30),47);

		       	if(s >= size) return(address);	 /* found enough */
	 	}
	}
}

/* search through pml4s */

pml4ptr=signextend(((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) 0x1ff << 21) | ((uint64_t) 0x1ff << 12)),47);

for(pml4count=0;pml4count<511;pml4count++) {
	if(pml4ptr[pml4count] == 0) {
		s += 0x1000000000000;
	
		address=signextend(((uint64_t) pml4count << 39),47);		/* + 256TB */

	       	if(s >= size) return(address); /* found enough */	         
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
size_t count;

while(next != NULL) {
	if(next->process == process) {
		asm volatile("mov %0, %%cr3":: "b"(next->pml4phys));		/* load cr3 with PML4 */
		return(0);
	}

	next=next->next;
}

return(-1);
}	

/*
* Get page table entry for virtual address
*
* In: virtual_address   Virtual address
*
* Returns 0 on success or -1 on failutr
* 
*/
size_t get_pagetable_entry(size_t virtual_address) {
PAGING *next;
size_t pml4_number;
size_t pdpt_number;
size_t page_directory_number;
size_t page_table_number;
uint64_t *pagetableptr;
uint64_t *pageptr;

pml4_number=((virtual_address & 0xFF8000000000) >> 39);
pdpt_number=((virtual_address & 0x7FC0000000) >> 30);
page_directory_number=((virtual_address & 0x3FE00000) >> 21);
page_table_number=((virtual_address & 0x1FF000) >> 12);

pagetableptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) pml4_number << 30) | ((uint64_t) pdpt_number << 21) | ((uint64_t) page_directory_number << 12),47);
pageptr=((size_t) pagetableptr | (pml4_number << 30) | (pdpt_number << 21) | (page_directory_number << 12));

return(pageptr[page_table_number]);				/* return page table entry */
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
uint64_t getphysicaladdress(size_t virtual_address) {
size_t entry=get_pagetable_entry(virtual_address);

if(entry == -1) return(-1);

return(entry & 0xfffffffffffff000);				/* return physical address */
}

/*
* Map user page
*
* In: page		Virtual address       
      process		Process ID
      physaddr		Physical address

* Returns 0 on success, -1 on error
* 
*/

size_t map_user_page(size_t page,void *physaddr) { 
return(map_page_internal(PAGE_USER | PAGE_RW | PAGE_PRESENT,page,physaddr));
}

/*
* Map system page
*
* In: page		Virtual address       
      physaddr		Physical address

* Returns 0 on success, -1 on error
* 
*/
size_t map_system_page(size_t page,void *physaddr) { 
return(map_page_internal(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,page,physaddr));
}

void set_initial_process_paging_end(void) {
processpaging_end=processpaging;
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

