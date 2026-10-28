/* CCP Version 0.0.1
  (C) Matthew Boote 2020-2022

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
  along with CCP.  If not, see <https:www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include "mutex.h"

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
 
 asm("movq $1,%rax");
 asm volatile("lock cmpxchg %%rax,%0":"=m"(mutex->is_locked): );	/* atomically set mutex->is_locked to 1 */

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
