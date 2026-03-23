#include <stdint.h>

size_t example_block_device_init(char *init);
size_t block_device_io(size_t op,size_t physdrive,uint64_t block,uint16_t *buf);
size_t example_block_device_ioctl(size_t handle,unsigned long request,char *buffer);


