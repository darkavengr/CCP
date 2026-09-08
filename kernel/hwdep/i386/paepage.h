#include <stdint.h>
#include <stddef.h>

#ifndef PAEPAGE_H
	#define PAEPAGE_H

	typedef struct PAEPAGING {
		uint64_t pdpt[4] __attribute__((aligned(0x20)));			/* PDPT */
		size_t process; 							/* process ID */
		uint32_t pdptphys;							/* Physical address of PDPT */
		struct PAEPAGING *next;							/* pointer to next paging entry */
	} PAEPAGING;
#endif

#define PDPT_LOCATION 	0xc0000000

#define PAGE_PRESENT  	(1 << 0)
#define PAGE_USER	(1 << 1)
#define PAGE_RW		(1 << 2)
#define PAGE_ACCESSED	(1 << 5)
#define PAGE_DIRTY	(1 << 6)
#define PAGE_SCANNED	(1 << 9)

#define PAGE_SYSTEM 	0
#define PAGE_RO 	0

#define PAGING_TYPE_PAE	2

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
void remove_processpaging_entry(size_t virtaddress);

