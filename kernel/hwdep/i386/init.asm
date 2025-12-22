;
;  CCP Version 0.0.1
;  (C) Matthew Boote 2020-2023
;
;  This file is part of CCP.
;
;  CCP is free software: you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation, either version 3 of the License, or
;  (at your option) any later version.
;
;  CCP is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with CCP.  If not, see <https://www.gnu.org/licenses/>.
;


;
; Initialization code for i386 processors
;
; In: Information passed via boot information
; 
; Returns: Doesn't return
;

%define TRUE 1
%define FALSE 0

extern kernel							; high-level kernel
extern kernel_begin						; start of kernel in memory
extern filemanager_init						; intialize file manager
extern memorymanager_init					; intialize memory manager
extern processmanager_init					; intialize process manager
extern devicemanager_init					; intialize device
extern init_multitasking					; initalize multitasking
extern driver_init						; initialize built-in drivers
extern initialize_interrupts					; initialize interrupts
extern initialize_memory_map					; initialize memory map
extern initializepaging						; initialize paging
extern initrd_init						; initialize initial RAM disk
extern tty_init							; initalize TTY
extern end							; end of kernel in memory
extern load_idt							; load IDT
extern unmap_lower_half						; unmap lower half
extern initialize_tss						; initialize tss
extern set_tss_esp0						; set kernel mode stack address in TSS
extern load_modules_from_initrd					; load modules from initial RAM disk
extern get_initial_kernel_stack_base				; get initial kernel stack base
extern get_initial_kernel_stack_top				; get initial kernel stack top
extern get_kernel_stack_size					; get kernel stack size
extern initialize_abstract_timer				; initialize abstract timer

;
; globals
;
global _asm_init
global MEMBUF_START
global gdt
global gdt_end

GDT_LIMIT equ 5

%include "init.inc"
%include "kernelselectors.inc"
%include "bootinfo.inc"
%include "gdtflags.inc"

_asm_init:
;
; 16-bit initialisation code
;
; Here the computer is still in real mode.
; Hhe kernel will enable the a20 line, detect memory
; load gdt and idt and jump to protected mode
;
; From here to jump_high, the code is located in the lower half of memory
; and addresses should have KERNEL_HIGH subtracted from them.
			
%macro a20wait 0
%%a20:
in    al,0x64
test  al,2
jnz   %%a20
%endmacro

%macro a20wait2 0
%%a202:
in    al,0x64
test  al,1
jz    %%a202
%endmacro

[BITS 16]
use16
cli
mov	sp,0xE000				; temporary stack

xor	ax,ax
mov	ds,ax
mov	es,ax
mov	ss,ax

mov	esi,starting_ccp
sub	esi,KERNEL_HIGH

next_banner_char:
mov	ah,0xE					; output character

db	0x67
mov	al,[esi]
int	0x10

inc	esi
test	al,al					; loop until end
jnz	next_banner_char

jmp	short	over_msg

starting_ccp db 10,13,10,13,"Starting CCP...",10,13,0
over_msg:

;
; enable a20 line
;
a20wait					; wait for a20 line
mov   al,0xAD					; disable keyboard
out   0x64,al
 
a20wait					; wait for a20 line

mov   al,0xD0
out   0x64,al
 
a20wait2
in    al,0x60
push  eax
 
a20wait
mov   al,0xD1
out   0x64,al
 
a20wait
pop   eax
or    al,2
out   0x60,al
 
a20wait
mov   al,0xAE
out   0x64,al
 
a20wait
		
a20done:
mov 	edi,gdtinfo
sub	edi,KERNEL_HIGH
db	0x66
lgdt 	[ds:edi]			; load gdt

mov 	eax,cr0   			; switch to protected mode
or	al,1
mov  	cr0,eax

db	0x66				; jmp dword 0x8:pmode
db	0xEA
dd	pmode-KERNEL_HIGH
dw	8

[BITS 32]
use32

pmode:
cli
;****************************************************
; 32 bit protected mode
;****************************************************

mov	ax,KERNEL_DATA_SELECTOR			; selector
mov	ds,ax					; put selectors
mov	es,ax					
mov	fs,ax					
mov	gs,ax					
mov	ss,ax					

call	get_initial_kernel_stack_top
mov	esp,eax					; temporary stack

call	get_initial_kernel_stack_base
mov	ebp,eax

;
; place memory map after initrd and symbol table

mov	eax,[BOOT_INFO_KERNEL_START]
add	eax,[BOOT_INFO_KERNEL_SIZE]		; Point to end of kernel
add	eax,[BOOT_INFO_SYMBOL_SIZE]
add	eax,[BOOT_INFO_INITRD_SIZE]
add	eax,KERNEL_HIGH

mov	[MEMBUF_START-KERNEL_HIGH],eax

mov	edx,end				; calculate kernel size
sub	edx,kernel_begin

push	dword [BOOT_INFO_MEMORY_SIZE]	; memory size
push	dword [BOOT_INFO_SYMBOL_SIZE]	; symbol table size
push	dword [BOOT_INFO_INITRD_SIZE]	; initial RAM disk size
push	dword [BOOT_INFO_KERNEL_SIZE]	; kernel size
call	initializepaging		; initialize paging to lower and higher halves of memory

mov	eax,higher_half
jmp	eax				; jump to higher half

higher_half:
call	get_initial_kernel_stack_top
add	eax,KERNEL_HIGH
mov	esp,eax					; temporary stack

