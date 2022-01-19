#define PROCESS_NEW	0
#define	PROCESS_READY	1
#define PROCESS_RUNNING	2
#define	PROCESS_BLOCKED	4
#define PROCESS_WAITING 8

#define PROCESS_FLAG_BACKGROUND		1

#define CCPVER 0x0100
#define _SHUTDOWN	 		0
#define _RESET				1
#define PROCESS_STACK_SIZE 65536
#define DEFAULT_QUANTUM_COUNT 30

#define SHUTDOWN_WAIT 10
#define SIGNAL_COUNT 256

#define MAX_REGS 256

extern tss_esp0;

unsigned int status;

typedef struct { 
 size_t regs[MAX_REGS];		// not same size as hardware-dependent number of registers!
 size_t pid;			
 uint8_t filename[MAX_PATH];		
 uint8_t args[MAX_PATH];		
 size_t parentprocess;
 unsigned int (*errorhandler)(char *,unsigned int,unsigned int,unsigned int);
 unsigned int (*signalhandler)(unsigned int);
 uint8_t currentdir[MAX_PATH];		
 unsigned int rc;			
 size_t flags;			
 size_t writeconsolehandle;	
 size_t readconsolehandle;	
 unsigned int lasterror;	
 size_t ticks;	
 size_t maxticks;
 struct PROCESS *findptr;
 size_t stackpointer;
 size_t stackbase;
 size_t kernelstackbase;	
 size_t kernelstackpointer;
 struct PROCESS *next;
}  __attribute__((packed)) PROCESS;

typedef struct {
 uint8_t slack[127]; 
 uint8_t cmdlinesize;
 uint8_t commandline[127];			/* command line */
} __attribute__((packed)) PSP;

typedef struct {
 char *name[MAX_PATH];
 char *magicnumber[MAX_PATH];
 unsigned int size;
 unsigned int location;
 unsigned int (*load)(size_t,char *);			/* handlers */
 struct EXECUTABLE_FORMAT *next;
} EXECUTABLE_FORMAT;

