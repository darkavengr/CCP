#include <stdint.h>
#include <stddef.h>

typedef struct PAGING {
	uint32_t pagedir[1024];				/* page directory */
	uint32_t pagedirphys;				/* physical address of page directory */
	uint32_t process; 				/* process ID */
	struct PAGING *next;				/* pointer to next paging entry */
} PAGING;

#define PAGE_DIRECTORY_LOCATION 0x1000

#define PAGE_PRESENT  	(1 << 0)
#define PAGE_RW		(1 << 1)
#define PAGE_USER	(1 << 2)
#define PAGE_ACCESSED	(1 << 5)
#define PAGE_DIRTY	(1 << 6)

#define PAGE_SYSTEM 	0
#define PAGE_RO 	0

#define PAGING_TYPE_LEGACY	1
#define PAGING_TYPE_PAE		2

#define PAGEDIR_ADDRESS 0xFFFFF000
#define PAGETABLE_ADDRESS 0xFFC00000

size_t map_page_internal(size_t mode,size_t page,void *physaddr); 
size_t page_init(size_t process);
size_t unmap_page(size_t page);
size_t freepages(size_t process);
size_t findfreevirtualpage(size_t size,size_t alloc);
size_t switch_address_space(size_t process);
size_t getphysicaladdress(size_t virtaddr);
size_t map_user_page(size_t page,void *physaddr); 
size_t map_system_page(size_t page,void *physaddr); 
size_t get_pagetable_entry(size_t virtaddr);
PAGING *GetProcessPagingPointer(void);
void remove_processpaging_entry(size_t virtaddress);

