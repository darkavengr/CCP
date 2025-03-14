#define TRUE		1
#define	FALSE		0

#define CTRL_AT		0
#define CTRL_A 		1
#define CTRL_B 		2
#define CTRL_C 		3
#define CTRL_D 		4
#define CTRL_E 		5
#define CTRL_F 		6
#define CTRL_G 		7
#define CTRL_H 		8
#define CTRL_I 		9
#define CTRL_J 		10
#define CTRL_K 		105
#define CTRL_L 		103
#define CTRL_M 		108
#define CTRL_N 		14
#define CTRL_O 		15
#define CTRL_P 		16
#define CTRL_Q 		17
#define CTRL_R		18
#define CTRL_S		21
#define CTRL_T		22
#define CTRL_U		23
#define CTRL_V		24
#define CTRL_W 		25
#define CTRL_X 		26
#define CTRL_Y 		27
#define CTRL_Z 		28

#define CTRL_BACKSLASH	30
#define CTRL_LEFTSB	29
#define CTRL_CARET	30
#define CTRL_UNDERSCORE	31

/* define scan codes */

#define KEY_ESC 		1
#define KEY_1_EXCL 		2
#define KEY_2_QUOTE 		3
#define KEY_3_POUND		4
#define KEY_4_DOLLAR 		5
#define KEY_5_PERCENT		6
#define KEY_6_CARET 		7
#define KEY_7_AMP 		8
#define KEY_8_STAR 		9
#define KEY_9_RIGHTBRACK 	10
#define KEY_0_LEFTBRACK 	11
#define KEYDASH_UNDERSCORE	12
#define KEYEQUALS_PLUS		13
#define KEY_BACK		14
#define KEY_TAB			15
#define KEY_Q			16
#define KEY_W			17
#define KEY_E			18
#define KEY_R 			19
#define KEY_T			20
#define KEY_Y			21
#define KEY_U			22
#define KEY_I			23
#define KEY_O			24
#define KEY_P			25
#define KEY_RIGHT_SQR_BRACKET_RIGHT_CURL_BRACKET 26
#define KEY_LEFT_SQR_BRACKET_LEFT_CURL_BRACKET 	 27
#define KEY_ENTER		28
#define KEY_CTRL		29
#define KEY_A			30
#define KEY_S			31
#define KEY_D			32
#define KEY_F 			33
#define KEY_G			34
#define KEY_H			35
#define KEY_J			36
#define KEY_K			37
#define KEY_L			38
#define KEY_SEMICOLON_COLON	29
#define KEY_SINGQUOTE_AT	40
#define KEY_HASH_TILDE		41
#define KEY_LEFTSHIFT		42
#define KEY_BACKSLASH_PIPE	43
#define KEY_Z			44
#define KEY_X			45
#define KEY_C			46
#define KEY_V			47
#define KEY_B			48
#define KEY_N			49
#define KEY_M 			50
#define KEY_COMMA_LESSTHAN	51
#define KEY_PERIOD_GREATERTHAN	52
#define KEY_SLASH_QUESTIONMARK	53
#define KEY_RIGHTSHIFT		54
#define KEY_PRINTSCR		55
#define KEY_ALT			56
#define KEY_SPACE		57
#define KEY_CAPSLOCK		58
#define KEY_F1			59
#define KEY_F2			60
#define KEY_F3			61
#define KEY_F4			62
#define KEY_F5			63
#define KEY_F6			64
#define KEY_F7			65
#define KEY_F8			66
#define KEY_F9			67
#define KEY_F10			68
#define KEY_F11			133
#define KEY_F12			134
#define KEY_NUMLOCK		69
#define KEY_SCROLL_LOCK		70
#define KEY_HOME_7		71
#define KEY_UP_8		72
#define KEY_PAGEUP9		73
#define KEY_GRAYDASH		74
#define KEY_LEFT4		75
#define KEY_CENTRE5		76
#define KEY_RIGHT6		77
#define KEY_GREYPLUS		78
#define KEY_END1		79
#define KEY_DOWN2		80
#define KEY_PAGEDOWN2		81
#define KEY_PAGEDOWN3		82
#define KEY_DEL			83
#define KEY_DELPOINT		84
#define RIGHT_SHIFT		0x36
#define LEFT_SHIFT		0x2a

#define LEFT_SHIFT_UP		0xAA
#define RIGHT_SHIFT_UP		0xB7
#define LEFT_ALT_UP		0xB8
#define RIGHT_ALT_UP		0x38
#define LEFT_CTRL_UP		0x9D
#define RIGHT_CTRL_UP		0x9D

#define KEYB_BUFFERSIZE 256

#define SHIFT_PRESSED 		1
#define ALT_PRESSED		2
#define CTRL_PRESSED		4
#define CAPS_LOCK_PRESSED	8
#define NUM_LOCK_PRESSED	16
#define SCROLL_LOCK_PRESSED	32

#define SCROLL_LOCK_LED		1		/* LED status bits */
#define NUM_LOCK_LED		2
#define CAPS_LOCK_LED		4

#define KEYBOARD_DATA_PORT	0x60
#define KEYBOARD_STATUS_PORT	0x64
#define KEYBOARD_COMMAND_PORT	0x64		/* same as status port */

#define KEYB_STATUS_OUTPUT_BUFFER_FULL	1	/* PS/2 status byte */
#define KEYB_STATUS_INPUT_BUFFER_FULL	2
#define KEYB_STATUS_SYSTEM_FLAG		4
#define KEYB_STATUS_COMMAND_OR_DATA	8
#define KEYB_STATUS_LOCKED		16
#define KEYB_STATUS_AUX_BUFFER_FULL	32
#define KEYB_STATUS_TIMED_OUT		64
#define KEYB_STATUS_PARITY_ERROR	128

#define BIOS_DATA_AREA_ADDRESS	0x417

#define KEYB_BDA_SCROLL_LOCK	16
#define KEYB_BDA_NUM_LOCK	32
#define KEYB_BDA_CAPS_LOCK	64

#define SET_KEYBOARD_LEDS	0xED

void readconsole(char *buf,size_t size);
size_t keyb_init(void);
void readkey(void);


