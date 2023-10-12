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
#include "../filemanager/vfs.h"
#include "process.h"

extern PROCESS *currentprocess;
extern PROCESS *processes;

size_t tickcount;
size_t multitaskingenabled=FALSE;

void disablemultitasking(void);
void enablemultitasking(void);
void init_multitasking(void);
PROCESS *find_next_process_to_switch_to(void);
void sched(void);

extern *switch_task();

/*
 * Disable multitasking
 *
 * In: nothing
 *
 * Returns nothing
 * 
 */
void disablemultitasking(void) {
multitaskingenabled=FALSE;

disable_interrupts();
return;
}

/*
 * Enable multitasking
 *
 * In: nothing
 *
 * Returns nothing
 * 
 */
void enablemultitasking(void) {
multitaskingenabled=TRUE;

enable_interrupts();
return;
}

/*
 * Initialize multitasking
 *
 * In: nothing
 *
 * Returns nothing
 * 
 */

void init_multitasking(void) {

multitaskingenabled=FALSE;
setirqhandler(0,switch_task);		/* Register timer */
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
 * Returns: pointer to next process
 * 
 */

PROCESS *find_next_process_to_switch_to(void) { 
if((currentprocess == NULL) || (currentprocess->next == NULL)) return(processes);	/* if at end, loop back to start */

return(currentprocess->next);
}

size_t is_process_ready_to_switch(void) { 
 if(currentprocess == NULL) return(FALSE);

 if(++currentprocess->ticks <= currentprocess->maxticks) return(FALSE);

 return(TRUE);
}

void reset_process_ticks(void) {
 currentprocess->ticks=0;
}

size_t get_tick_count(void) {
 return(tickcount);
}

size_t increment_tick_count(void) {
 tickcount++;
}

void kwait(size_t ticks) {
 size_t newticks;

 newticks=get_tick_count()+ticks;

 while(get_tick_count() < newticks) ;;
}

