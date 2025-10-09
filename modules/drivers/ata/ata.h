
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
#define ATA_STATUS_PORT		7

#define	ATA_ERROR		1
#define ATA_IDX			2
#define ATA_CORR		4
#define ATA_DATA_READY		8
#define ATA_OVERLAPPED_MODE	16
#define ATA_DRIVE_FAULT		32
#define ATA_RDY			64
#define ATA_BUSY		128

#define ATA_CONTROL_PORT	0x206

#define PRDT_COMMAND_PRIMARY	0
#define PRDT_STATUS_PRIMARY	2
#define PRDT_ADDRESS_PRIMARY	4

#define PRDT_COMMAND_SECONDARY		8
#define PRDT_STATUS_SECONDARY		0xA
#define PRDT_ADDRESS_SECONDARY	0xC

#define IOCTL_ATA_IDENTIFY 0

#define ATA_IRQ_TIMEOUT	    300	   /* milliseconds to wait for IRQ 14 or 15 to arrive */

typedef struct {
	uint16_t general_configuration;		//0			
	uint16_t obsolete1;				//1
	uint16_t specific_configuration;		//2
	uint16_t obsolete2;				//3
	uint16_t retired1;				//4
	uint16_t retired2;				//5
	uint16_t obsolete3;				//6
	uint16_t reserved_compact_flash1;		//7
	uint16_t reserved_compact_flash2;		//8
	uint16_t retired3;				//9
	uint16_t serial_number[10];			//10
	uint16_t retired4;				//20
	uint16_t retired5;				//21
	uint16_t obsolete4;				//22
	uint16_t firmware_revision[4];			//23
	uint16_t modelnumber[20];			//27
	uint16_t transfer_config;			//47
	uint16_t reserved;				//48
	uint16_t capabilities; 				//49
	uint16_t capabilities2;				//50
	uint16_t obsolete5;				//51
	uint16_t obsolete6;				//52
	uint16_t valid_fields;				//53
	uint16_t obsolete666[4];			//54
	uint16_t reserved1;				//59
	uint32_t lba28_size;				//60
	uint16_t obsolete7;				//62
	uint16_t dma;					//63
	uint16_t dma2;					//64
	uint16_t multiword_dma_transfer_cycle_time;	//65
	uint16_t recommended_multiword_dma_transfer_cycle_time; //66
	uint16_t minimum_pio_transfer_time_control_flow; //67
	uint16_t minimum_pio_transfer_time_iordy_control_flow; //68
	uint16_t reserved2;				//69
	uint16_t reserved3;				//70
	uint16_t reserved_atapi1;			//71
	uint16_t reserved_atapi2;			//72
	uint16_t reserved_atapi3;			//73
	uint16_t reserved_atapi4;			//74
	uint16_t queuedepth;				//75
	uint16_t reserved_serial_ata1;			//76
	uint16_t reserved_serial_ata2;			//77
	uint16_t reserved_serial_ata3;			//78
	uint16_t reserved_serial_ata4;			//79
	uint16_t major_version_number;			//80
	uint16_t minor_version_number;			//81
	uint16_t serial_ata_features_supported;		//82
	uint16_t commands_and_feature_sets_supported;	//83
	uint16_t commands_and_feature_sets_supported2;	//84
	uint16_t commands_and_feature_sets_supported3;	//85
	uint16_t commands_and_feature_sets_supported4;	//86
	uint16_t commands_and_feature_sets_supported5;	//87
	uint16_t reserved4;				//88
	uint16_t erase_time;				//89
	uint16_t enhanced_erase_time;			//90
	uint16_t current_apm_level_value;		//91
	uint16_t master_password_identify;		//92
	uint16_t hardware_reset_result;			//93
	uint16_t aam_value;				//94
	uint16_t stream_minimum_request_size;		//95
	uint16_t stream_transfer_time_dma;		//96
	uint16_t stream_access_latency;			//97
	uint16_t streaming_performance_granularity1;	//98
	uint16_t streaming_performance_granularity2;	//99
	uint64_t lba48_size;				//100
	uint16_t streaming_transfer_time_pio;		//104
	uint16_t reserved5;				//105
	uint16_t sector_size;				//106
	uint16_t interseek_delay;			//107
	uint16_t naa_ieee1;				//108
	uint16_t naa_ieee2;				//109
	uint16_t naa_ieee3;				//110
	uint16_t naa_ieee4;				//111
	uint16_t world_wide_name[3];			//112
	uint16_t reserved6;				//116
	uint16_t logical_sector_size[2];		//117
	uint16_t supported_settings;			//119
	uint16_t enabled_settings;			//120
	uint16_t settings_reserved1;			//121
	uint16_t settings_reserved2;			//122
	uint16_t settings_reserved3;			//123
	uint16_t settings_reserved4;			//124
	uint16_t settings_reserved5;			//125
	uint16_t settings_reserved6;			//126
	uint16_t removable_media_notification;		//127
	uint16_t security_status;			//128
	uint16_t vendor_specific[30];			//129
	uint16_t cfa_power_mode;			//160
	uint16_t reserved7[14];				//161
	uint16_t media_serial_number[30];		//162
	uint16_t sct_command_transport;			//206
	uint16_t reserved8[2];				//207
	uint16_t sector_block_alignment;		//209
	uint16_t write_read_verify_sector_count_mode3[2];	// 210
	uint16_t verify_sector_count_mode_2[2];		//212
	uint16_t reserved9;				//214
	uint16_t reserved10[6];				//215
	uint16_t transport_major_version;		//223
	uint16_t reserved11[9];				//224
	uint16_t reserved12[21];			//234
	uint16_t integrity_word;			//255
} ATA_IDENTIFY __attribute__((__packed__));

typedef struct {
	uint32_t address;
	uint16_t size;
	uint16_t last;
} prdt_struct;

size_t ata_init(char *initstring);
size_t ata_pio(size_t op,size_t physdrive,uint64_t block,uint16_t *buf);
size_t ata_chs (size_t op,size_t physdrive,size_t blocksize,size_t head,size_t cylinder,size_t sector,uint16_t *buf);
size_t ata_ident(size_t physdrive,ATA_IDENTIFY *buf);
size_t ata_dma(size_t op,size_t physdrive,uint64_t block,uint16_t *buf);
size_t irq14_handler(void);
size_t irq15_handler(void);
size_t ata_ioctl(size_t handle,unsigned long request,char *buffer);
size_t ata_irq_handler_internal(size_t irqnumber);
size_t wait_for_ata_irq(size_t irqnumber);
