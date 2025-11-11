#include "pagesize.h"

#ifndef PROCESS_H
#define PROCESS_H

#define PROCESS_NEW	0
#define	PROCESS_READY	1
#define PROCESS_RUNNING	2
#define	PROCESS_BLOCKED	4
#define PROCESS_FORCE_SWITCH	8

#define PROCESS_FLAG_BACKGROUND		1

#define _SHUTDOWN	 		0
#define _RESET				1
#define DEFAULT_KERNEL_STACK_SIZE 65536
#define DEFAULT_USER_STACK_SIZE PAGE_SIZE

#define DEFAULT_QUANTUM_COUNT 100

#define SHUTDOWN_WAIT 10
#define SIGNAL_COUNT 256

#define ENVIROMENT_SIZE 32768

typedef struct { 
	size_t pid;
	size_t ticks;	
	size_t maxticks;
	struct PROCESS *blockedprocess;		
	uint8_t filename[MAX_PATH];		
	uint8_t args[MAX_PATH];		
	size_t parentprocess;
	size_t (*errorhandler)(char *,size_t,size_t,size_t);
	size_t (*signalhandler)(size_t);
	uint8_t currentdirectory[MAX_PATH];		
	size_t childprocessreturncode;			
	size_t flags;
	size_t kernelstackpointer;
	size_t kernelstacktop;
	size_t kernelstackbase;
	size_t stackpointer;
	size_t stackbase;
	size_t lasterror;
	char *envptr;	
	struct PROCESS *next;
}  __attribute__((packed)) PROCESS;

typedef struct {
	uint8_t slack[127]; 
	uint8_t cmdlinesize;
	uint8_t commandline[127];			/* command line */
} __attribute__((packed)) PSP;

typedef struct {
	 uint8_t name[MAX_PATH];
	 uint8_t magic[MAX_PATH];
	 size_t magicsize;
	 size_t (*callexec)(char *);
	 struct EXECUTABLEFORMAT *next;
} EXECUTABLEFORMAT;
#endif

size_t exec(char *filename,char *argsx,size_t flags);
size_t kill(size_t process);
size_t exit(size_t val);
void shutdown(size_t shutdown_status); 
size_t findfirstprocess(PROCESS *processbuf);
size_t findnextprocess(PROCESS *processbuf);
size_t wait(size_t pid);
size_t dispatchhandler(void *argsix,void *argfive,void *argfour,void *argthree,void *argtwo,size_t argone);
size_t getcwd(char *dir);
size_t chdir(char *dirname);
size_t getpid(void);
void setlasterror(size_t err);
size_t getlasterror(void);
size_t getprocessfilename(char *buf);
size_t getwriteconsolehandle(void);
size_t getreadconsolehandle(void);
void getprocessargs(char *buf);
size_t getppid(void);
size_t getreturncode(void);
void ksleep(size_t wait);
void signal(void *handler);
size_t sendsignal(size_t process,size_t signal);
size_t set_critical_error_handler(void *addr);
size_t call_critical_error_handler(char *name,size_t drive,size_t flags,size_t error);
void processmanager_init(void);
size_t blockprocess(size_t pid);
size_t unblockprocess(size_t pid);
char *getenv();
void save_kernel_stack_pointer(size_t new_stack_pointer);
size_t get_kernel_stack_pointer(void);
size_t get_kernel_stack_top(void);
PROCESS *get_processes_pointer(void);
void update_current_process_pointer(PROCESS *ptr);
PROCESS *get_current_process_pointer(void);
PROCESS *get_next_process_pointer(void);
size_t register_executable_format(EXECUTABLEFORMAT *format);
size_t load_executable(char *filename);
void reset_current_process_ticks(void);
size_t get_tick_count(void);
size_t increment_tick_count(void);
size_t GetVersion(void);
size_t get_usermode_stack_base(void);
size_t get_usermode_stack_top(void);
void set_usermode_stack_base(void *base);

