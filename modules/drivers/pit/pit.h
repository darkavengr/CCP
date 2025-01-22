#define PIT_VAL 1193180 / 50				/* frequency of irq 0 called by PIT */
#define NULL 0

size_t pit_init(char *init);
size_t pit_io(size_t op,size_t *buf,size_t ignored);
size_t pit_read(size_t size,size_t *buf);
size_t pit_write(size_t size,size_t *buf);


