#include "pagesize.h"

#define FREE_PHYSICAL	(1 << 0)

#define ALLOC_NORMAL	(1 << 0)
#define ALLOC_KERNEL	(1 << 1)
#define ALLOC_NOPAGING 	(1 << 2)
#define ALLOC_GLOBAL	(1 << 4)

#define DMA_BUFFER_SIZE	32768

void *alloc_int(size_t flags,size_t size,size_t overrideaddress);
size_t free_internal(void *address,size_t flags);
void *alloc(size_t size);
void *kernelalloc(size_t size);
void *kernelalloc_nopaging(size_t size);
void *dma_alloc(size_t size);
size_t kernelfree(void *address);
size_t memorymanager_init(void);
void *realloc_int(size_t flags,void *address,size_t size);
void *realloc_kernel(void *address,size_t size);
void *realloc_user(void *address,size_t size);
size_t initialize_dma_buffer(size_t dmasize);
void *GetKernelHeapAddress(void);
void *GetKernelHeapEnd(void);
size_t GetHeapSize(void);
void *heapalloc_int(size_t type,size_t size);
size_t heapfree(size_t type,void *address);
size_t kernelfree(void *address);
size_t kernelalloc_page_aligned(size_t size);

#ifndef MEMORYMANAGER_H
	#define MEMORYMANAGER_H
	typedef struct {
		uint32_t magic;
		size_t size;
	} HEAPHEADER;
#endif

