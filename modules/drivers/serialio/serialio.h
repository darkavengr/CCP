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
	void (*readhandler)(int,int,int,int,void *);
	void (*writehandler)(int,int,int,int,void *);
	char *buffer;
	char *bufptr;
	int portrcount;
	int buffersize;
} port;

typedef struct _parity {
	char *name;
	int val;
} parity;


void serialio_init(char *init);
uint8_t comport(size_t op,uint16_t port,char *buf,size_t size);
void read_com1(char *buf,size_t size);
void write_com1(char *buf,size_t size);
void read_com2(char *buf,size_t size);
void write_com2(char *buf,size_t size);
void read_com3(char *buf,size_t size);
void write_com3(char *buf,size_t size);
void read_com4(char *buf,size_t size);
void write_com4(char *buf,size_t size);
void com1_irq_handler(void *regs);
void com2_irq_handler(void *regs);
void com3_irq_handler(void *regs);
void com4_irq_handler(void *regs);
size_t serial_ioctl(size_t handle,unsigned long request,char *buffer);