call	get_initial_kernel_stack_base
add	eax,KERNEL_HIGH
mov	ebp,eax
;
; from here, don't need to subtract KERNEL_HIGH
;
call	unmap_lower_half		; unmap lower half of memory

lgdt	[gdt_high]			; load high gdt

call	initialize_interrupts		; initialize interrupts
call	load_idt			; set exception interrupts and syscall and load IDT

call	processmanager_init		; intialize process manager
call	devicemanager_init		; intialize device manager

mov	edi,end				; calculate kernel size
sub	edi,kernel_begin

mov	ebx,BOOT_INFO_MEMORY_SIZE	; get memory size
add	ebx,KERNEL_HIGH
mov	ebx,[ebx]

mov	ecx,kernel_begin		; get kernel start address
mov	edx,[MEMBUF_START]

call	get_kernel_stack_size
push	eax

call	get_initial_kernel_stack_base
push	eax				; stack address
push	edi				; kernel size
push	ecx				; kernel start address
push	ebx				; memory size
push	edx				; memory map start address
call	initialize_memory_map		; initialize memory map

mov	edx,[MEMBUF_START]

call	memorymanager_init		; initalize memory manager

; This block of code is a copy of the
; irq remapping code from drivers/pic/pic.c
; translated into assembler.
;
; It is  included here to make sure that
; the IRQs are remapped to interrupts
; 0xF0 to 0xFF even if the PIC driver
; is not used If it is, it will be used twice
; with no bad effects.
; Without this int 0x21 will be used by irq 1 and will conflict the system call interface

mov	al,0x11
mov	dx,0x20
out	dx,al				; remap irq

mov	al,0x11
mov	dx,0xA0
out	dx,al	

mov	al,0xF0
mov	dx,0x21
out	dx,al	

mov	al,0xF8
mov	dx,0xA1
out	dx,al	

mov	al,4
mov	dx,0x21
out	dx,al	

mov	al,0x2
mov	dx,0xA1
out	dx,al	

mov	al,1
mov	dx,0x21
out	dx,al	

mov	al,0x1
mov	dx,0xA1
out	dx,al	

xor	al,al
mov	dx,0x21
out	dx,al

xor	al,al
mov	dx,0xA1
out	dx,al

call	initialize_tss				; initialize TSS

push	esp
call	set_tss_esp0				; set TSS ESP0 to top of initial kernel stack

;call	init_multitasking			; initialize multitasking
call	filemanager_init			; initialize file manager
call	driver_init				; initialize built-in modules
call	initrd_init				; intialize modules in initial RAM disk
call	tty_init				; initalize TTY
call	initialize_abstract_timer		; initialize abstract timer

call	get_initial_kernel_stack_top

add	eax,KERNEL_HIGH
mov	esp,eax					; temporary stack

call	load_modules_from_initrd		; load modules from initial RAM disk

sti
jmp	kernel					; jump to highlevel code


; GDT

gdtinfo:
dw gdt_end - gdt-1
dd gdt-KERNEL_HIGH

gdt_high:
dw gdt_end - gdt-1
dd gdt

; null entries, don't modify
gdt dw 0
dw 0
db 0
db 0,0
db 0

; ring 0 code segment

dw 0xFFFF					; low word of limit
dw 0						; low word of base
db 0						; middle byte of base
						; access byte
db SEGMENT_PRESENT | RING0_SEGMENT | CODE_OR_DATA_SEGMENT | EXECUTABLE_SEGMENT | CODE_SEGMENT_NON_CONFORMING | CODE_SEGMENT_READABLE
db FLAG_GRANULARITY_PAGE | FLAG_32BIT_SEGMENT | 0xF	; Flags (High nibble) and low four bits of limit (low nibble)
db 0						; last byte of base

; ring 0 data segment

dw 0xFFFF					; low word of limit
dw 0		 				; low word of base
db 0	 					; middle byte of base
						; access byte
db SEGMENT_PRESENT | RING0_SEGMENT | CODE_OR_DATA_SEGMENT | NON_EXECUTABLE_SEGMENT | DATA_SEGMENT_GROWS_UP | DATA_SEGMENT_WRITEABLE
db FLAG_GRANULARITY_PAGE | FLAG_32BIT_SEGMENT | 0xF	; Flags (High nibble) and low four bits of limit (low nibble)
db 0						; last byte of base

; ring 3 code segment
dw 0xFFFF					; low word of limit
dw 0						; low word of base
db 0						; middle byte of base
						; access byte
db SEGMENT_PRESENT | RING3_SEGMENT | CODE_OR_DATA_SEGMENT | EXECUTABLE_SEGMENT | CODE_SEGMENT_NON_CONFORMING | CODE_SEGMENT_READABLE
db FLAG_GRANULARITY_PAGE | FLAG_32BIT_SEGMENT | 0xF	; Flags (High nibble) and low four bits of limit (low nibble)
db 0						; last byte of base

; ring 3 data segment
dw 0xFFFF					; low word of limit
dw 0		 				; low word of base
db 0	 					; middle byte of base
						; access byte
db SEGMENT_PRESENT | RING3_SEGMENT | CODE_OR_DATA_SEGMENT | NON_EXECUTABLE_SEGMENT | DATA_SEGMENT_GROWS_UP | DATA_SEGMENT_WRITEABLE
db FLAG_GRANULARITY_PAGE | FLAG_32BIT_SEGMENT | 0xF	; Flags (High nibble) and low four bits of limit (low nibble)
db 0						; last byte of base

times GDT_LIMIT-4 db 0,0,0,0,0,0,0,0		; extra entries for TSS and other things

gdt_end:

MEMBUF_START dd end

