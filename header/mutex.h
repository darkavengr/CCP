#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint32_t is_locked;
	size_t owner_process;
} MUTEX;

void lock_mutex(MUTEX *mutex);
void unlock_mutex(MUTEX *mutex);
void initialize_mutex(MUTEX *mutex);
