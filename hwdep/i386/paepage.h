
/* PAE paging

XXAAAAAAAAABBBBBBBBBCCCCCCCCCCCC

/* XX     AAAAAAAAA        BBBBBBBBB	    	    CCCCCCCCCCCC
   PDPT	| page 	directory| | page table|	    | 4k page  |  */
/*	  1GB			2MB			4KB */

#define PAGE_DIRECTORY_LOCATION 0x1000

#define ALLOC_NORMAL	0
#define ALLOC_KERNEL	1
#define ALLOC_NOPAGING 2

#define PAGE_SIZE 4096
#define PAGE_PRESENT  	1
#define PAGE_USER	4
#define PAGE_SYSTEM 	0
#define PAGE_RO 	0
#define PAGE_RW		2 

#define ALLOC_NORMAL	0
#define ALLOC_KERNEL	1
#define ALLOC_NOPAGING 	2
#define MEMBUF_START 0x100000

unsigned int addpage(unsigned int mode, unsigned int process,unsigned int page,void *physaddr); 
unsigned int page_init(unsigned int process);
unsigned int removepage(unsigned int page, unsigned int process);
unsigned int freepages(unsigned int process);
unsigned int loadpagetable(unsigned int pid);
unsigned int findfreevirtualpage(unsigned int size,unsigned int alloc,unsigned int process);

void page_init_first_time(void);

struct ppt {
 uint64_t pagedir[4][512]  __attribute__((aligned(0x1000)));;
 uint64_t pdpt[4] __attribute__((aligned(0x20)));
 uint32_t pagedirphys;
 unsigned int process; 
 uint32_t pdptphys;
 struct ppt *next;
} *processpaging=PAGE_DIRECTORY_LOCATION;
		

/* add page */


unsigned int addpage_int(unsigned int mode,unsigned int process,unsigned int page,void *physaddr) { 
 unsigned int count;
 unsigned int countx;
 struct ppt *next;
 struct ppt *remap;
 unsigned int pdpt_of;
 unsigned int pd;
 unsigned int pt;
 uint64_t *pagedir;
 uint64_t *pagetab;
 unsigned int c;
 char *z[10];

 next=processpaging;					/* find process page directory */

 do {	
  if(next->process == process) break;

  next=next->next;
 } while(next->next != NULL);
//33322222222221111111111
//21098765432109876543210987654321
//00111111111111111111111111111111
//00111111111000000000000000000000
//00000000000111111111000000000000

 pdpt_of=page >> 30;			/*  directory pointer table */
 pd=(page & 0x3FE00000) >> 21;
 pt=(page & 0x1FF000) >> 12;

// tohex(page,z);  
// kprintf(z);
// kprintf(" ");
	
// tohex(pdpt_of,z);
// kprintf(z);
// kprintf(" ");

// tohex(pd,z);
// kprintf(z);
// kprintf(" ");

// tohex(pt,z);
// kprintf(z);
// kprintf("\n");

 pagedir=(uint64_t) next->pdpt[pdpt_of] & 0xfffff000;  /* get address of page table  */
 pagetab=(uint64_t) *pagedir+(pdpt_of*512) & 0xfffff000;
 
if(pagedir[pd] == 0) {				/* if page directory empty */
  c=kernelalloc_nopaging(sizeof(struct ppt));

  if(c == _ERROR) {					/* allocation error */
   currentprocess->lasterror=NO_MEM;
   return(_ERROR);
  }

  pagetab=*pagedir+(pdpt_of*512)+pd;

  *pagedir=c+(PAGE_USER+PAGE_RW+PAGE_PRESENT);	/* page directory entry */
  addpage_int(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,0,c,(void *) c);	/* map into address space */
  fillmem(c,0,PAGE_SIZE);				/* clear memory */
  addpage_int(PAGE_SYSTEM | PAGE_RW | PAGE_PRESENT,0,c,(void *) c);	/* map into address space */
 }

 pagedir=(uint64_t) next->pdpt[pdpt_of] & 0xfffff000;  /* get address of page table  */
 pagetab=(uint64_t) *pagedir+(pdpt_of*512) & 0xfffff000;

 pagetab[pt]=((unsigned int) physaddr)+mode;			/* page table */

 if(page < PROCESS_BASE_ADDRESS && (flags == ALLOC_KERNEL)) {		/* if kernelalloc map into every process */
  remap=processpaging;

  while(remap != NULL) {
   pagedir=(uint64_t) remap->pdpt[pdpt_of] & 0xfffff000;  /* get address of page table  */
   pagetab=(uint64_t) *pagedir+(pdpt_of*512) & 0xfffff000;

   pagetab[pt]=((unsigned int) physaddr)+mode;			/* page table */

   remap=remap->next;
  }
}

if(process == currentprocess->pid) asm volatile("mov %0, %%cr3":: "b"(next->pagedirphys));
}

