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
#include "page.h"
#include "hwdefs.h"
#include "errors.h"
#include "debug.h"
#include "memorymanager.h"

size_t signextend(size_t num,size_t bitnum);

size_t PAGE_SIZE=4096;

struct ppt {
	uint64_t pml4[512];							/* PML4 */
	uint64_t pml4phys;							/* Physical address of PML4 */
	size_t process; 							/* process ID */
	struct ppt *next;							/* pointer to next paging entry */
} *processpaging=PML4_PHYS_LOCATION+KERNEL_HIGH;

struct ppt *processpaging_end=NULL;


/* 64-bit paging

SSSSSSSSSSSSSSSSZZZZZZZZZXXXXXXXXXAAAAAAAAABBBBBBBBBCCCCCCCCCCCC

/* SSSSSSSSSSSSSSSS     ZZZZZZZZZ    XXXXXXXXX    AAAAAAAAA         BBBBBBBBB     CCCCCCCCCCCC
   Set to MSB of PML4  |   PML4 |    |  PDPT |    | page directory| | page table| |  4k page |  */

/*
* Add page
*
* In: 	mode		Page flags
	process 	Process ID
	page		Virtual address
	physaddr	Physical address
*
* Returns 0 on success or -1 on error
* 
*/

size_t addpage_int(size_t mode,size_t process,size_t page,void *physaddr) { 
struct ppt *next;
size_t pml4_of;
size_t pdpt_of;
size_t pd;
size_t pt;
size_t p;
uint64_t *pml4ptr;
uint64_t *pdptptr;
uint64_t *pdptr;
uint64_t *ptptr;
uint64_t c;
uint64_t addr;
uint64_t *v;

next=processpaging;					/* find process page directory */

do {	
	if(next->process == process) break;

	next=next->next;
} while(next != NULL);

if(next == NULL) {
	setlasterror(INVALID_PROCESS);
	return(-1);
}

kprintf_direct("**********************\n");


pml4_of=((page & 0xFF8000000000) >> 39);
pdpt_of=((page & 0x7FC0000000) >> 30);
pd=((page & 0x3FE00000) >> 21);
pt=((page & 0x1FF000) >> 12);

pml4ptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) 0x1ff << 21) | ((uint64_t) 0x1ff << 12),47);
pdptptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) 0x1ff << 21) | ((uint64_t) pml4_of << 12),47);
pdptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) pml4_of << 21) | ((uint64_t) pdpt_of << 12),47);
ptptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) pml4_of << 30) | ((uint64_t) pdpt_of << 21) | ((uint64_t) pd << 12),47);

//DEBUG_PRINT_HEX_LONG(page);
//DEBUG_PRINT_HEX_LONG(physaddr);
//DEBUG_PRINT_HEX_LONG(pml4_of);
//DEBUG_PRINT_HEX_LONG(pdpt_of);
//DEBUG_PRINT_HEX_LONG(pd);
//DEBUG_PRINT_HEX_LONG(pt);

if(pml4ptr[pml4_of] == 0) {			/* if pml4 empty */
	c=kernelalloc_nopaging(PAGE_SIZE);
	if(c == NULL) {					/* allocation error */
		setlasterror(NO_MEM);
		return(-1);
	}
	
//	kprintf_direct("pml4 entry (pdpt) physical address=%lX\n",c);
	pml4ptr[pml4_of]=(c & 0xfffffffffffff000) | PAGE_PRESENT;

//	if(c ==  0x2009000) asm("xchg %bx,%bx");

//	kprintf_direct("pml4 virtual address=%lX\n",&pml4ptr[pml4_of]);
}

//kprintf_direct("pdpt virtual address=%lX\n",&pdptptr[pdpt_of]);

if(pdptptr[pdpt_of] == 0) {					/* if pdpt empty */
	c=kernelalloc_nopaging(PAGE_SIZE);

//	kprintf_direct("pdpt physical address=%lX\n",c);

	if(c == NULL) {					/* allocation error */
		setlasterror(NO_MEM);
		return(-1);
	}

	pdptptr[pdpt_of]=c | PAGE_USER | PAGE_RW | PAGE_PRESENT;

//	if(c ==  0x2009000) asm("xchg %bx,%bx");
}

//kprintf_direct("page directory virtual address=%lX\n",&pdptr[pdpt_of]);

if(pdptr[pd] == 0) {				/* if page directory empty */
	c=kernelalloc_nopaging(PAGE_SIZE);

	if(c == NULL) {					/* allocation error */
		setlasterror(NO_MEM);
		return(-1);
	}
	
	kprintf_direct("page directory physical address=%lX\n",c);

	pdptr[pd]=c | PAGE_USER | PAGE_RW | PAGE_PRESENT;

//	if(c ==  0x2009000) asm("xchg %bx,%bx");
}
	
/* page tables are mapped at PAGETABLE_LOCATION via recursive paging */

addr=physaddr;

