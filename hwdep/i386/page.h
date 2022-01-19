#define PAGE_DIRECTORY_LOCATION 0x1000

#define ALLOC_NORMAL	0
#define ALLOC_KERNEL	1
#define ALLOC_NOPAGING 2
#define KERNEL_HIGH 1 << (sizeof(unsigned int)*8)-1

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




