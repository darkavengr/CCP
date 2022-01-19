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

/* address */
/* AAAAAAAAAABBBBBBBBBBCCCCCCCCCCCC */ 
  
/* Paging is divided up into 4MB chunks, which are subdivided into 4k entires which represent a 4k block of *physical* memory */
/*   AAAAAAAAAA				       BBBBBBBBBB	    CCCCCCCCCCCC
 * |  4MB Chunk     	         |    

 * | Points to 1024 page tables  | ->  | 1024 4kb page entries |-> | 4k page |	

/* add page */

#include <stdint.h>
#include <stddef.h>
#include "page.h"
#include "hwdefs.h"
#include "../../header/errors.h"

unsigned int PAGE_SIZE=4096;

size_t addpage_int(unsigned int mode,size_t process,uint32_t page,void *physaddr); 
void page_init_first_time(void);
size_t page_init(size_t process);
size_t removepage(uint32_t page,size_t process);
size_t freepages(size_t process);
size_t  findfreevirtualpage(size_t size,unsigned int alloc,size_t process);
size_t loadpagetable(size_t process);
size_t getphysicaladdress(size_t process,uint32_t virtaddr);
size_t addpage_user(uint32_t page,size_t process,void *physaddr); 
size_t addpage_system(uint32_t page,size_t process,void *physaddr); 

extern end(void);

extern kernel_begin(void);
extern MEM_SIZE;

unsigned int paging_type=1;

#define MEMBUF_START end

struct ppt {
 uint32_t pagedir[1024];
 uint32_t pagedirphys;
 size_t process; 
 struct ppt *next;
} *processpaging=PAGE_DIRECTORY_LOCATION+KERNEL_HIGH;

size_t addpage_int(unsigned int mode,size_t process,uint32_t page,void *physaddr) { 
 size_t count;
 struct ppt *next;
 struct ppt *remap;
 struct ppt *current;
 uint32_t pd;
 uint32_t pt;
 uint32_t p;
 uint32_t *v;
 uint32_t c;
 uint32_t temp;

 next=processpaging;

 while(next != NULL) {					/* find process */	
  if(next->process == getpid()) current=next;
  if(next->process == process) break;		

  next=next->next;
 }

 pd=(page >> 22) & 0x3ff;				/* page directory offset */
 pt=(page >> 12) & 0x3ff;				/* page table offset */
 p=page & 0xFFF;

 if(next->pagedir[pd] == 0) {				/* if page directory empty */	
  c=kernelalloc_nopaging(PAGE_SIZE);
  if(c == NULL) return(-1);

  next->pagedir[pd]=(uint32_t) c | PAGE_USER | PAGE_RW | PAGE_PRESENT;	/* page directory entry */
 }

/* page tables are mapped at 0xffc00000 via recursive paging */

 v=(uint32_t *) 0xffc00000 + (pd*1024);
 v[pt]=((uint32_t) physaddr & 0xfffff000)+mode;			/* page table */
 
 if(page >= KERNEL_HIGH) {		/* if kernelalloc map into every process */
  remap=processpaging;

  while(remap != NULL) {
	  next->pagedir[1022]=remap->pagedirphys | PAGE_RW | PAGE_PRESENT;

	  v=(uint32_t *) 0xff800000 + (pd*1024);
  	  v[pt]=((uint32_t) physaddr & 0xfffff000)+mode;			/* page table */

	remap=remap->next;
  }
 }

 return;
}


/* intialize address space */

size_t page_init(size_t process) {
 struct ppt *p;
 struct ppt *last;
 uint32_t oldvirtaddress;
 uint32_t virtaddress;
 uint32_t oldpp;
 uint32_t pp;
 size_t count;
 size_t size;

 if(process == 0) return;

 p=processpaging;							/* find process */
 last=p;

 while(p != NULL) {
  last=p;  
  p=p->next;
 }

/* get a physical address and manually map it to a virtual address becaus the physical addres of p->pagedir
   is needed to fill cr3 */

size=(sizeof(struct ppt) & ((0-1)-(PAGE_SIZE-1)))+PAGE_SIZE;		/* round */

pp=kernelalloc_nopaging(size);					/* add link */
if(pp == NULL) return(-1);

virtaddress=findfreevirtualpage(size,ALLOC_KERNEL,0);		/* find free virtual address */

oldvirtaddress=virtaddress;
oldpp=pp;

//kprintf("address=%X %X\n",virtaddress,pp);

for(count=0;count != (sizeof(struct ppt)/PAGE_SIZE)+1;count++) {
 addpage_system(virtaddress,0,(void *) pp);

 pp += PAGE_SIZE;
 virtaddress += PAGE_SIZE;
}

last->next=oldvirtaddress;
last=last->next;

last->pagedirphys=oldpp;						/* physical address of page directory */
last->process=process;

memcpy(&last->pagedir[512],&processpaging->pagedir[512],PAGE_SIZE/2);			/* copy page directory */
last->pagedir[1023]=last->pagedirphys+(PAGE_RW+PAGE_PRESENT);

last->next=NULL;
return;
}

/* remove page */
size_t removepage(uint32_t page,size_t process) {
 addpage_int(0,process,page,0);						/* clear page */
 return;
}

size_t freepages(size_t process) {
 struct ppt *next;
 struct ppt *last;
 size_t count;
 size_t countx;
 uint32_t *v;

 next=processpaging;
 last=processpaging;

 while(next != NULL) {					/* find process */
  if(next->process == process) break;		

  last=next;
  next=next->next;
 }

 last->next=next->next;	

 kernelfree(next);						/* and free it */

 return;
}

/* find free virtual address */

size_t  findfreevirtualpage(size_t size,unsigned int alloc,size_t process) {
uint32_t s;
size_t start;
size_t end;
size_t count;
size_t countx;
uint32_t *v;
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

     v=(uint32_t *) 0xffc00000 + (count*1024);

     for(countx=0;countx<1023;countx++) {      
       if(v[countx] == 0) {
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

size_t loadpagetable(size_t process) {
struct ppt *next; 

next=processpaging;

while(next != NULL) {
 if(next->process == process) {
  asm volatile("mov %0, %%cr3":: "a"(next->pagedirphys));
 
  return;
 }

 next=next->next;
}


return(-1);
}

size_t getphysicaladdress(size_t process,uint32_t virtaddr) {
 size_t count;
 struct ppt *next;
 struct ppt *remap;
 struct ppt *current;
 uint32_t pd;
 uint32_t pt;
 uint32_t p;
 uint32_t *v;
 uint32_t c;
 
 next=processpaging;					/* find process page directory */

 do {
  if(next->process == process) break;
		
  next=next->next;
 } while(next != NULL);

 pd=(virtaddr >> 22) & 0x3ff;				/* page directory offset */
 pt=(virtaddr >> 12) & 0x3ff;				/* page table offset */
 p=(virtaddr & 0xfff);

 v=(uint32_t *) 0xffc00000 + (pd*1024);
 return(v[pt] & 0xfffff000);				/* return physical address */
}

size_t addpage_user(uint32_t page,size_t process,void *physaddr) { 
 return(addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}

size_t addpage_system(uint32_t page,size_t process,void *physaddr) { 
 return(addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}