v=((size_t) ptptr | (pml4_of << 30) | (pdpt_of << 21) | (pd << 12));

v[pt]=(size_t) (addr & 0xfffffffffffff000)+mode;			/* page table */

//if(physaddr ==  0x2009000) asm("xchg %bx,%bx");

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
struct ppt *p;
struct ppt *last;
uint64_t oldvirtaddress;
uint64_t virtaddress;
uint64_t oldpp;
uint64_t pp;
size_t count;
size_t size;

if(process == 0) return(0);		/* don't need to intialize if process 0 */

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
processpaging_end->pml4phys=oldpp;			/* physical address of PML4 */
processpaging_end->process=process;

/* add bottom half PML4s */

for(count=0;count != 256;count++) {
	pp=kernelalloc_nopaging(PAGE_SIZE);
	if(pp == NULL) return(-1);

	last->pml4[count]=pp | PAGE_PRESENT;

	addpage_system(pp+KERNEL_HIGH,0,(void *) pp);
}

last->pml4[510]=processpaging->pml4[510];		/* copy top-half PML4 */
last->pml4[511]=oldpp | PAGE_PRESENT;		/* map last PML4 to PML4 */

last->next=NULL;
return(0);
}

/*
* Remove page
*
* In: uint64_t page	Page to remove
	     size_t process	Process ID
*
* Returns NULL
* 
*/

size_t removepage(size_t page,size_t process) {
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
size_t *v;
struct ppt *last;
uint64_t *pagedir;
size_t count;
size_t pml4count;
size_t pdptcount;
size_t pdcount;
size_t ptcount;
uint64_t *pdptr;
uint64_t *ptptr;

next=processpaging;
;
while(next != NULL) {					/* find process */
	if(next->process == process) break;		

	last=next;
	next=next->next;
}

if(next == NULL) {
	setlasterror(INVALID_PROCESS);
	return(-1);
}

for(pml4count=0;pml4count != 512;pml4count++) {
	for(pdptcount=0;pdptcount != 512;pdptcount++) {				/* page directories */
		for(pdcount=0;pdcount != 512;pdcount++) {				/* page directories */
			pdptr=signextend((0x1ff << 39) | (0x1ff << 30) | (pml4count << 21) | (pdptcount << 12),47);

			if(pdptr[pdcount] != 0) {							/* in use */
				v=signextend((0x1ff << 39) | (pml4count << 30) | (pdptcount << 21) | (pdcount << 12),47);

				for(ptcount=0;ptcount<511;ptcount++) {
					if(v[ptcount] != 0) {					/* free page table  */
					     kernelfree(v[ptcount] & 0xfffffffffffff000);
					     removepage(v[ptcount] & 0xfffffffffffff000,getpid());
					}
				}	 
		  	}
		}
	}
}

last->next=next->next;						/* entry before this entry will now point to the next entry,removing it from the chain */
kernelfree(next);						/* and free it */

return(0);
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
uint64_t *pdptr;
uint64_t *ptptr;
uint64_t *pdptptr;
uint64_t *pml4ptr;
size_t pdcount;
size_t ptcount;
size_t last;
size_t pdptcount;
size_t pml4count;
size_t address;
size_t countx;

next=processpaging;

do {
	if(next->process == process) break;

	next=next->next;
} while(next != NULL);

if(next == NULL) {
	setlasterror(INVALID_PROCESS);
	return(-1);
}

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

//kprintf_direct("findfreevirtualaddress() pml4ptr=%lX\n",next->pml4);

for(pml4count=start;pml4count != end;pml4count++) {

//	DEBUG_PRINT_HEX_LONG(next->pml4[pml4count]);

	if(next->pml4[pml4count] != 0) {
		pdptptr=signextend((uint64_t) (((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) 0x1ff << 21) | ((uint64_t) pml4count << 12)),47);

//		kprintf_direct("findfreevirtualaddress() pdptptr=%lX\n",pdptptr);
		//asm("xchg %bx,%bx");

		for(pdptcount=0;pdptcount != 511;pdptcount++) {

			if(pdptptr[pdptcount] != 0) {

				pdptr=signextend(((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) pml4count << 21) | ((uint64_t) pdptcount << 12)),47);

				//asm("xchg %bx,%bx");

				for(pdcount=0;pdcount<511;pdcount++) {			/* pdpts */     

					//kprintf_direct("findfreevirtualaddress() pdptr=%lX\n",pdptr);

			  		if(pdptr[pdcount] != 0) {
			
//						kprintf_direct("pt offsets=%lX %lX %lX %lX\n",pml4count,pdptcount,pdcount,last);

						ptptr=signextend((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) pml4count << 30) | ((uint64_t) pdptcount << 21) | ((uint64_t) pdcount << 12),47);

						//kprintf_direct("findfreevirtualaddress() ptptr=%lX\n",ptptr);

						s=0;

						for(ptcount=0;ptcount != 511;ptcount++) {   
   
						//	kprintf_direct("ptptr[%lX]=%lX\n",ptcount,ptptr[ptcount]);

							if(ptptr[ptcount] == 0) {

	        						if(s == 0) last=ptcount;

								if(s >= size) {
								//	kprintf_direct("pt free=%lX %lX %lX %lX\n",pml4count,pdptcount,pdcount,last);

									address=signextend(((uint64_t) ((uint64_t) pml4count << 39)+((uint64_t) pdptcount << 30)+((uint64_t) pdcount << 21)+((uint64_t) last << 12)),47);
								
								//	kprintf_direct("find free virtual address=%lX\n",address);		
								//	asm("xchg %bx,%bx");

									return((uint64_t) address);
								}
	 	  					
								s += PAGE_SIZE;		
	     						}
	    						else
	    						{
	     							last=countx;
	     							s=0;				/* must be contiguous */
	    						}
	  					}

						//asm("xchg %bx,%bx");
					}				
		 		}
			}
		}
	}
}

