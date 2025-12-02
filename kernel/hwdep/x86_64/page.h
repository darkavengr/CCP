#define PAGE_PRESENT  	1
#define PAGE_USER	4
#define PAGE_SYSTEM 	0
#define PAGE_RO 	0
#define PAGE_RW		2 

#define PML4_PHYS_LOCATION	 0x1000

#define PML4_HIGHER_HALF_ENTRY	511

size_t map_page_internal(size_t mode,size_t process,size_t page,void *physaddr); 
size_t page_init(size_t process);
size_t unmap_page(size_t page,size_t process);
size_t freepages(size_t process);
size_t findfreevirtualpage(size_t size,size_t alloc,size_t process);
size_t switch_address_space(size_t process);
uint64_t getphysicaladdress(size_t process,uint64_t virtaddr);
size_t map_user_page(uint64_t page,size_t process,void *physaddr); 
size_t map_system_page(uint64_t page,size_t process,void *physaddr); 
size_t signextend(size_t num,size_t bitnum);

