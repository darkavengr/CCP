#include <stdint.h>
#include <stddef.h>

typedef struct {
 uint32_t is_locked;
 size_t owner_process;
} MUTEX;