/* Intialize page table */

void page_init_first_time(void) {
 char *z[10];
 unsigned int c;
 uint32_t ad;
 unsigned int count;
 unsigned int haha;
 void *pp;

 kprintf("Enabling PAE paging\n");

//  asm("xchg %bx,%bx");

 processpaging->pdptphys=processpaging->pdpt;			/* pdpt physical address */
 processpaging->pagedirphys=processpaging->pagedir;		/* page directory physical address */
 processpaging->pdpt[0]=(uint64_t) &processpaging->pagedir[0] | PAGE_PRESENT;		/* first 2gb is for system */
 processpaging->pdpt[1]=(uint64_t) &processpaging->pagedir[1] | PAGE_PRESENT;
 processpaging->pdpt[2]=(uint64_t) &processpaging->pagedir[2] | PAGE_PRESENT;		/* first 2gb is for system */
 processpaging->pdpt[3]=(uint64_t) &processpaging->pagedir[3] | PAGE_PRESENT;

 addpage_int(PAGE_SYSTEM| PAGE_RW | PAGE_PRESENT,0,0,0);				/* real mode ivt & bios data area */

ad=DMA_BUF;

/* identity mapped dma buffer */
 for(c=0;c<(DMA_BUF_SIZE/PAGE_SIZE);c++) {
  addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,0,ad,ad);		/* floppy transfer buffer */
  ad=ad+PAGE_SIZE;
 }

 haha=PAGE_DIRECTORY_LOCATION;

 for(count=0;count<sizeof(struct ppt);count=count+PAGE_SIZE) {
  addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,0,haha,haha);

  haha=haha+PAGE_SIZE;
 }

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

 while(c < MEMBUF_START+(((MEM_SIZE/PAGE_SIZE+PAGE_SIZE)+PAGE_SIZE)*sizeof(m))) {
  addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,0,c,(void *) c);			/* add page */
  c=c+PAGE_SIZE;
 } 

asm volatile("mov %0, %%cr3":: "b"(processpaging->pdptphys));

asm(".intel_syntax noprefix");
asm("mov eax,cr4");
asm("bts eax,5");
asm("mov cr4,eax");
asm("mov eax,cr0");
asm("or	eax,0x80000000");
asm("mov cr0,eax");
asm(".att_syntax");
}

/* intialize address space */

