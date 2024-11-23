#include <stddef.h>

#define ATAPI_TEST_UNIT_READY	0x00
#define ATAPI_REQUEST_SENSE	0x03
#define ATAPI_FORMAT_UNIT	0x04
#define ATAPI_INQUIRY		0x12
#define ATAPI_START_STOP_UNIT	0x1B
#define ATAPI_PREVENT_ALLOW_MEDIUM_REMOVAL	0x1E
#define ATAPI_READ_FORMAT_CAPACITIES	0x23
#define ATAPI_READ_CAPACITY	0x25
#define ATAPI_READ10		0x28
#define ATAPI_WRITE10		0x2A
#define ATAPI_SEEK		0x2B
#define ATAPI_WRITE_AND_VERIFY	0x2E
#define ATAPI_VERIFY		0x2F
#define ATAPI_SYNCHRONIZE_CACHE	0x35
#define ATAPI_WRITE_BUFFER	0x3B
#define ATAPI_READ_BUFFER	0x3C
#define ATAPI_READ_TOC/PMA/ATIP	0x43
#define ATAPI_GET_CONFIGURATION	0x46
#define ATAPI_GET_EVENT_STATUS_NOTIFICATION	0x4A
#define ATAPI_READ_DISC_INFORMATION	0x51
#define ATAPI_READ_TRACK_INFORMATION	0x52
#define ATAPI_RESERVE_TRACK	0x53
#define ATAPI_SEND_OPC_INFORMATION	0x54
#define ATAPI_MODE_SELECT	0x55
#define ATAPI_REPAIR_TRACK	0x58
#define ATAPI_MODE_SENSE	0x5A
#define ATAPI_CLOSE_TRACK_SESSION	0x5B
#define ATAPI_READ_BUFFER_CAPACITY	0x5C
#define ATAPI_SEND_CUE_SHEET	0x5D
#define ATAPI_PACKET	0xA0
#define ATAPI_BLANK	0xA1
#define ATAPI_SECURITY_PROTOCOL_IN	0xA2
#define ATAPI_SEND_KEY	0xA3
#define ATAPI_REPORT_KEY	0xA4
#define ATAPI_LOAD/UNLOAD_MEDIUM	0xA6
#define ATAPI_SET_READ_AHEAD	0xA7
#define ATAPI_READ12		0xA8
#define ATAPI_WRITE12		0xAA
#define ATAPI_READ_SERIAL_NUMBER	0xAB
#define ATAPI_GET_PERFORMANCE	0xAC
#define ATAPI_READ_DISC_STRUCTURE	0xAD
#define ATAPI_SECURITY_PROTOCOL_OUT	0xB5
#define ATAPI_SET_STREAMING	0xB6
#define ATAPI_READ_CD_MSF	0xB9
#define ATAPI_SET_CD_SPEED	0xBB
#define ATAPI_MECHANISM_STATUS	0xBD
#define ATAPI_READ_CD	0xBE
#define ATAPI_SEND_DISC_STRUCTURE	0xBF

typedef struct {
	uint8_t opcode;
	uint8_t lun;
	uint32_t lba;
	uint8_t reserved;
	uint8_t reserved2;
	uint8_t pmi;
	uint8_t control;
	uint8_t reserved3;
	uint8_t reserved4;
} atapipacket;

typedef struct {
	uint32_t lba;
	uint32_t blocklength;
} CAPACITY_DATA;

size_t atapi_send_command(size_t physdrive,atapipacket *atapi_packet);
size_t atapi_io(size_t op,size_t physdrive,size_t block,uint16_t *buf);
size_t atapi_ident(size_t physdrive,ATA_IDENTIFY *buf);
size_t atapi_init(char *initstring);

