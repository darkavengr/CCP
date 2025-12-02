#include <stdint.h>
#include <stddef.h>

#ifdef USING_PAE_PAGING
typedef struct {
		uint64_t pdpt[4] __attribute__((aligned(0x20)));			/* PDPT */
		size_t process; 							/* process ID */
		uint32_t pdptphys;							/* Physical address of PDPT */
		struct ppt *next;							/* pointer to next paging entry */
	} PAGING;
#else
	typedef struct {
		uint32_t pagedir[1024];				/* page directory */
		uint32_t pagedirphys;				/* physical address of page directory */
		size_t process; 				/* process ID */
		struct ppt *next;				/* pointer to next paging entry */
	} PAGING;
#endif

#define PAGE_DIRECTORY_LOCATION 0x1000

#define PAGE_PRESENT  	1
#define PAGE_USER	4
#define PAGE_SYSTEM 	0
#define PAGE_RO 	0
#define PAGE_RW		2 
#define PAGE_ACCESSED	32
#define PAGE_DIRTY	64

#define PAGE_SWAPPED_OUT 2

#define PAGING_TYPE_LEGACY	1
#define PAGING_TYPE_PAE		2

size_t map_page_internal(size_t mode,size_t process,uint32_t page,void *physaddr); 
size_t page_init(size_t process);
size_t unmap_page(uint32_t page,size_t process);
size_t freepages(size_t process);
size_t findfreevirtualpage(size_t size,size_t alloc,size_t process);
size_t switch_address_space(size_t process);
size_t getphysicaladdress(size_t process,uint32_t virtaddr);
size_t map_user_page(uint32_t page,size_t process,void *physaddr); 
size_t map_system_page(uint32_t page,size_t process,void *physaddr); 
size_t getpagetableentry(size_t process,uint32_t virtaddr);
PAGING *getprocesspagingaddress(void);

