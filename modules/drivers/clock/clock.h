#include <stdint.h>

#define CMOS_TIMEDATE_SIZE 6

#define CMOS_COMMAND_PORT	0x70
#define CMOS_DATA_PORT		0x71

size_t clockio_internal(size_t op,void *buf);
size_t clock_init(char *init);
size_t clockio_read(void *buf,size_t ignored);
size_t clockio_write(void *buf,size_t ignored);


