#include <stdint.h>
#include <stddef.h>
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "string.h"

/*
 * Get time and date
 *
 * In:  timebuf 	struct to store time and date
 *
 *  Returns: 0 on success, -1 on failure
 *
 */
size_t get_time_date(TIME *timebuf) {
size_t handle;

handle=open("CLOCK$",O_RDONLY);		/* open clock device */
if(handle == -1) return(-1);

if(read(handle,timebuf,sizeof(TIME)) == -1) {	/* read time and date */
	close(handle);
	return(-1);
}

close(handle);
}

/*
 * Set time and date
 *
 * In:  timebuf 	struct with time and date
 *
 *  Returns: nothing
 *
 */

size_t set_time_date(TIME *timebuf) {
size_t handle;

handle=open("CLOCK$",O_RDONLY);		/* open clock device */
if(handle == -1) return(-1);

if(write(handle,timebuf,sizeof(TIME)) == -1) {	/* read write and date */
	close(handle);
	return(-1);
}

close(handle);
}

