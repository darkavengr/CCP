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
/* PAE paging

XXAAAAAAAAABBBBBBBBBCCCCCCCCCCCC

/* XX     AAAAAAAAA        BBBBBBBBB	    	    CCCCCCCCCCCC
   PDPT	| page 	directory| | page table|	    | 4k page  |  */
/*	  1GB			2MB			4KB */

#include <stdint.h>
#include <stddef.h>
#include "page.h"
#include "hwdefs.h"
#include "../../header/errors.h"

extern end(void);
extern kernel_begin(void);
extern MEM_SIZE;

unsigned int PAGE_SIZE=4096;
unsigned int paging_type=2;

#define KERNEL_HIGH (1 << (sizeof(unsigned int)*8)-1)

unsigned int addpage_int(uint32_t mode,size_t process,uint32_t page,void *physaddr); 
unsigned int page_init(size_t process);
unsigned int removepage(uint32_t page,size_t process);
unsigned int freepages(size_t process);
unsigned int findfreevirtualpage(unsigned int size,unsigned int alloc,size_t process);
unsigned int loadpagetable(size_t process);
uint64_t getphysicaladdress(size_t process,uint32_t virtaddr);
unsigned int addpage_user(uint32_t page,size_t process,void *physaddr); 
unsigned int addpage_system(uint32_t page,size_t process,void *physaddr); 

#define PDPT_LOCATION 0x5000

struct ppt {
 uint64_t pdpt[4] __attribute__((aligned(0x20)));
 size_t process; 
 uint32_t pdptphys;
 struct ppt *next;
} *processpaging=PDPT_LOCATION+KERNEL_HIGH;
		

/* add page */


unsigned int addpage_int(uint32_t mode,size_t process,uint32_t page,void *physaddr) { 
 struct ppt *next;
 struct ppt *remap;
 unsigned int pdpt_of;
 unsigned int pd;
 unsigned int pt;
 uint64_t *v;
 uint64_t c;

// asm("xchg %bx,%bx");

 next=processpaging;					/* find process page directory */

 do {	
  if(next->process == process) break;

  next=next->next;
 } while(next != NULL);

//33222222222211111111110000000000
//10987654321098765432109876543210

//00111111111111111111111111111111
//00111111111000000000000000000000
//00000000000111111111000000000000

 pdpt_of=page >> 30;			/*  directory pointer table */

 pd=(page & 0x3FE00000) >> 21;
 pt=(page & 0x1FF000) >> 12;

 if(next->pdpt[pdpt_of] == 0) {	/* if pdpt empty */
  c=kernelalloc_nopaging(sizeof(struct ppt));

  if(c == -1) {					/* allocation error */
   setlasterror(NO_MEM);
   return(-1);
  }
 
 next->pdpt[pdpt_of]=c | PAGE_PRESENT;
}

asm("xchg %bx,%bx");

v=0xc0000000+(pdpt_of*512);
if(v[pd] == 0) {				/* if page directory empty */

 c=kernelalloc_nopaging(PAGE_SIZE);

 if(c == -1) {					/* allocation error */
  setlasterror(NO_MEM);
  return(-1);
 }

 v[pd]=c+(PAGE_USER+PAGE_RW+PAGE_PRESENT);	/* page directory entry */
}

/* page tables are mapped at 0xc0000000 via recursive paging */
 
v=0xc0000000+(pd*512);
v[pt]=((unsigned int) physaddr)+mode;			/* page table */


}

/* intialize address space */

unsigned int page_init(size_t process) {
 struct ppt *p;
 uint32_t virtualaddress;
 uint32_t haha;
 void *pp;
 void *ppage;
 uint64_t ppx;
 unsigned int count;
 p=processpaging;							/* find process */
 struct ppt *temppp;

 while(p->next != NULL) p=p->next;

/* get a physical address and manually map it to a virtual address because the physical addres of p->pagedir
   is needed to fill cr3 */

 pp=kernelalloc_nopaging(sizeof(struct ppt));					/* add link */
 if(pp == -1) return(-1);
 
 virtualaddress=findfreevirtualpage(sizeof(struct ppt),ALLOC_KERNEL,0);		/* find free virtual address */
 if(virtualaddress == -1) {
  kernelfree(pp);
  return(-1);
 }

 haha=virtualaddress;
 ppage=pp;

 for(count=0;count<sizeof(struct ppt)+PAGE_SIZE;count=count+PAGE_SIZE) {
  addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,0,haha,ppage);

  haha=haha+PAGE_SIZE;
  ppage=ppage+PAGE_SIZE;
 }

 p=virtualaddress;
 p->next=NULL;
 p->process=process;
 p->pdptphys=pp;

 memcpy(p->pdpt,(511*8));			/* copy pdpt */

 p->pdpt[512]=p->pdptphys | PAGE_RW | PAGE_PRESENT;	/* map pdpts recursively*/

 asm volatile("mov %0, %%cr3":: "b"(p->pdptphys));

 return(NULL);
}


