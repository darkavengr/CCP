#define VOLUME_FLAG_ACTIVEFAT		1
#define VOLUME_FLAG_VOLUMEDIRTY		2
#define VOLUME_FLAG_MEDIAFAILURE	4
#define VOLUME_FLAG_CLEARTOZERO		8

#define EXFAT_ENTRY_FILE	0x85
#define EXFAT_ENTRY_INFO	0xC0
#define EXFAT_ENTRY_FILENAME	0xC1

#define EXFAT_ATTRIB_READONLY		1
#define EXFAT_ATTRIB_HIDDEN 		2
#define EXFAT_ATTRIB_SYSTEM 		4
#define EXFAT_ATTRIB_DIRECTORY 		16
#define EXFAT_ATTRIB_ARCHIVE		32

typedef struct {
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
	uint16_t FileSystemRevision 	/* Filesystem version number */
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

typedef struct {
        uint8_t entrytype;
        uint8_t entrycount;
        uint8_t pad[2];
        uint32_t attributes;
        uint32_t create_time_date;
	uint32_t last_written_time_date;
	uint32_t last_accessed_time_edate;
        uint8_t pad[12];
}  ExFATFileEntry;

typedef struct {
        uint8_t entrytype;
        uint8_t flags;
        uint8_t unk;
        uint8_t filenamelength;
        uint64_t filesize;
        uint32_t unk;
        uint32_t startblock;
        uint64_t filesize2;
} ExFATFileInfoEntry;

typedef struct {
        uint8_t entrytype;
	uint8_t entrycount;
        uint16_t name[15];
} ExFATFilenameEntry;

