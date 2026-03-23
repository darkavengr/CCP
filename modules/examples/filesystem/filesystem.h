#include <stdint.h>
#include <time.h>

size_t example_filesystem_init(char *initstr);
size_t fat_findfirst(char *filename,FILERECORD *buf);
size_t fat_findnext(char *filename,FILERECORD *buf);
size_t example_filesystem_find(size_t find_type,char *filename,FILERECORD *buf);
size_t example_filesystem_read(size_t handle,void *addr,size_t size);
size_t example_filesystem_write(size_t handle,void *addr,size_t size);
size_t example_filesystem_rename(char *filename,char *newname);
size_t example_filesystem_delete(char *filename);
size_t example_filesystem_create(char *filename);
size_t example_filesystem_mkdir(char *dirname);
size_t example_filesystem_rmdir(char *dirname);
size_t example_filesystem_chmod(char *filename,size_t attributes);
size_t example_filesystem_touch(char *filename,TIME *create_time_date,TIME *last_modified_time_date,TIME *last_accessed_time_date);
