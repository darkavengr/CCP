#include <stdint.h>
#include <stddef.h>
#include "device.h"
#include "timer.h"

size_t timer_count=0;
size_t timer_internal=0;

size_t timer_init(char *initstr) {
setirqhandler(0,'TIMR',&timer_increment);		/* Register timer */

timer_count=0;
timer_internal=0;
}

/*
 * Increment timer count
 *
 * In: nothing
 *
 * Returns: noting
 * 
 */
size_t timer_increment(void) {
if(++timer_internal % TIMER_PERIOD) {
	++timer_count;

	//kprintf_direct("Timer: %d\n",timer_count);
}

return(timer_count);
}

/*
 * Get timer count
 *
 * In: nothing
 *
 * Returns: Timer count
 * 
 */
size_t get_timer_count(void) {
return(timer_count);
}

size_t get_timer_period(void) {
return(TIMER_PERIOD);
}

