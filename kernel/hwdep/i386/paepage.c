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
#include "page.h"
#include "hwdefs.h"
#include "errors.h"
#include "debug.h"
#include "memorymanager.h"
#include "string.h"

size_t PAGE_SIZE=4096;
size_t paging_type=2;

#define PDPT_LOCATION 0x2000

struct ppt {
	uint64_t pdpt[4] __attribute__((aligned(0x20)));			/* PDPT */
	size_t process; 							/* process ID */
	uint32_t pdptphys;							/* Physical address of PDPT */
	struct ppt *next;							/* pointer to next paging entry */
} __attribute__((packed)) *processpaging=PDPT_LOCATION+KERNEL_HIGH;

struct ppt *processpaging_end;

/*
* Add page
*
* In: 	mode		Allocation mode(ALLOC_KERNEL or ALLOC_NORMAL)
	process 	Process ID
	page		Virtual address
	physaddr	Physical address
*
* Returns 0 on success or -1 on error
* 
*/

size_t addpage_int(uint32_t mode,size_t process,uint32_t page,void *physaddr) { 
struct ppt *next;
size_t pdpt_entry_number;
size_t pagedir_entry_number;
size_t pagetable_entry_number;
uint64_t *pagetableptr;
uint64_t newpagedirentry;
uint32_t addr;

next=processpaging;					/* find process page directory */

do {	
	if(next->process == process) break;
	next=next->next;
} while(next != NULL);

//33222222222211111111110000000000
//10987654321098765432109876543210
//11000000011000000010000000000000

pdpt_entry_number=page >> 30;			/*  directory pointer table */
pagedir_entry_number=(page & 0x3FE00000) >> 21;
pagetable_entry_number=(page & 0x1FF000) >> 12;

/* if pdpt empty, add page directory to it, the page tables in the page directory will appear at 0xc0000000+(pagedir_entry_number*512) */

if(next->pdpt[pdpt_entry_number] == 0) {			/* if pdpt empty */
	newpagedirentry=kernelalloc_nopaging(PAGE_SIZE);
	if(newpagedirentry == NULL) {					/* allocation error */
		setlasterror(NO_MEM);
		return(-1);
	}


	next->pdpt[pdpt_entry_number]=(newpagedirentry & 0xfffff000) | PAGE_PRESENT;
}

pagetableptr=KERNEL_HIGH+(next->pdpt[pdpt_entry_number] & 0xfffff000);

if(pagetableptr[pagedir_entry_number] == 0) {				/* if page directory empty */
	newpagedirentry=kernelalloc_nopaging(PAGE_SIZE);

	if(newpagedirentry == NULL) {					/* allocation error */
		setlasterror(NO_MEM);
		return(-1);
	}

	pagetableptr[pagedir_entry_number]=newpagedirentry | PAGE_USER | PAGE_RW | PAGE_PRESENT;
}
	
/* page tables are mapped at 0xc0000000 via recursive paging */

pagetableptr=0xc0000000+(pdpt_entry_number << 21) | (pagedir_entry_number << 12);

addr=physaddr;

pagetableptr[pagetable_entry_number]=(addr & 0xfffff000)+mode;			/* page table */
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
struct ppt *p;
struct ppt *last;
uint32_t oldvirtaddress;
uint32_t virtaddress;
uint32_t oldpp;
uint32_t pp;
size_t count;
size_t size;

if(process == 0) return;		/* don't need to intialize if process 0 */

/* Get a physical address and manually map it to a virtual address. The physical addres of p->pagedir is needed to fill cr3 */

size=(sizeof(struct ppt) & ((0-1)-(PAGE_SIZE-1)))+PAGE_SIZE;		/* round */

pp=kernelalloc_nopaging(size);					/* add link */
if(pp == NULL) return(-1);

virtaddress=findfreevirtualpage(size,ALLOC_KERNEL,0);		/* find free virtual address */

oldvirtaddress=virtaddress;
oldpp=pp;

for(count=0;count<(size/PAGE_SIZE)+1;count++) {
	addpage_system(virtaddress,0,(void *) pp);

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

	addpage_system(pp+KERNEL_HIGH,0,(void *) pp);
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
	     size_t process	Process ID
*
* Returns NULL
* 
*/

size_t removepage(uint32_t page,size_t process) {
addpage_int(0,process,page,0);						/* clear page */
return(NULL);
}

/*
* Free address space
*
* In: process	Process ID
*
*/

size_t freepages(size_t process) {
struct ppt *next;
size_t count;
size_t countx;
size_t *pagetableptr;
struct ppt *last;
uint64_t *pagedir;
size_t pdptcount;

next=processpaging;

while(next != NULL) {					/* find process */
	if(next->process == process) break;		

	last=next;
	next=next->next;
}

pagetableptr=0xc0000000;

for(count=0;count != 512 ;count++) {				/* page directories */

	pagetableptr=(uint64_t)pagedir[count] & 0xfffffffffffff000;			/* point to page table */   

	if(pagetableptr != 0) {							/* in use */

		pagetableptr += KERNEL_HIGH;

		for(countx=0;countx<511;countx++) {
			if(pagetableptr[countx] != 0) {					/* free page table  */
			     kernelfree(pagetableptr[countx] & 0xfffffffffffff000);
			     removepage(pagetableptr[countx] & 0xfffffffffffff000,getpid());
			}	 
	  	}
	}
}

last->next=next->next;						/* entry before this entry will now point to the next entry,removing it from the chain */
kernelfree(next);						/* and free it */

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
struct ppt *next;
uint64_t *pagedirptr;
uint64_t *ptptr;
size_t pdcount;
size_t ptcount;
size_t last;
uint32_t pdptcount;

next=processpaging;

do {						/* find process struct */
	if(next->process == process) break;
	next=next->next;
} while(next != NULL);

/* walk through page directories and tables to find free pages in directories already used */

if(alloc == ALLOC_NORMAL) {
start=0;	
end=1;
}
else
{
start=2;
end=3;
}

/* search through mapped in page tables for a free entry */

last=0;

for(pdptcount=start;pdptcount<end;pdptcount++) {

	if(next->pdpt[pdptcount] != 0) {

			pagedirptr=0xc0000000+(pdptcount << 21);

			for(pdcount=0;pdcount<512;pdcount++) {			/* pdpts */     

		  		if(pagedirptr[pdcount] == 0) {
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

last=0;
s=0;

for(pdcount=start;pdcount<end;pdcount++) {			/* pdpts */     

	  	 if(next->pdpt[pdcount] != 0) {
			pagedirptr=KERNEL_HIGH+(next->pdpt[pdcount] & 0xfffff000);

			ptptr=KERNEL_HIGH+(next->pdpt[pdcount] & 0xfffff000);
		
			 for(ptcount=0;ptcount<512;ptcount++) {			/* pdpts */     
		 		if(ptptr[ptcount] == 0) {
	         			if(s == 0) last=ptcount;
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

/* search through pdpts for a free entry */

for(pdcount=start;pdcount != end;pdcount++) {			/* page directories */
	if(next->pdpt[pdcount] == 0) {
		s=s+(1024*1024*1024)*2;						/* found 2gb */

		if(s >= size) return(pdptcount << 30);
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

size_t loadpagetable(size_t process) {
struct ppt *next; 
next=processpaging;
size_t count;

/* Find process */

while(next != NULL) {

	if(next->process == process) {

		/* Clear unecessary bits in the PDPT, the accessed and dirty bits are set
		 * whenever they are accessed via recursive paging. 
		 * This can cause problems when loading cr3 with a PDPT so the bits must be cleared. */

		for(count=0;count<4;count++) {
			if(next->pdpt[count] != 0) next->pdpt[count]=(next->pdpt[count] & 0xfffffffffffff000) | PAGE_PRESENT;
		}

		asm volatile("mov %0, %%cr3":: "b"(next->pdptphys));		/* load cr3 with PDPT */
		return;
	}

	next=next->next;
}

return(-1);
}	

/*
* Get physical address from virtual address
*
* In: process		Process ID
      virtaddr	Virtual address
*
* Returns NULL
* 
*/
uint32_t getphysicaladdress(size_t process,uint32_t virtaddr) {
struct ppt *next;
uint64_t pdpt_entry_number;
uint64_t pagedir_entry_number;
uint64_t pagetable_entry_number;
uint64_t *pagetableptr;

next=processpaging;					/* find process page directory */

do {
	if(next->process == process) break;
		
	next=next->next;
} while(next->next != NULL);

pdpt_entry_number=(virtaddr & 0xC0000000) >> 30;			/*  directory pointer table */
pagedir_entry_number=(virtaddr & 0x3FE00000) >> 21;

pagetableptr=0xc0000000+(pdpt_entry_number << 21+(pdpt_entry_number*PAGE_SIZE))+(pagedir_entry_number << 12);

return(pagetableptr[pagetable_entry_number] & 0xfffff000);				/* return physical address */
}

/*
* Add user page
*
* In: page		Virtual address       
      process		Process ID
      physaddr		Physical address

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

