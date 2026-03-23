#include <stdint.h>

size_t example_chardevice_init(char *initstr);
size_t example_chardevice_read(void *buf,size_t size);
size_t example_chardevice_write(void *buf,size_t size);
size_t example_chardevice_ioctl(size_t handle,unsigned long request,char *buffer);


