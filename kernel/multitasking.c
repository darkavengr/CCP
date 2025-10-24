/*  CCP Version 0.0.1
    (C) Matthew Boote 2020-2023

    This file is part of CCP.
    CCP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    CCP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with CCP.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stddef.h>
#include "mutex.h"
#include "vfs.h"
#include "process.h"
#include "string.h"
#include "multitasking.h"
#include "debug.h"

size_t multitaskingenabled=FALSE;

extern void switch_task(void *regs);

/*
 * Disable multitasking
 *
 * In: nothing
 *
 * Returns: nothing
 * 
 */
void disablemultitasking(void) {
//multitaskingenabled=FALSE;
//return;
}

/*
 * Enable multitasking
 *
 * In: nothing
 *
 * Returns: nothing
 * 
 */
void enablemultitasking(void) {
//multitaskingenabled=TRUE;
return;
}

/*
 * Initialize multitasking
 *
 * In: nothing
 *
 * Returns: nothing
 * 
 */

void init_multitasking(void) {
multitaskingenabled=FALSE;

//setirqhandler(0,'SCHD',&switch_task);		/* Register task switcher */
return;
}

/*
 * Get if multitasking enabled
 *
 * In: nothing
 *
 * Returns: true or false
 * 
 */

size_t is_multitasking_enabled(void) {
return(multitaskingenabled);
}

/*
 * Find next process to switch to
 *
 * In: nothing
 *
 * Returns: pointer to next process or NULL on error
 * 
 */

PROCESS *find_next_process_to_switch_to(void) { 
PROCESS *newprocess;

if(get_current_process_pointer() == NULL) return(get_processes_pointer());

newprocess=get_current_process_pointer();

/* find next process */
newprocess=newprocess->next;

if(newprocess == NULL) newprocess=get_processes_pointer();	/* if at end, loop back to start */

return(newprocess);
}

/*
 * Get if process is ready to switch
 *
 * In: nothing
 *
 * Returns: true or false
 * 
 */

size_t is_current_process_ready_to_switch(void) { 
if(get_processes_pointer() == NULL) return(FALSE);

if(timer_increment() < get_process_max_tick_count()) return(FALSE);

return(TRUE);
}