unsigned int page_init(unsigned int process) {
 struct ppt *p;
 char *z[10];
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
 if(pp == _ERROR) return(_ERROR);
 
 virtualaddress=findfreevirtualpage(sizeof(struct ppt),ALLOC_KERNEL,0);		/* find free virtual address */
 if(virtualaddress == _ERROR) {
  kernelfree(pp);
  return(_ERROR);
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

 ppx=(uint64_t) pp | PAGE_PRESENT;
	
 p->pdpt[0]=((uint64_t) ppx);		/* first 2gb is for system */
 p->pdpt[1]=((uint64_t) ppx+PAGE_SIZE);

 temppp=pp;

 p->pdptphys=pp+(4*512)*8;						/* physical address of page directory */
 p->pagedirphys=pp;						/* physical address of page directory */
 p->process=process;

 memcopy(processpaging->pagedir[0],p->pagedir[0],512);			/* copy page directory */
 memcopy(processpaging->pagedir[1],p->pagedir[1],512);			/* copy page directory */

 p->next=NULL;						/* ugggh */

 asm volatile("mov %0, %%cr3":: "b"(p->pdptphys));

 return(NULL);
}


/* remove page */
unsigned int removepage(uint32_t page,unsigned int process) {
 addpage_int(0,process,page,0);						/* clear page */
 return(NULL);
}

unsigned int freepages(unsigned int process) {
 struct ppt *next;
 unsigned int count;
 unsigned int countx;
 unsigned int *v;
 struct ppt *last;
 char *z[10];

 next=processpaging;
 
 while(next != NULL) {					/* find process */
  if(next->process == currentprocess->pid) break;		

  last=next;
  next=next->next;
 }

for(count=0;count<511;count++) {				/* page directories */

  v=(uint64_t)next->pagedir[count] & 0xfffff000;			/* point to page table */   

  if(v != 0) {							/* in use */
 
     for(countx=0;countx<511;countx++) {
      if(v[countx] != 0) {					/* free page table  */
       kernelfree(v[countx] & 0xfffff000);
       removepage(v[countx] & 0xfffff000,currentprocess->pid);
      }
    }
  }
}

last->next=next->next;						/* entry before this entry will now point to the next entry,removing it from the chain */
 kernelfree(next);						/* and free it */

 return(NULL);
}

/* find free virtual address */

unsigned int findfreevirtualpage(unsigned int size,unsigned int alloc,unsigned int process) {
unsigned int s;
unsigned int start;
unsigned int end;
uint64_t pdpt_of;
uint64_t pd;
uint64_t pt;
uint64_t *pagedir;
unsigned int count;
unsigned int countx;
unsigned int pdpt_count;
uint32_t a;
uint64_t *pagetab;
char *z[10];
struct ppt *next;

 next=processpaging;
 
 do {						/* find process struct */
  if(next->process == process) break;
  next=next->next;
 } while(next != NULL);

 /* walk through page directories and tables to find free pages in directories already used */

if(alloc == ALLOC_NORMAL) {
 start=2;			/* start halfway or 0x80000000 */
 end=4;
}
else
{
 start=0;
 end=1;
}

for(pdpt_count=start;pdpt_count<end;pdpt_count++) {
  if(next->pdpt[pdpt_count] == 0) return(pdpt_count << 30); 
 }
for(pdpt_count=start;pdpt_count<end;pdpt_count++) {
  if(next->pdpt[pdpt_count] != 0) {				/* in use */

     pagedir=next->pdpt[pdpt_count] & 0xfffffffffffff000;

	  for(count=0;count<511;count++) {			/* page directories */
	    if(pagedir[count] != 0) {						/* in use */
	     pagetab=(uint64_t) pagedir[count] & 0xfffffffffffff000;				/* point to page table */  
	     s=0;

   	     a=countx;
	 
	     for(countx=0;countx<511;countx++) {       
	       if(pagetab[countx] == 0) s=s+PAGE_SIZE;			/* free page table */

	       if(pagetab[countx] != 0) {
                s=0;				/* must be contiguous */
		a=countx;
               }
	
	       if(s >= size) return( (pdpt_count << 30)+(count << 21)+(countx << 12)); /* found enough */	       

	     }
 	    }
          }
    }
 }

 currentprocess->lasterror=NO_MEM;
 return(_ERROR);
}

unsigned int loadpagetable(unsigned int process) {
struct ppt *next; 
char *z[10];

next=processpaging;

while(next->next != NULL) {
 if(next->process == process) break;
 next=next->next;
}

 asm volatile("mov %0, %%cr3":: "b"(next->pdptphys));
return;
}

unsigned int getphysicaladdress(uint32_t process,uint32_t virtaddr) {
 struct ppt *next;
 uint64_t pdpt_of;
 uint64_t pd;
 uint64_t pt;
	 uint64_t *p;
 uint64_t *v;
 char *z[10];

 next=processpaging;					/* find process page directory */

 do {
  if(next->process == process) break;
		
  next=next->next;
 } while(next->next != NULL);

 pdpt_of=(virtaddr & 0xC0000000) >> 30;			/*  directory pointer table */
 pd=(virtaddr & 0x3FE00000) >> 21;
 pt=(virtaddr & 0x1FF000) >> 12;

 p=next->pdpt[pdpt_of] & 0xfffffffe;		/* get page directory */
 v=p[pd]  & 0xfffff000;		/* get page table */

 return(v[pt] & 0xfffff000);				/* return physical address */
}

unsigned int addpage_user(uint32_t page,unsigned int process,void *physaddr) { 
 return(addpage_int(PAGE_USER+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}

unsigned int addpage_system(uint32_t page,unsigned int process,void *physaddr) { 
 return(addpage_int(PAGE_SYSTEM+PAGE_RW+PAGE_PRESENT,process,page,physaddr));
}
	
