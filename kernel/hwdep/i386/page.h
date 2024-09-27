#define PAGE_DIRECTORY_LOCATION 0x1000

#define PAGE_PRESENT  	1
#define PAGE_USER	4
#define PAGE_SYSTEM 	0
#define PAGE_RO 	0
#define PAGE_RW		2 

size_t addpage_int(size_t mode,size_t process,uint32_t page,void *physaddr); 
size_t page_init(size_t process);
size_t removepage(uint32_t page,size_t process);
size_t freepages(size_t process);
size_t findfreevirtualpage(size_t size,size_t alloc,size_t process);
size_t loadpagetable(size_t process);
size_t getphysicaladdress(size_t process,uint32_t virtaddr);
size_t addpage_user(uint32_t page,size_t process,void *physaddr); 
size_t addpage_system(uint32_t page,size_t process,void *physaddr); 

