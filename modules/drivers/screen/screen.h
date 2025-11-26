#define SCREEN_MAX_X	80
#define SCREEN_MAX_Y	25

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

#define COLOR_BLACK	0
#define COLOR_BLUE	1
#define COLOR_GREEN	2
#define COLOR_CYAN 	3
#define COLOR_RED	4
#define COLOR_PURPLE	5
#define COLOR_ORANGE	6
#define COLOR_WHITE	7
#define COLOR_DARK_GREY 8
#define COLOR_LIGHT_BLUE	9
#define COLOR_LIGHT_GREEN	10
#define COLOR_LIGHT_CYAN	11
#define COLOR_LIGHT_RED	12
#define COLOR_LIGHT_PURPLE	13
#define COLOR_YELLOW		14
#define COLOR_LIGHT_WHITE	15

#define DEFAULT_TEXT_COLOR (COLOR_BLACK << 4) | COLOR_WHITE

#define IOCTL_SCREEN_SET_CURSOR_ROW 1
#define IOCTL_SCREEN_SET_CURSOR_COL 2
#define IOCTL_SCREEN_GET_CURSOR_ROW 3
#define IOCTL_SCREEN_GET_CURSOR_COL 4
#define IOCTL_SCREEN_SET_COLOR	 5
#define IOCTL_SCREEN_GET_COLOR  6

size_t screen_write(char *s,size_t size);
void movecursor(uint16_t row,uint16_t col);
size_t getcursorpos(void);
size_t screen_init(char *init);
void set_screen_char_colour(uint8_t c);
size_t screen_ioctl(size_t handle,unsigned long request,void *buffer);

