#define FAT_ATTRIB_READONLY	1
#define FAT_ATTRIB_HIDDEN 	2
#define FAT_ATTRIB_SYSTEM 	4
#define FAT_ATTRIB_VOLUME_LABEL	8
#define FAT_ATTRIB_DIRECTORY 	16
#define FAT_ATTRIB_ARCHIVE	32

#define FAT_ENTRY_SIZE 	32
#define MAX_BLOCK_SIZE 32768

typedef struct {
 uint8_t filename[11]; 			// 0
 uint8_t attribs; 	  		// 11
 uint8_t reserved;			// 12
 uint8_t create_time_fine_resolution;	// 13
 uint16_t createtime;			// 14
 uint16_t createdate;			// 16
 uint16_t lastaccessdate;		// 18
 uint16_t block_high_word;		// 20
 uint16_t last_modified_time;		// 22
 uint16_t last_modified_date;		// 24
 uint16_t block_low_word;		// 26
 uint32_t filesize;			// 28
} __attribute__((packed)) DIRENT;

struct lfn_entry {
 uint8_t sequence;
 uint8_t firstfive_chars[10];				/* unicode uses two bytes */
 uint8_t attributes;
 uint8_t typeindicator;
 uint8_t checksum;
 uint8_t nextsix_chars[12];
 uint16_t sc_alwayszero;
 uint8_t lasttwo_chars[4];	
}  __attribute__((packed)) lfn;

typedef struct {
/* dos 1.x+ bpb */

 uint8_t jump;
 uint16_t jump2;
 uint8_t fsmarker[8];
 uint16_t sectorsize;
 uint8_t sectorsperblock;
 uint16_t reservedsectors;
 uint8_t numberoffats;
 uint16_t rootdirentries;
 uint16_t numberofsectors;
 uint8_t mediadescriptor;
 uint16_t sectorsperfat;

/* dos 3.0 bpb */

 uint16_t sectorspertrack;
 uint16_t numberofheads;
 uint32_t numberofhiddensectors;
 uint32_t sectors_dword;

 /* dos 3.31 fat16 extended bpb */ 

 uint8_t physicaldriveno; 
 uint8_t reserved;
 uint8_t exbootsig;
 uint32_t serialno;
 uint8_t volumelabel[11];
 uint8_t fatname[8];

 /* fat32 extended bpb */
 
 uint32_t fat32sectorsperfat;
 uint32_t fat32mirrorflags;
 uint32_t fat32filesystemversion;
 uint32_t fat32rootdirstart;
 uint16_t fat32infosector;
 uint16_t fat32bootblockcopy;
 uint32_t fat32reserved[12];
 uint32_t fat32physdriveno;
 uint32_t fat32reserved2;
 uint32_t fat32signature;
 uint32_t fat32serialno;
} __attribute__((packed)) BPB;



