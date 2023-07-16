#define PROCESS_NEW	0
#define	PROCESS_READY	1
#define PROCESS_RUNNING	2
#define	PROCESS_BLOCKED	4

#define PROCESS_FLAG_BACKGROUND		1

#define CCPVER 0x0100
#define _SHUTDOWN	 		0
#define _RESET				1
#define PROCESS_STACK_SIZE 65536
#define DEFAULT_QUANTUM_COUNT 1000

#define SHUTDOWN_WAIT 10
#define SIGNAL_COUNT 256

#define MAX_REGS 256

#define ENVIROMENT_SIZE 32768

extern tss_esp0;



typedef struct { 
 size_t regs[MAX_REGS];		// not same size as hardware-dependent number of registers!
 size_t pid;
 size_t ticks;	
 size_t maxticks;
 struct PROCESS *next;		
 uint8_t filename[MAX_PATH];		
 uint8_t args[MAX_PATH];		
 size_t parentprocess;
 size_t (*errorhandler)(char *,size_t,size_t,size_t);
 size_t (*signalhandler)(size_t);
 uint8_t currentdir[MAX_PATH];		
 size_t rc;			
 size_t flags;			
 size_t writeconsolehandle;	
 size_t readconsolehandle;	
 size_t kernelstackpointer;
 size_t kernelstackbase;
 size_t stackpointer;
 size_t stackbase;
 size_t lasterror;
 char *envptr;	
 struct PROCESS *findptr;
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

