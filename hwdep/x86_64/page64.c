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

/* long mode paging

PML4		PDP	PD	PT	page
MMMMMMMMMPPPPPPPPPDDDDDDDDDTTTTTTTTT pppppppppppp
*/

size_t PAGE_SIZE=4096;

size_t addpage(size_t mode, size_t process,size_t page,void *physaddr); 
size_t page_init(size_t process);
size_t removepage(size_t page,size_t process);
size_t freepages(size_t process);
size_t loadpagetable(size_t pid);
size_t findfreevirtualpage(size_t size,size_t alloc,size_t process);
void page_init_first_time(void);

extern MEMBUF_START;
 struct ppt {
  uint64_t pml[512];
  uint64_t pmlphys;
  size_t process;
} *processpaging=PAGE_DIRECTORY_LOCATION;

size_t addpage(uint32_t mode,size_t process,size_t page,void *physaddr); 
struct ppt *next;
size_t pt;
size_t pd;
size_t pdp;
size_t pml;
struct ppt *remap;
uint64_t *ptr;

next=processpaging;

while(next != NULL) {
 if(next->pid == process) break;		/* found */
 next=next->next;
}

pml=(page & 0xFF8000000000) >> 41;
pdp=(page & 0x7FC0000000) >> 30;
pd=(page & 0x3FE00000) >> 21;
pt=(page & 0x1FF000) >> 12;

if(next->pml[pml] == 0) {		/* pml4 */
 next->pml[pml]=kernelalloc(PAGE_SIZE);
 if(next->pml[pml] == NULL) return(-1); /* can't allocate */

 addpage_int(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,0,next->pdp[pdp],(void *) next->pdp[pdp]);	/* map into address space */
 memset(c,0,PAGE_SIZE);				/* clear memory */
 addpage_int(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,0,next->pdp[pdp],(void *) next->pdp[pdp]);	/* map into address space */

 ptr=next->pml[pml];		/* point to pdp */
 if(ptr[pdp] == 0) {
  ptr[pdp]=kernelalloc_nopaging(PAGE_SIZE);
  if(ptr[pdp] == NULL) return(-1); /* can't allocate */

  addpage_int(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,0,ptr[pdp],(void *) ptr[pdp]);	/* map into address space */
  memset(c,0,PAGE_SIZE);				/* clear memory */
  addpage_int(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,0,ptr[pdp],(void *) ptr[pdp]);	/* map into address space */

  ptr=ptr[pd];				/* point to page directory */
  if(ptr[pd] == 0) {
   ptr[pd]=kernelalloc_nopaging(PAGE_SIZE);
   if(ptr[pd] == NULL) return(-1); /* can't allocate */

   addpage_int(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,0,ptr[pd],(void *) ptr[pd]);	/* map into address space */
   memset(c,0,PAGE_SIZE);				/* clear memory */
   addpage_int(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,0,ptr[pd],(void *) ptr[pd]);	/* map into address space */
  }
 }
}

ptr=next->pml[pml];		/* point to pml */
ptr=ptr[pdp];
ptr=ptr[pd];			/* point to page directory */
ptr[pd]=(page & 0xfff)+flags;

if(page < PROCESS_BASE_ADDRESS) {		/* if kernelalloc map into every process */
 remap=processpaging;

 while(remap != NULL) {
  v=(uint32_t) remap->pagedir[pd] & 0xfffff000;
  v[pt]=((uint32_t) physaddr & 0xfffff000)+mode;			/* page table */
  remap=remap->next;
 }

if(process == getpid()) asm volatile("mov %0, %%cr3":: "b"(next->pagedirphys));
}


/* Intialize page table */

void page_init_first_time(void) {
 uint64_t c;
 size_t count;
 size_t ad;

 memset(processpaging,0,(PAGE_SIZE*2));				/* clear page directory */
 processpaging->pagedirphys=PAGE_DIRECTORY_LOCATION;			/* page directory physical address */


 addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,0,0,0);				/* real mode ivt & bios data area */
 addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,0,PAGE_DIRECTORY_LOCATION,PAGE_DIRECTORY_LOCATION);	
 addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,0,PAGE_DIRECTORY_LOCATION+PAGE_SIZE,PAGE_DIRECTORY_LOCATION+PAGE_SIZE);

 /* ROMs and graphic display -0xA0000 - 0xFFFFF */
 for(c=0xA0000;c<0xFFFFF;c=c+PAGE_SIZE) {
  addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,0,c,c);
 }

 for(c=kernel_begin;c<end;c=c+PAGE_SIZE) { 				/* kernel */
  addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,0,c,(void *) c);
 }

/* stack */ 
 for(c=KERNEL_STACK_ADDRESS;c<KERNEL_STACK_ADDRESS+KERNEL_STACK_SIZE+PAGE_SIZE;c=c+PAGE_SIZE) {
  addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,0,c,(void *) c);
 }

/* memory map*/

 c=MEMBUF_START;

 while(c < MEMBUF_START+(((MEM_SIZE/PAGE_SIZE+PAGE_SIZE)+PAGE_SIZE)*sizeof(size_t))) {
  addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,0,c,(void *) c);			/* add page */
  c=c+PAGE_SIZE;
 } 

