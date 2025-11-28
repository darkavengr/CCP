#define PIT_VAL 		1193180 / get_timer_period()				/* frequency of IRQ 0 called by PIT */
#define PIT_CHANNEL_0_REGISTER	0x40
#define PIT_CHANNEL_1_REGISTER	0x41
#define PIT_CHANNEL_2_REGISTER	0x42
#define PIT_COMMAND_REGISTER	0x43


void pit_init(char *init);
size_t pit_io(size_t op,size_t *buf,size_t ignored);
size_t pit_read(size_t size,size_t *buf);
size_t pit_write(size_t size,size_t *buf);


