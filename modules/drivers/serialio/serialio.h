#define COMBUFSIZE 4096

#define COM1 0x3f8
#define COM2 0x2f8
#define COM3 0x3e8
#define COM4 0x2e8
#define LPT1 0x3bc
#define LPT2 0x378

#define	_READ	0
#define _WRITE	1	

#define IOCTL_SERIAL_BAUD 0
#define IOCTL_SERIAL_DATABITS 1
#define IOCTL_SERIAL_STOPBITS 2
#define IOCTL_SERIAL_PARITY 3

extern setirqhandler();

typedef struct _port {
 char *name;
 int port;
 int baud;
 int databits;
 int stopbits;
 int parity;
 int interrupts;
 void (*handler)(int,int,int,int,void *);
 char *buffer;
 char *bufptr;
 int portrcount;
 int buffersize;
} port;

typedef struct _parity {
 char *name;
 int val;
} parity;



