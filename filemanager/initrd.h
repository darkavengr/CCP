#include <stddef.h>

typedef struct {
 char filename[100];
 uint64_t mode;
 uint64_t userid;
 char *size[12];			/* size in ascii */
 char *lmtime[12];			/* also in ascii */
 uint64_t checksum;
 uint8_t linkindicator;
 char *linkname[100];
 uint8_t  type;
 char *ustar[6];
 uint16_t version;
 char *username[32];
 char *groupname[32];
 uint64_t device_major;
 uint64_t device_minor;
 char *filenameprefix[155];
} TAR_HEADER;