/* remove page */
unsigned int removepage(uint32_t page,size_t process) {
 addpage_int(0,process,page,0);						/* clear page */
 return(NULL);
}

unsigned int freepages(size_t process) {
 struct ppt *next;
 unsigned int count;
 unsigned int countx;
 unsigned int *v;
 struct ppt *last;
 uint64_t *pagedir;
 unsigned int pdptcount;

 next=processpaging;
 
 while(next != NULL) {					/* find process */
  if(next->process == process) break;		

  last=next;
  next=next->next;
 }

 v=0xc0000000;

 for(count=0;count != 512 ;count++) {				/* page directories */

  v=(uint64_t)pagedir[count] & 0xfffffffffffff000;			/* point to page table */   
 
  if(v != 0) {							/* in use */
 
     v += KERNEL_HIGH;

     for(countx=0;countx<511;countx++) {
      if(v[countx] != 0) {					/* free page table  */
       kernelfree(v[countx] & 0xfffffffffffff000);
       removepage(v[countx] & 0xfffffffffffff000,getpid());
      }
     
    }
  }
 }

last->next=next->next;						/* entry before this entry will now point to the next entry,removing it from the chain */
 kernelfree(next);						/* and free it */

 return(NULL);
}

/* find free virtual address */

unsigned int findfreevirtualpage(unsigned int size,unsigned int alloc,size_t process) {
unsigned int s;
unsigned int start;
unsigned int end;
struct ppt *next;
uint32_t *pagedirptr;
uint32_t *ptptr;
unsigned int pdcount;
unsigned int ptcount;
unsigned int last;
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


/* search through page tables for a free entry */


for(pdptcount=start;pdptcount<end;pdptcount++) {

 if(next->pdpt[pdptcount] != 0) {
  pagedirptr=0x80000000;
  pagedirptr += (next->pdpt[pdptcount] & 0xffffff000);

     for(pdcount=0;pdcount<512;pdcount++) {			/* pdpts */     

    	 if(pagedirptr[pdcount] != 0) {
	        if(s == 0) last=pdcount;

		s=s+PAGE_SIZE;
	       if(s >= size) return( (pdptcount << 30)+(pdcount << 21)+(last << 12)); /* found enough */	       
	 }
	 else
	 {
	        s=0;				/* must be contiguous */
      		last=pdcount;
         }
	
   }

  }
}

/* search through pdpts for a free entry */

for(pdcount=start;pdcount<end;pdcount++) {			/* page directories */
 if(next->pdpt[pdcount] == 0) {
  s=s+(1024*1024)*2;						/* found 4Mb */

  if(s >= size)  return(pdcount << 22);
 }
}
	
 setlasterror(NO_MEM);
 return(-1);
}

unsigned int loadpagetable(size_t process) {
struct ppt *next; 
next=processpaging;

while(next != NULL) {
 if(next->process == process) {

  asm volatile("mov %0, %%cr3":: "b"(next->pdptphys));
  return;
 }

 next=next->next;
}

return(-1);
}

uint64_t getphysicaladdress(size_t process,uint32_t virtaddr) {
 struct ppt *next;
 uint64_t pdpt_of;
 uint64_t pd;
 uint64_t pt;
 uint64_t *p;
 uint64_t *v;

 next=processpaging;					/* find process page directory */

 do {
  if(next->process == process) break;
		
  next=next->next;
 } while(next->next != NULL);

 pdpt_of=(virtaddr & 0xC0000000) >> 30;			/*  directory pointer table */
 pd=(virtaddr & 0x3FE00000) >> 21;
 pt=(virtaddr & 0x1FF000) >> 12;

 v=0xc0000000+(pd*512);
 return(v[pt] & 0xfffffffffffff000);				/* return physical address */
}

unsigned int addpage_user(uint32_t page,size_t process,void *physaddr) { 
 return(addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}

unsigned int addpage_system(uint32_t page,size_t process,void *physaddr) { 
 return(addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}
	
