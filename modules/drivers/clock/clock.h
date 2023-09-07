#include <stdint.h>

#define _READ 0
#define _WRITE 1

typedef struct {
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day;
	uint8_t month;
	uint16_t year;
}  __attribute__((packed)) TIMEBUF;


