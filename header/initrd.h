#include <stddef.h>


#ifndef INITRD_H
#define INITRD_H
#define TAR_BLOCK_SIZE	512
#define INITRD_MAGIC_SIZE 6

typedef struct {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
} TAR_HEADER;
#endif