//kprintf_direct("NO FREE PAGE TABLE ENTRY FOUND\n");

/* search through PDPTs */

for(pml4count=start;pml4count<end;pml4count++) {
	pdptptr=signextend(((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) 0x1ff << 21) | ((uint64_t) pml4count << 12)),47);

	for(pdptcount=0;pdptcount<511;pdptcount++) {
		if(pdptptr[pdptcount] == 0) {
			s += 0x8000000000;		/* + 512GB */
	
			address=signextend((uint64_t) ((uint64_t) pml4count << 39) | ((uint64_t) pdptcount << 30),47);

//			kprintf_direct("pdpt findfreevirtualaddress() address=%lX\n",address);

		       	if(s >= size) return(address);	 /* found enough */	         
	 	}
	}
}

////kprintf_direct("NO FREE PDPT ENTRY FOUND\n");

/* search through pml4s */

pml4ptr=signextend(((uint64_t) ((uint64_t) 0x1ff << 39) | ((uint64_t) 0x1ff << 30) | ((uint64_t) 0x1ff << 21) | ((uint64_t) 0x1ff << 12)),47);

for(pml4count=0;pml4count<511;pml4count++) {
	if(pml4ptr[pml4count] == 0) {
		s += 0x1000000000000;
	
		address=signextend(((uint64_t) pml4count << 39),47);		/* + 256TB */

//		kprintf_direct("pml4 findfreevirtualaddress() address=%lX\n",address);

	       	if(s >= size) return(address); /* found enough */	         
 	}
	
}

//kprintf_direct("NO FREE PML4 ENTRY FOUND\n");

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

while(next != NULL) {
	if(next->process == process) {
//		 DEBUG_PRINT_HEX_LONG(&next->pml4phys);

		asm volatile("mov %0, %%cr3":: "b"(next->pml4phys));		/* load cr3 with PML4 */
		return(0);
	}

	next=next->next;
}

return(-1);
}	

/*
* Get physical address from virtual address
*
* In: process		Process ID
      vidrtaddr	Virtual address
*
* Returns NULL
* 
*/
uint64_t getphysicaladdress(size_t process,uint64_t virtaddr) {
struct ppt *next;
size_t pml4_of;
size_t pdpt_of;
size_t pd;
size_t pt;
uint64_t *p;
uint64_t *v;

next=processpaging;					/* find process page directory */

do {
	if(next->process == process) {
		pml4_of=((virtaddr & 0xFF8000000000) >> 39);
		pdpt_of=((virtaddr & 0x7FC0000000) >> 30);
		pd=((virtaddr & 0x3FE00000) >> 21);

		v=signextend((0x1ff << 39) | (pml4_of << 30) | (pdpt_of << 21) | (pd << 12),47);

		return(v[pt] & 0xfffffffffffff000);				/* return physical address */
	}
		
	next=next->next;
} while(next->next != NULL);

return(-1);
}

/*
* Add user page
*
* In: page		Virtual address       
      process		Process ID
      physaddr		Physical address

* Returns 0 on success, -1 on error
* 
*/

size_t addpage_user(uint64_t page,size_t process,void *physaddr) { 
return(addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}

/*
* Add system page
*
* In: uint64_t page		Virtual address       
	     size_t process		Process ID
	     void *physaddr		Physical address

* Returns 0 on success, -1 on error
* 
*/
size_t addpage_system(uint64_t page,size_t process,void *physaddr) { 
return(addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}


/*
 * Sign-extends number
 *
 * In:	number				Number to sign-extend
 *	bitnum				Bit number to sign-extend from
 */
size_t signextend(size_t num,size_t bitnum) {
size_t signextendnum;
size_t signextendcount;

signextendnum=(size_t) ((size_t) num & (((size_t) 1 << bitnum)));

for(signextendcount=0;signextendcount != (sizeof(num)*8)-bitnum;signextendcount++) {
	num |= ((size_t) signextendnum);

	signextendnum=((size_t) signextendnum << 1);
}

return((size_t) num);
}
