#include <stdint.h>
#include "mutex.h"
#include "string.h"

/*
 * Lock mutex
 *
 * In: MUTEX *mutex	Mutex to lock
 *
 * Returns nothing
 * 
 */

void lock_mutex(MUTEX *mutex) {
if(mutex->owner_process == getpid()) return;			/* waiting for already owned mutex */

while(mutex->is_locked == 1) ;;				/* wait for mutex to unlock */
 
asm("movl $1,%eax");
asm volatile("lock cmpxchg %%eax,%0":"=m"(mutex->is_locked): );	/* atomically set mutex->is_locked to 1 */

mutex->owner_process=getpid();
}

/*
 * Unlock mutex
 *
 * In: MUTEX *mutex	Mutex to unlock
 *
 * Returns nothing
 * 
 */

void unlock_mutex(MUTEX *mutex) {
asm("movl $0,%eax");
asm volatile("lock cmpxchg %%eax,%0":"=m"(mutex->is_locked): );	/* atomically set mutex->is_locked to 0 */
}

/*
 * Initialize mutex
 *
 * In: MUTEX *mutex	Mutex to initialize
 *
 * Returns nothing
 * 
 */

void initialize_mutex(MUTEX *mutex) {
mutex->owner_process=getpid();
mutex->is_locked=0;
} 
