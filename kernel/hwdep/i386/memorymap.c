#include <stdint.h>
#include <stddef.h>
#include "kernelhigh.h"
#include "bootinfo.h"
#include "debug.h"

extern size_t PAGE_SIZE;

#define SYSTEM_USE -1		/* page frame marked for system use */

void initialize_memory_map(size_t memory_map_address,size_t memory_size,size_t kernel_begin,size_t kernel_size,size_t stack_address,size_t stack_size) {
size_t count;
size_t *bufptr;
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;		/* point to boot information */
size_t memory_buffer_size;


memory_size=(memory_size & ((0-1)-(PAGE_SIZE-1)));	/* round down memory size to last PAGE_SIZE */

/* clear memory map area before filling it */

bufptr=memory_map_address;

memset(bufptr,0,(memory_size/PAGE_SIZE));

/* map first megabyte of memory	*/
bufptr=memory_map_address;

for(count=0;count<(1024*1024)/PAGE_SIZE;count++) {
	*bufptr++=SYSTEM_USE;
}

/* map kernel */

bufptr=memory_map_address+(((kernel_begin-KERNEL_HIGH)/PAGE_SIZE)*sizeof(size_t));

for(count=0;count<(kernel_size/PAGE_SIZE);count++) {
	*bufptr++=SYSTEM_USE;
}

/* map initial kernel stack */
bufptr=memory_map_address+((stack_address/PAGE_SIZE)*sizeof(size_t));

for(count=0;count<(stack_size/PAGE_SIZE);count++) {
	*bufptr++=SYSTEM_USE;
}

/* map memory map */
bufptr=memory_map_address+(((memory_map_address-KERNEL_HIGH)/PAGE_SIZE)*sizeof(size_t));

memory_buffer_size=(memory_size/PAGE_SIZE)*sizeof(size_t);

for(count=0;count<(memory_buffer_size/PAGE_SIZE)*sizeof(size_t);count++) {
	*bufptr++=SYSTEM_USE;
}

/* map initial RAM disk	*/
bufptr=memory_map_address+(bootinfo->initrd_start/PAGE_SIZE)*sizeof(size_t);

for(count=0;count<(bootinfo->initrd_size/PAGE_SIZE);count++) {
	*bufptr++=SYSTEM_USE;
}

/* map kernel symbols */
bufptr=memory_map_address+((bootinfo->symbol_start/PAGE_SIZE)*sizeof(size_t));

for(count=0;count<(bootinfo->symbol_size/PAGE_SIZE);count++) {
	*bufptr++=SYSTEM_USE;
}

return;
}
