#define MOUSE_GET_COMPAQ_STATUS			0x20
#define MOUSE_SET_COMPAQ_STATUS 		0x60
#define MOUSE_ENABLE_AUXILARY_DEVICE 		0xA8
#define MOUSE_SET_SCALING_1_1 			0xE6
#define MOUSE_SET_SCALING_2_1 			0xE7
#define MOUSE_SET_RESOLUTION 			0xE8
#define MOUSE_STATUS_REQUEST 			0xE9
#define MOUSE_SET_STREAM_MODE 			0xEA
#define MOUSE_REQUEST_SINGLE_PACKET 		0xEB
#define MOUSE_RESET_WRAP_MODE			0xEC
#define MOUSE_SET_WRAP_MODE			0xEE
#define MOUSE_GET_MOUSEID 			0xf2
#define MOUSE_SET_SAMPLE_RATE 			0xf3
#define MOUSE_ENABLE_PACKET_STREAMING 		0xf4
#define MOUSE_DISABLE_PACKET_STREAMING 		0xf5
#define MOUSE_SET_DEFAULTS 			0xF6
#define MOUSE_SET_REMOTE_MODE 			0xF0
#define MOUSE_RESEND				0xFE
#define MOUSE_RESET				0xFF

#define MOUSE_X_OVERFLOW_MASK 			0x40
#define MOUSE_Y_SIGN_BIT_MASK 			0x20
#define MOUSE_X_SIGN_BIT_MASK 			0x10
#define MOUSE_Y_ALWAYS1_MASK 			0x8
#define MOUSE_MIDDLE_BUTTON_MASK		0x4
#define MOUSE_RIGHT_BUTTON_MASK			0x2
#define MOUSE_LEFT_BUTTON_MASK 			0x1

#define	MOUSE_LEFT_BUTTON_DOUBLECLICK		0x8
#define MOUSE_DOUBLECLICK_INTERVAL		0x250		/* in milliseconds */

#define MOUSE_Y_OVERFLOW_MASK 			0x80

#define MOUSE_DATA_READY_READ	1
#define MOUSE_DATA_READY_WRITE	2

#define MOUSE_IS_MOUSE_DATA	0x20

#define MOUSE_PORT 0x60
#define MOUSE_STATUS 0x64
#define MOUSE_COMMAND 0x64

#define MOUSE_ENABLE_IRQ12 0x2
#define MOUSE_DISABLE_CLOCK 0x20

#define MOUSE_IOCTL_SET_RESOLUTION 0
#define MOUSE_IOCTL_ENABLE_SCROLL_WHEEL 1
#define MOUSE_IOCTL_ENABLE_EXTRA_BUTTONS 2
#define MOUSE_IOCTL_SET_SAMPLE_RATE 3

size_t mouse_init(char *init);
uint8_t readmouse(void);
void mouse_handler(void);
void wait_for_mouse_read(void);
void wait_for_mouse_write(void);
size_t mouseio_read(void *buf,size_t size);
size_t wait_for_ack(void);
void mouse_send_command(uint8_t command);
void wait_for_mouse_ack(void);
size_t mouse_set_sample_rate(size_t resolution);
size_t mouse_enable_scrollwheel(void);
size_t mouse_set_resolution(size_t resolution);

