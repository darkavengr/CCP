PIC_MASTER_COMMAND	equ 0x20
PIC_MASTER_DATA		equ 0x21
PIC_SLAVE_COMMAND	equ 0xA0
PIC_SLAVE_DATA		equ 0xA1

PIC_EOI		equ  0x20

ICW1_ICW4	equ 0x01		; ICW4 (not) needed 
ICW1_SINGLE	equ 0x02		; Single (cascade) mode 
ICW1_INTERVAL4	equ 0x04		; Call address interval 4 (8) 
ICW1_LEVEL	equ 0x08		; Level triggered (edge) mode 
ICW1_INIT	equ 0x10		; Initialization - required! 
 
ICW4_8086	equ 0x01		; 8086/88 (MCS-80/85) mode 
ICW4_AUTO	equ 0x02		; Auto (normal) EOI 
ICW4_BUF_SLAVE	equ 0x08		; Buffered mode/slave 
ICW4_BUF_MASTER	equ 0x0C		; Buffered mode/master 
ICW4_SFNM	equ 0x10		; Special fully nested (not) 

