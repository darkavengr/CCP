#define DSP_MIXER_PORT 0x224
#define DSP_MIXER_DATA_PORT 0x225
#define DSP_RESET 0x226
#define DSP_READ 0x22A
#define DSP_WRITE 0x22C
#define DSP_READ_STATUS 0x22E
#define DSP_16BIT_INT_ACK 0x22F

#define SB_SET_TIME_CONSTANT 0x40
#define SB_SET_SAMPLE_RATE 0x41
#define SB_SPEAKER_ON 0xD1
#define SB_SPEAKER_OFF 0xD3
#define SB_8BIT_STOP 0xD0
#define SB_8BIT_RESUME 0xD4
#define SB_16BIT_STOP 0xD5
#define SB_16BIT_RESUME 0xD6
#define SB_GET_VERSION 0xE1
#define SB_SET_VOLUME 0x22

#define SB_SINGLE_MODE 0x48
#define SB_AUTO_MODE 0x58 

#define SB_DMA_BUFFER_DEFAULT_SIZE  16256
#define SB_DEFAULT_IRQ 5
#define SB_DEFAULT_CHANNEL 1
#define SB_DEFAULT_TIME_CONSTANT 0x40
#define SB_DEFAULT_SAMPLE_RATE 256000 - (65536/5)
#define SB_DEFAULT_DATA_TYPE 0

#define IOCTL_SB16_IRQ 0
#define IOCTL_SB16_SET_SAMPLE_RATE 1
#define IOCTL_SB16_SET_CHANNEL 2
#define IOCTL_SB16_SET_VOLUME 3
#define IOCTL_SB16_SET_TIME_CONSTANT 4
#define IOCTL_SB16_SET_DATATYPE 5

#define SB_CHANNEL 0
#define SB_START 1
#define SB_COUNT 2
#define SB_PAGE 3

size_t sb16_init(char *initstring);
size_t sb16_io_read(char *buf,size_t len);
size_t sb16_ioctl(size_t handle,unsigned long request,char *buffer);
void sb_irq_handler(void);