asm volatile("mov %0, %%cr3":: "b"(processpaging->pagedirphys));

asm(".intel_syntax noprefix");
asm("mov eax,cr0");
asm("or	eax,0x80000000");
asm("mov cr0,eax");
asm(".att_syntax");						/* ugggh */

}

/* intialize address space */

size_t page_init(size_t process) {
 struct ppt *p;
 struct ppt *last;
 uint64_t virtaddress;
 uint64_t pp;

 p=processpaging;							/* find process */
 last=p;

 while(p != NULL) {
  last=p;  
  p=p->next;
 }

/* get a physical address and manually map it to a virtual address becaus the physical addres of p->pagedir
   is needed to fill cr3 */

pp=kernelalloc_nopaging(sizeof(struct ppt));					/* add link */
 if(pp == NULL) return(-1);

virtaddress=findfreevirtualpage(sizeof(struct ppt),ALLOC_KERNEL,0);		/* find free virtual address */

addpage_user(virtaddress,0,(void *) pp);
addpage_user(virtaddress+PAGE_SIZE,0,(void *) pp+PAGE_SIZE);

memset(virtaddress,0,sizeof(struct ppt));
last->next=virtaddress;
last=last->next;

 memcpy(last->pagedir,processpaging->pagedir,512);			/* copy page directory */
 last->next=NULL;
 last->pmlphys=pp;						/* physical address of page directory */
 last->process=process;

}

size_t removepage(uint64_t page,size_t process) {
 addpage_int(0,process,page,0);						/* clear page */
 return(NULL);
}

size_t freepages(size_t process) {
size_t pt;
size_t pd;
size_t pdp;
size_t pml;
uint64_t *pmlptr;
uint64_t *pdpptr;
uint64_t *pdptr;

for(pml=0;pml<512;pml++) {
 if(next->pml[pml] != 0) {		/* found entry */
  pmlptr=next->pml[pml];

  for(pdp=0;pdp<512;pdp++) {
    
    if(pdpptr[pdp] != 0) {
        pdptr=pdpptr[pd];

        for(pd=0;pd<512;pdp++) {
	  if(pdptr[pd] != 0) kernelfree(pdptr[pd & 0xfffffffffffff000);
        }

     kernelfree(pdptr[pdp]);
    }
   }
  
   kernelfree(pmlptr[pml] & 0xfffffffffffff000);
 }
}
}

size_t loadpagetable(size_t pid) {
struct ppt *next; 

next=processpaging;

while(next != NULL) {
 if(next->process == pid) {
  asm volatile("mov %0, %%cr3":: "a"(next->pmlphys));
  return;
 }

 next=next->next;
}

return;
}

size_t findfreevirtualpage(size_t size,size_t alloc,size_t process) {
size_t pt;
size_t pd;
size_t pdp;
size_t pml;
uint64_t *pmlptr;
uint64_t *pdpptr;
uint64_t *pdptr;
size_t count=0;

for(pml=0;pml<512;pml++) {
 if(next->pml[pml] != 0) {		/* found entry */
  pmlptr=next->pml[pml];

  for(pdp=0;pdp<512;pdp++) {
    
    if(pdpptr[pdp] != 0) {
        pdptr=pdpptr[pd];
         
        for(pd=0;pd<512;pdp++) {
	         pdptr=pd[pd];			/* point to page table */

       		 if(pdptr[pd] == 0) {
		 	  for(pt=0;pt<512;pt++) {
				  ptptr=pdptr[pd];
				  if(ptptr[pt] == 0) {		/* found a free page */
			           count=count+PAGE_SIZE;'
			          }
          			  else
			          {
			           count=0;			/* must be continguous */
			          }         
        
	  			  if(count >= size) return((pml >> 39)+(pdp >> 30)+(pd >> 21)+(pt >> 12);
         		  }
       
	/* can't find find free page table, try to find free pagedir */

         	count += 0x200000;
		if(count >= size) return((pml >> 39)+(pdp >> 30)+(pd >> 21)+(pt >> 12);
         }
   
    }

	/* find free pdp instead */

	count=count+0x40000000;
	if(count >= size) return((pml >> 39)+(pdp >> 30)+(pd >> 21)+(pt >> 12);
   }
  

 }
}
}


uint64_t getphysicaladdress(size_t process,uint32_t virtaddr) {
struct ppt *next;
size_t pt;
size_t pd;
size_t pdp;
size_t pml;
uint64_t *pdpptr; 
uint64_t *pdptr; 
uint64_t *ptptr; 

next=processpaging;					/* find process page directory */

 do {
  if(next->process == process) break;
		
  next=next->next;
 } while(next->next != NULL);

pml=(page & 0xFF8000000000) >> 41;
pdp=(page & 0x7FC0000000) >> 30;
pd=(page & 0x3FE00000) >> 21;
pt=(page & 0x1FF000) >> 12;

pdptr=next->pml;		/* point to pdp */
pdptr=pdptr[pd];


 v=(uint32_t) next->pagedir[pd] & 0xfffff000;
 return(v[pt] & 0xfffff000);				/* return physical address */
}

size_t addpage_user(uint64_t page,size_t process,void *physaddr) { 
 return(addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}
size_t addpage_system(uint64_t page,size_t process,void *physaddr) { 
 return(addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}

