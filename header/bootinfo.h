#include <stddef.h>
#include <stdint.h>

#define BOOT_INFO_ADDRESS		0xAC

typedef struct {
	uint8_t physicaldrive;
	uint8_t drive;
	uint8_t cursor_row;
	uint8_t cursor_col;
	uint32_t kernel_start;
	uint32_t kernel_size;
	uint32_t initrd_start;
	uint32_t initrd_size;
	uint32_t symbol_start;
	uint32_t symbol_size;
	uint32_t number_of_symbols;
	uint64_t memorysize;
	uint32_t boot_drive_start_lba;	/* Used by the loader */
} __attribute__((packed)) BOOT_INFO;

