#include <stdint.h>
#include <stddef.h>

#define	DIR_COUNT	10

#define RUN_FOREGROUND	0
#define RUN_BACKGROUND	1

#define MAX_READ_SIZE	512

#define COMMAND_TOKEN_COUNT 		20

#define RUN_COMMAND_AND_EXIT		1
#define RUN_COMMAND_AND_CONTINUE	2
#define COMMAND_PERMANENT		4

#define COMMAND_VERSION_MAJOR		1
#define COMMAND_VERSION_MINOR		0

#define TERMINATING 			2

#define MAX_PATH		255
#define O_RDONLY 		1
#define O_WRONLY 		2
#define O_CREAT			4
#define O_NONBLOCK		8
#define O_SHARED		16
#define O_TRUNC			32
#define O_RDWR 			O_RDONLY | O_WRONLY

#define MAX_PATH		255
#define VFS_MAX 		1024

#define	_READ 			0
#define _WRITE 			1

#define stdin			0
#define stdout			1
#define stderr			2

unsigned long doline(char *buf);
unsigned long copyfile(char *source,char *destination);
unsigned long getvar(char *name,char *buf);
unsigned long runcommand(char *fname,char *args,unsigned long backg);
unsigned long setvar(char *name,char *val);
unsigned long doline(char *buf);
size_t set_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t if_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t cd_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t copy_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t for_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t del_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t mkdir_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t rmdir_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t rename_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t echo_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t exit_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t type_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t dir_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t ps_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t kill_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t goto_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
size_t critical_error_handler(char *name,size_t drive,size_t flags,size_t error);
size_t rem_command(size_t tc,char *parsebuf[MAX_PATH][MAX_PATH]);
void signal_handler(size_t signalno);
size_t critical_error_handler(char *name,size_t drive,size_t flags,size_t error);
void write_error(void);
unsigned long runcommand(char *filename,char *args,unsigned long backg);
char *get_buf_pointer(void);
size_t get_batch_mode(void);
size_t set_batch_mode(size_t bm);
void set_current_batchfile_pointer(char *b);

