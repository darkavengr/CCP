#define PAGE_DIRECTORY_LOCATION 0x1000

#define ALLOC_NORMAL	0
#define ALLOC_KERNEL	1
#define ALLOC_NOPAGING 2

#define PAGE_PRESENT  	1
#define PAGE_USER	4
#define PAGE_SYSTEM 	0
#define PAGE_RO 	0
#define PAGE_RW		2 

#define ALLOC_NORMAL	0
#define ALLOC_KERNEL	1
#define ALLOC_NOPAGING 	2

#define KERNEL_STACK_SIZE  65536
#define KERNEL_STACK_ADDRESS 0x80000

size_t addpage_int(size_t mode,size_t process,uint32_t page,void *physaddr); 
void page_init_first_time(void);
size_t page_init(size_t process);
size_t removepage(uint32_t page,size_t process);
size_t freepages(size_t process);
size_t  findfreevirtualpage(size_t size,size_t alloc,size_t process);
size_t loadpagetable(size_t process);
size_t getphysicaladdress(size_t process,uint32_t virtaddr);
size_t addpage_user(uint32_t page,size_t process,void *physaddr); 
size_t addpage_system(uint32_t page,size_t process,void *physaddr); 


