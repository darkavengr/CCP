#include <stdint.h>

#ifndef TIME_H
#define TIME_H
typedef struct {
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint8_t day;
	uint8_t month;
	uint16_t year;
}  __attribute__((packed)) TIME;
#endif

