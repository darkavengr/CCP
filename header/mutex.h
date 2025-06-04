#include <stdint.h>
#include <stddef.h>

#ifndef MUTEX_H
#define MUTEX_H
typedef struct {
	size_t is_locked;
	size_t owner_process;
} MUTEX;
#endif

void lock_mutex(MUTEX *mutex);
void unlock_mutex(MUTEX *mutex);
void initialize_mutex(MUTEX *mutex);
