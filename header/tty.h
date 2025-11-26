#include <stdint.h>
#include <stddef.h>

#define TTY_DEVICE "CON"
#define TTY_MODE_RAW	1
#define TTY_MODE_ECHO	2

#define IOCTL_SET_TTY_MODE_RAW		1
#define IOCTL_CLEAR_TTY_MODE_RAW	2
#define IOCTL_SET_TTY_MODE_ECHO		3
#define IOCTL_CLEAR_TTY_MODE_ECHO	4

#ifndef TTY_H
#define TTY_H
typedef struct {
	size_t (*ttyread)(char *addr,size_t size);
	size_t (*ttywrite)(char *addr,size_t size);
	struct TTL_HANDLER *next;
} TTY_HANDLER;
#endif

size_t tty_ioctl(size_t handle,unsigned long request,char *buffer);
size_t tty_write(char *data,size_t size);
size_t tty_read(char *data,size_t size);
size_t tty_init(void);
size_t tty_register_read_handler(TTY_HANDLER *read_handler);
size_t tty_register_write_handler(TTY_HANDLER *write_handler);
void tty_set_default_read_handler(TTY_HANDLER *handler);

