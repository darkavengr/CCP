#include <stdint.h>
	
#define EBDA_START_ADDRESS 0x00100000+KERNEL_HIGH
#define RDSP_START_ADDRESS 0x000E0000+KERNEL_HIGH
#define RDSP_END_ADDRESS 0x000FFFFF+KERNEL_HIGH
	
typedef struct {
	uint8_t signature[8];
	uint8_t checksum;
	uint8_t OEMID[6];
	uint8_t revision;
	uint32_t rsdt_address;
	uint32_t length;
	uint64_t xsdt_address;
	uint8_t extended_checksum;
	uint8_t reserved[3];
} RSDP;
	
	
typedef struct {
	RSDP	header;
	uint32_t FirmwareCtrl;
	uint32_t Dsdt;
		
	// field used in ACPI 1.0; no longer in use, for compatibility only
	uint8_t  Reserved;
		
	uint8_t  PreferredPowerManagementProfile;
	uint16_t SCI_Interrupt;
	uint32_t SMI_CommandPort;
	uint8_t  AcpiEnable;
	uint8_t  AcpiDisable;
	uint8_t  S4BIOS_REQ;
	uint8_t  PSTATE_Control;
	uint32_t PM1aEventBlock;
	uint32_t PM1bEventBlock;
	uint32_t PM1aControlBlock;
	uint32_t PM1bControlBlock;
	uint32_t PM2ControlBlock;
	uint32_t PMTimerBlock;
	uint32_t GPE0Block;
	uint32_t GPE1Block;
	uint8_t  PM1EventLength;
	uint8_t  PM1ControlLength;
	uint8_t  PM2ControlLength;
	uint8_t  PMTimerLength;
	uint8_t  GPE0Length;
	uint8_t  GPE1Length;
	uint8_t  GPE1Base;
	uint8_t  CStateControl;
	uint16_t WorstC2Latency;
	uint16_t WorstC3Latency;
	uint16_t FlushSize;
	uint16_t FlushStride;
	uint8_t  DutyOffset;
	uint8_t  DutyWidth;
	uint8_t  DayAlarm;
	uint8_t  MonthAlarm;
	uint8_t  Century;
		
	// reserved in ACPI 1.0; used since ACPI 2.0+
	uint16_t BootArchitectureFlags;
		
	uint8_t  Reserved2;
	uint32_t Flags;
		
	// 12 byte structure; see below for details
	GenericAddressStructure ResetReg;
		
	uint8_t  ResetValue;
	uint8_t  Reserved3[3];
		
	// 64bit pointers - Available on ACPI 2.0+
	uint64_tX_FirmwareControl;
	uint64_tX_Dsdt;
		
	GenericAddressStructure X_PM1aEventBlock;
	GenericAddressStructure X_PM1bEventBlock;
	GenericAddressStructure X_PM1aControlBlock;
	GenericAddressStructure X_PM1bControlBlock;
	GenericAddressStructure X_PM2ControlBlock;
	GenericAddressStructure X_PMTimerBlock;
	GenericAddressStructure X_GPE0Block;
	GenericAddressStructure X_GPE1Block;
} FADT;
	
// MADR SRAT SSDT
	
typedef struct {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
	struct RSDT *rsdt_entries;
} RSDT;
	
typedef struct {
	RSDP header;
	uint32_t localapic;
	uint32_t flags
} MADT;
	
typedef struct {
	RSDP header;
	void *amlptr;
} DSDT;
	
typedef struct {
	RSDP header;
	ProcessorLocalAPIC pla;
} SRAT;

typedef struct {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
	uint64_t *xsdt_entries;
} XSDT;

