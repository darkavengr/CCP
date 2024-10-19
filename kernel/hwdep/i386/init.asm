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
%define offset

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
extern end							; end of kernel in memory
extern load_idt							; load IDT
extern unmap_lower_half						; unmap lower half
extern initialize_tss						; initialize tss
extern set_tss_esp0						; set kernel mode stack address in TSS
extern load_modules_from_initrd					; load modules from initial RAM disk
extern get_initial_kernel_stack_base				; get initial kernel stack base
extern get_initial_kernel_stack_top				; get initial kernel stack top
extern get_kernel_stack_size					; get kernel stack size

; globals
;
global _asm_init
global MEMBUF_START
global gdt
global gdt_end

DMA_BUFFER_SIZE equ 32768	

GDT_LIMIT equ 5

%include "init.inc"
%include "kernelselectors.inc"
%include "bootinfo.inc"

_asm_init:
;
; 16-bit initialisation code
;
; here the computer is still in real mode,
; the kernel will enable the a20 line, detect memory
; load gdt and idt and jump to protected mode
;
;
; from here to jump_high, the code is located in the lower
; half of memory.
; addresses should have KERNEL_HIGH subtracted from them
			
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

section.text

[BITS 16]
use16

cli
mov	sp,0e000h				; temporary stack

xor	ax,ax
mov	ds,ax
mov	es,ax
mov	ss,ax

mov	esi,starting_ccp
sub	esi,KERNEL_HIGH

next_banner_char:
mov	ah,0eh					; output character
mov	al,[esi]
int	10h

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
xor	ax,ax
mov	ds,ax
mov	es,ax
mov	ss,ax

mov 	edi,offset gdtinfo
add	edi,KERNEL_HIGH
db	66h
lgdt 	[ds:edi]			; load gdt

mov 	eax,cr0   			; switch to protected mode
or	al,1
mov  	cr0,eax

db	66h				; jmp dword 0x8:pmode
db	0eah
dd	offset pmode-KERNEL_HIGH
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
push	dword [BOOT_INFO_INITRD_SIZE]	; initrd size
push	dword [BOOT_INFO_KERNEL_SIZE]				; kernel size
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
call	load_idt			; load interrupts

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

push	dword DMA_BUFFER_SIZE
call	memorymanager_init		; initalize memory manager

call	filemanager_init		; initialize file manager


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

mov	al,11h
mov	dx,20h
out	dx,al				; remap irq

mov	al,11h
mov	dx,0A0h
out	dx,al	

mov	al,0f0h
mov	dx,21h
out	dx,al	

mov	al,0f8h
mov	dx,0A1h
out	dx,al	

mov	al,4
mov	dx,21h
out	dx,al	

mov	al,2h
mov	dx,0A1h
out	dx,al	

mov	al,1
mov	dx,21h
out	dx,al	

mov	al,1h
mov	dx,0A1h
out	dx,al	

xor	al,al
mov	dx,21h
out	dx,al

xor	al,al
mov	dx,0A1h
out	dx,al

;call	init_multitasking
call	driver_init				; initialize built-in drivers and filesystems
call	initrd_init

call	get_initial_kernel_stack_top
add	eax,KERNEL_HIGH
mov	esp,eax					; temporary stack

push	esp
call	set_tss_esp0

call	initialize_tss					; initialize tss

call	load_modules_from_initrd

sti

; jump to highlevel code
jmp	kernel

section .data
gdtinfo:
dw offset gdt_end - offset gdt-1
dd offset gdt-KERNEL_HIGH

gdt_high:
dw offset gdt_end - offset gdt-1
dd offset gdt

; null entries, don't modify
gdt dw 0
dw 0
db 0
db 0,0
db 0

; intial gdt
; ring 0 segments

dw 0ffffh					; low word of limit
dw 0						; low word of base
db 0						; middle byte of base
db 09ah,0cfh					; Code segment
db 0						; last byte of base

dw 0ffffh					; low word of limit
dw 0		 				; low word of base
db 0	 					; middle byte of base
db 92h,0cfh					; Data segment
db 0						; last byte of base

; ring 3 segments
dw 0ffffh					; low word of limit
dw 0						; low word of base
db 0						; middle byte of base
db 0FAh,0cfh					; Code segment
db 0						; last byte of base

dw 0ffffh					; low word of limit
dw 0		 				; low word of base
db 0	 					; middle byte of base
db 0f2h,0cfh					; Data segment
db 0						; last byte of base

times GDT_LIMIT-4 db 0,0,0,0,0,0,0,0		; extra entries for TSS and other things

gdt_end:

MEMBUF_START dd offset end

