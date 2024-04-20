#define FREE_PHYSICAL	1

#define ALLOC_NORMAL	0
#define ALLOC_KERNEL	1
#define ALLOC_NOPAGING 	2
#define ALLOC_GLOBAL	4

#define INITIAL_HEAP_SIZE 1024*1024

typedef struct {
	uint8_t allocationtype;		/* M=allocated, Z=last */
	size_t size;
}  __attribute__((packed)) HEAPENTRY;

void *alloc_int(size_t flags,size_t process,size_t size,size_t overrideaddress);
size_t free_internal(size_t process,void *b,size_t flags);
void *alloc(size_t size);
void *kernelalloc(size_t size);
void *kernelalloc_nopaging(size_t size);
void *dma_alloc(size_t size);
size_t heapfree(void *address);
HEAPENTRY *getheapaddress(void);
HEAPENTRY *getheapend(void);
size_t getheapsize(void);
void *heapalloc_int(size_t type,HEAPENTRY *heap,HEAPENTRY *heapend,size_t size);


