#define FREE_PHYSICAL	1

#define ALLOC_NORMAL	1
#define ALLOC_KERNEL	2
#define ALLOC_NOPAGING 	4
#define ALLOC_GLOBAL	8
#define ALLOC_GUARDPAGE	16

#define DMA_BUFFER_SIZE	32768

void *alloc_int(size_t flags,size_t process,size_t size,size_t overrideaddress);
size_t free_internal(size_t process,void *b,size_t flags);
void *alloc(size_t size);
void *kernelalloc(size_t size);
void *kernelalloc_nopaging(size_t size);
void *dma_alloc(size_t size);
size_t kernelfree(void *address);
size_t memorymanager_init(void);
void *realloc_int(size_t flags,size_t process,void *address,size_t size);
void *realloc_kernel(void *address,size_t size);
void *realloc_user(void *address,size_t size);
size_t initialize_dma_buffer(size_t dmasize);

