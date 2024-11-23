#include <stdint.h>

typedef struct {
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day;
	uint8_t month;
	uint16_t year;
}  __attribute__((packed)) TIMEBUF;

size_t gettime(TIMEBUF *timebuf);
void settime(TIMEBUF *timebuf);
size_t delay_loop(size_t delaycount);
size_t clockio(size_t op,void *buf,size_t size);
size_t clock_init(char *init);

