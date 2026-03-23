#define VOLUME_FLAG_ACTIVEFAT		1
#define VOLUME_FLAG_VOLUMEDIRTY		2
#define VOLUME_FLAG_MEDIAFAILURE	4
#define VOLUME_FLAG_CLEARTOZERO		8

#define EXFAT_ENTRY_FILE	0x85
#define EXFAT_ENTRY_INFO	0xC0
#define EXFAT_ENTRY_FILENAME	0xC1
#define EXFAT_BITMAP_ENTRY	0x81

#define EXFAT_ATTRIB_READONLY		1
#define EXFAT_ATTRIB_HIDDEN 		2
#define EXFAT_ATTRIB_SYSTEM 		4
#define EXFAT_ATTRIB_DIRECTORY 		16
#define EXFAT_ATTRIB_ARCHIVE		32

#define EXFAT_ENTRY_SIZE		32

#define EXFAT_NAME_SEGMENT_LENGTH	15

#define EXFAT_ALLOCATION_POSSIBLE	1
#define EXFAT_NO_FAT_CHAIN		2
#define EXFAT_FILE_CONTIGUOUS		4

#define MAX_BLOCK_SIZE			32768

#ifndef EXFAT_H
	#define EXFAT_H

	typedef struct __attribute__((packed)) {
		uint8_t jump;			/* jump instruction */
		uint16_t jump2;
		uint8_t zero[53];
		uint64_t PartitionOffset;	/* start sector */
		uint64_t VolumeLength;		/* volume size */
		uint32_t FATOffset;		/* FAT start sector */
		uint32_t FATLength;		/* FAT length */
		uint32_t ClusterHeapOffset;	/* Cluster heap start sector */
		uint32_t ClusterCount;		/* Cluster heap size in blocks */
		uint32_t RootDirectoryStart;	/* Root directory start block */
		uint32_t VolumeSerialNumber;	/* Volume serial number */	
		uint16_t FileSystemRevision; 	/* Filesystem version number */
		uint16_t VolumeFlags;		/* Volume flags */
		uint8_t BytesPerSectorShift;	/* Shift value to calculate bytes per sector or sectors per bytes */
		uint8_t BytesPerBlockShift;	/* Shift value to calculate bytes per block or blocks per bytes */
		uint8_t NumberOfFATs;		/* Number of FATs */
		uint8_t Drive;			/* Ignore this */
		uint8_t PercentInUse;		/* Percentage of drive used */
		uint8_t Reserved;
		uint8_t boot[390];
		uint16_t signature;		/* Boot signature 0x55 0xAA */
	} ExFATBootSector;

	typedef struct __attribute__((packed)) {
        	uint8_t entrytype;
        	uint8_t entrycount;
        	uint8_t pad[2];
        	uint32_t attributes;
        	uint32_t create_time_date;
		uint32_t last_written_time_date;
		uint32_t last_accessed_time_date;
        	uint8_t pad2[12];
	}  ExFATFileEntry;

	typedef struct __attribute__((packed)) {
        	uint8_t entrytype;
        	uint8_t flags;
        	uint8_t reserved1;
        	uint8_t filenamelength;
		uint16_t filenamehash;
		uint16_t reserved2;
        	uint64_t validfilesize;
        	uint32_t reserved3;
        	uint32_t startblock;
        	uint64_t filesize;
	} ExFATFileInfoEntry;

	typedef struct __attribute__((packed)) {
        	uint8_t entrytype;
		uint8_t entrycount;
        	uint16_t name[EXFAT_NAME_SEGMENT_LENGTH];
	} ExFATFilenameEntry;

	typedef struct __attribute__((packed)) {
		uint8_t entrytype;
		uint8_t flags;
		uint8_t reserved[18];
		uint32_t startblock;
		uint64_t length;
	} ExFATBitmapEntry;
#endif

size_t exfat_init(char *initstr);
size_t exfat_findfirst(char *filename,FILERECORD *buf);
size_t exfat_findnext(char *filename,FILERECORD *buf);
size_t exfat_find(size_t find_type,char *filename,FILERECORD *buf);
size_t exfat_read(size_t handle,void *addr,size_t size);
size_t exfat_write(size_t handle,void *addr,size_t size);
size_t exfat_rename(char *filename,char *newname);
size_t exfat_unlink(char *filename);
size_t exfat_mkdir(char *dirname);
size_t exfat_create(char *filename);
size_t exfat_create_int(size_t type,char *filename);
size_t exfat_rmdir(char *dirname);
size_t exfat_chmod(char *filename,size_t attributes);
size_t exfat_touch(char *filename,TIME *create_time_date,TIME *last_modified_time_date,TIME *last_accessed_time_date);
size_t exfat_get_bitmap_information(size_t drive,ExFATBitmapEntry *entry);
uint64_t exfat_find_free_bitmap_entries(size_t drive,size_t numberofentries);
size_t exfat_update_bitmap_entry(size_t drive,size_t entry,size_t SetOrClear);
size_t exfat_get_bitmap_entry(size_t drive,size_t entry);
size_t exfat_convert_bitmap_to_fat_chain(size_t drive,size_t startentry);
size_t exfat_get_start_block(size_t drive,char *name);
size_t exfat_get_next_block(size_t drive,uint64_t block);
size_t exfat_update_fat(size_t drive,uint64_t block,uint64_t blockentry);
size_t exfat_detect_change(size_t drive);

