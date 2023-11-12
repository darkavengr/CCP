
#define ATA_BUFFER_SIZE		32768
#define ATA_DMA_BUFFER_SIZE	512

#define	READ_SECTORS		0x20
#define	WRITE_SECTORS		0x30
#define READ_SECTORS_EXT	0x24
#define WRITE_SECTORS_EXT	0x34
#define FLUSH			0xE7
#define IDENTIFY		0xec

#define READ_SECTORS_DMA	0xC8
#define WRITE_SECTORS_DMA	0x25
#define READ_SECTORS_EXT_DMA	0xCA
#define WRITE_SECTORS_EXT_DMA	0x35

#define ATA_DEBUG

#define _READ 0
#define _WRITE 1

#define ATA_ALT_STATUS_PORT	0
#define ATA_FEATURES_PORT	1
#define ATA_DATA_PORT		0
#define	ATA_ERROR_PORT		1
#define	ATA_SECTOR_COUNT_PORT   2
#define	ATA_SECTOR_NUMBER_PORT  3
#define	ATA_CYLINDER_LOW_PORT 	4
#define	ATA_CYLINDER_HIGH_PORT	5
#define	ATA_DRIVE_HEAD_PORT  	6
#define	ATA_COMMAND_PORT  	7

#define	ATA_ERROR		1
#define ATA_DATA_READY		8
#define ATA_OVERLAPPED_MODE	8
#define ATA_DRIVE_FAULT		32
#define ATA_RDY			64
#define ATA_BUSY		128

#define PRDT_COMMAND_PRIMARY	0
#define PRDT_STATUS_PRIMARY	2
#define PRDT_ADDRESS1_PRIMARY	4
#define PRDT_ADDRESS2_PRIMARY	5
#define PRDT_ADDRESS3_PRIMARY	6
#define PRDT_ADDRESS4_PRIMARY	7

#define PRDT_COMMAND_SECONDARY		8
#define PRDT_STATUS_SECONDARY		0xA
#define PRDT_ADDRESS1_SECONDARY	0xC
#define PRDT_ADDRESS2_SECONDARY	0xD
#define PRDT_ADDRESS3_SECONDARY	0xE
#define PRDT_ADDRESS4_SECONDARY	0xF

#define IOCTL_ATA_IDENTIFY 0

typedef struct {
	uint16_t general_configuration;		//0			
	uint16_t obsolete1;				//1
	uint16_t specific_configuration;		//2
	uint16_t obsolete2;				//3
	uint16_t retired[2];				//5
	uint16_t obsolete3;				//6
	uint16_t reserved_compact_flash[2];		//8
	uint16_t retired2;				//9
	uint16_t serial_number[8];			//18
	uint16_t retired3[2];				//20
	uint16_t obsolete4;				//21
	uint16_t firmware_revision[3];			//24
	uint16_t modelnumber[19];			//35
	uint16_t trusted_computer_feature_set_options; //36
	uint16_t capabilities; 			//37
	uint16_t capabilities2;			//38
	uint16_t obsolete5[2];				//40
	uint16_t valid_fields;				//41
	uint16_t obsolete666[4];			//43
	uint16_t lba28_size;				//44
	uint16_t obso[6];
	uint16_t dma;
	uint16_t dma2;
	uint16_t multiword_dma_transfer_cycle_time;
	uint16_t recommended_multiword_dma_transfer_cycle_time;
	uint16_t minimum_pio_transfer_time_control_flow;
	uint16_t minimum_pio_transfer_time_iordy_control_flow;
	uint16_t addsupport;
	uint16_t reserved999;
	uint16_t reserved7[4];
	uint16_t queuedepth;
	uint16_t serial_ata_capabilities;
	uint16_t reservedforserialata;
	uint16_t serial_ata_features_supported;
	uint16_t serial_ata_features_enabled;
	uint16_t major_version;
	uint16_t minor_version;
	uint16_t commands_and_feature_sets_supported;
	uint16_t commands_and_feature_sets_supported2;
	uint16_t commands_and_feature_sets_supported3;
	uint16_t commands_and_feature_sets_supported4;
	uint16_t commands_and_feature_sets_supported5;
	uint16_t commands_and_feature_sets_supported6;
	uint16_t ultra_dma_modes;
	uint16_t erase_time;
	uint16_t enhanced_erase_time;
	uint16_t current_apm_level_value;
	uint16_t master_password_identify;
	uint16_t hardware_reset_result;
	uint16_t aam_value;
	uint16_t reserved12[12];
	uint16_t stream_minimum_request_size;
	uint16_t stream_transfer_size;
	uint16_t stream_transfer_time;
	uint16_t streaming_access_latency;
	uint16_t streaming_performance_granularity[2];
	uint16_t lba48_size[4];
	uint16_t streaming_transfer_time_pio;
	uint16_t reserved14;
	uint16_t sector_size;
	uint16_t interseek_delay;
	uint16_t world_wide_name[3];
	uint16_t reserved15[3];
	uint16_t reserved16;
	uint16_t logical_sector_size[2];
} ATA_IDENTIFY __attribute__((__packed__));

typedef struct {
	uint32_t address;
	uint16_t size;
	uint16_t last;
} prdt_struct;

size_t ata_init(char *initstring);
size_t ata_io(size_t op,size_t physdrive,uint64_t block,uint16_t *buf);
size_t ata_io_chs (size_t op,size_t physdrive,size_t blocksize,size_t head,size_t cylinder,size_t sector,uint16_t *buf);
size_t ata_ident(size_t physdrive,ATA_IDENTIFY *buf);
size_t ata_io_dma(size_t op,size_t physdrive,uint64_t block,uint16_t *buf);
size_t irq14_handler(void);
size_t irq15_handler(void);
size_t ata_ioctl(size_t handle,unsigned long request,char *buffer);

