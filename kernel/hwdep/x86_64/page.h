#define PAGE_PRESENT  	1
#define PAGE_USER	4
#define PAGE_SYSTEM 	0
#define PAGE_RO 	0
#define PAGE_RW		2 

#define PML4_PHYS_LOCATION	0x1000
#define PML4_LOCATION		0xfffffffffffff000
#define PDPT_LOCATION		0xffffffffffffe000
#define PAGEDIR_LOCATION	0xffffffffffc00000
#define PAGETABLE_LOCATION	0xffffffff80000000

extern end(void);
extern kernel_begin(void);
size_t addpage_int(size_t mode,size_t process,size_t page,void *physaddr); 
size_t page_init(size_t process);
size_t removepage(size_t page,size_t process);
size_t freepages(size_t process);
size_t findfreevirtualpage(size_t size,size_t alloc,size_t process);
size_t loadpagetable(size_t process);
uint64_t getphysicaladdress(size_t process,uint64_t virtaddr);
size_t addpage_user(uint64_t page,size_t process,void *physaddr); 
size_t addpage_system(uint64_t page,size_t process,void *physaddr); 
size_t signextend(size_t num,size_t bitnum);

