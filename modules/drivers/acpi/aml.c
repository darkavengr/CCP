#include <stdint.h>
#include <stddef.h>

#include "kernelhigh.h"
#include "acpi.h"

size_t parse_aml(void *buf) {
	switch(*buf) {

		case 0:			/* nop */
			break;

		case 1:
			break;
	}
}

