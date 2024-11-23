#define CONSOLE_MAX_X	80
#define CONSOLE_MAX_Y	25

#define NULL 0
/* colours are stored as a byte XY where X is four bits for the background colour and Y is four bits for the foreground colour

0 	Black 			8 	Dark Grey (Light Black)
1 	Blue 			9 	Light Blue
2 	Green 			10 	Light Green
3 	Cyan 			11 	Light Cyan
4 	Red 			12 	Light Red
5 	Purple 			13 	Light Purple
6 	Brown/Orange 		14 	Yellow (Light Orange)
7 	Light Grey (White) 	15 	White (Light White) */

#define CONSOLE_BLACK	0
#define CONSOLE_BLUE	1
#define CONSOLE_GREEN	2
#define CONSOLE_CYAN 	3
#define CONSOLE_RED	4
#define CONSOLE_PURPLE	5
#define CONSOLE_ORANGE	6
#define CONSOLE_WHITE	7
#define CONSOLE_DARK_GREY 8
#define CONSOLE_LIGHT_BLUE	9
#define CONSOLE_LIGHT_GREEN	10
#define CONSOLE_LIGHT_CYAN	11
#define CONSOLE_LIGHT_RED	12
#define CONSOLE_LIGHT_PURPLE	13
#define CONSOLE_YELLOW		14
#define CONSOLE_LIGHT_WHITE	15

#define IOCTL_CONSOLE_SET_CURSOR_ROW 1
#define IOCTL_CONSOLE_SET_CURSOR_COL 2
#define IOCTL_CONSOLE_GET_CURSOR_ROW 3
#define IOCTL_CONSOLE_GET_CURSOR_COL 4
#define IOCTL_CONSOLE_SET_COLOR	 5
#define IOCTL_CONSOLE_GET_COLOR  6

size_t outputconsole(char *s,size_t size);
void movecursor(uint16_t row,uint16_t col);
size_t getcursorpos(void);
size_t console_init(char *init);
void setconsolecolour(uint8_t c);
size_t console_ioctl(size_t handle,unsigned long request,void *buffer);

