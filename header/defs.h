#include <stdint.h>
#include "errors.h"

#define CCPVER 0x0100
#define PROCESS_BASE_ADDRESS 1 << (sizeof(size_t)*8)-1

void null_function(void);


typedef struct {
 uint8_t seconds;
 uint8_t minutes;
 uint8_t hours;
 uint8_t day;
 uint8_t month;
 uint16_t year;
}  __attribute__((packed)) TIMEBUF;

	
