#include <stddef.h>
#include <stdint.h>

#define BOOT_INFO_ADDRESS		0xAC

typedef struct {
	uint8_t drive;
	uint8_t cursor_row;
	uint8_t cursor_col;
	size_t kernel_start;
	size_t kernel_size;
	size_t initrd_start;
	size_t initrd_size;
	size_t symbol_start;
	size_t symbol_size;
	size_t number_of_symbols;
	uint64_t memorysize;
} __attribute__((packed)) BOOT_INFO;

