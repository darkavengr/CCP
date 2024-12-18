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
	uint16_t last_accessed_date;		// 18
	uint16_t block_high_word;		// 20
	uint16_t last_modified_time;		// 22
	uint16_t last_modified_date;		// 24
	uint16_t block_low_word;		// 26
	uint32_t filesize;			// 28
} __attribute__((packed)) DIRENT;

typedef struct {
	uint8_t sequence;
	uint8_t firstfive_chars[10];				/* unicode uses two bytes */
	uint8_t attributes;
	uint8_t typeindicator;
	uint8_t checksum;
	uint8_t nextsix_chars[12];
	uint16_t sc_alwayszero;
	uint8_t lasttwo_chars[4];	
}  __attribute__((packed)) LFN_ENTRY;

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
	uint16_t fat32mirrorflags;
	uint16_t fat32filesystemversion;
	uint32_t fat32rootdirstart;
	uint16_t fat32infosector;
	uint16_t fat32bootblockcopy;
	uint8_t fat32reserved[12];
	uint8_t fat32physdriveno;
	uint8_t fat32flags;
	uint8_t fat32signature;
	uint32_t fat32serialno;
	uint8_t fat32volumelabel[11];
	uint8_t fat32ident[8];
} __attribute__((packed)) BPB;


size_t fat_findfirst(char *filename,FILERECORD *buf);							// TESTED
size_t fat_findnext(char *filename,FILERECORD *buf);							// TESTED
size_t fat_find(size_t find_type,char *filename,FILERECORD *buf);					// TESTED
size_t fat_rename(char *filename,char *newname);							// TESTED - MORE
size_t fat_rmdir(char *dirname);									// TESTED
size_t fat_mkdir(char *dirname);									// TESTED
size_t fat_create(char *filename);									// TESTED - MORE						
size_t fat_unlink(char *filename);									// TESTED
size_t fat_read(size_t handle,void *addr,size_t size);							// TESTED
size_t fat_write(size_t handle,void *addr,size_t size);							// TESTED
size_t fat_chmod(char *filename,size_t attribs);							// TESTED
size_t fat_set_file_time_date(char *filename,TIME *create_time_date,TIME *last_modified_time_date,TIME *last_accessed_time_date); // TESTED
size_t fat_find_free_block(size_t drive);								// TESTED
size_t fat_get_start_block(size_t drive,char *name);							// TESTED
size_t fat_get_next_block(size_t drive,uint64_t block);							// TESTED
size_t fat_update_fat(size_t drive,uint64_t block,uint16_t block_high_word,uint16_t block_low_word);	// TESTED - MORE
size_t fat_detect_change(size_t drive);									// TESTED
size_t fat_convert_filename(char *filename,char *outname);						// TESTED
size_t fat_read_long_filename(size_t drive,uint64_t block,size_t entryno,FILERECORD *n);		// TESTED
size_t fat_update_long_filename(size_t type,size_t drive,uint64_t block,size_t entryno,FILERECORD *new);// TESTED
size_t fat_create_short_name(char *filename,char *out);							// TESTED
uint8_t fat_create_long_filename_checksum(char *filename);						// TESTED
size_t fat_unlink_long_filename(char *filename,uint64_t block,size_t entry);				// TESTED
size_t fat_create_long_filename(size_t type,FILERECORD *newname,uint64_t block,size_t entry);		// TESTED
void fat_entry_to_filename(char *filename,char *out);							// TESTED
size_t fat_init(char *i);										// TESTED
size_t fat_create_int(size_t entrytype,char *filename);							// TESTED
size_t fat_create_entry(size_t type,size_t drive,uint64_t rb,size_t entryno,size_t datastart,char *filename,char *blockbuf); // TESTED
size_t fat_is_long_filename(char *filename);								// TESTED
size_t fat_create_subdirectory_entries(char *dirname,size_t datastart,size_t startblock,TIME *time,TIME *date,size_t fattype,size_t drive); // TESTED

