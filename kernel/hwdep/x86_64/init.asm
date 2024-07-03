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
; Initialization code for x86_64 processors
;
; In: Information passed via boot information
; 
; Returns: Doesn't return
;

ROOT_PML4		equ	0x1000
ROOT_PDPT		equ	0x2000
ROOT_PAGEDIR		equ	0x3000
ROOT_PAGETABLE		equ	0x4000
ROOT_EMPTY		equ	0x5000

TEMP_GDT		equ	0x6000

PAGE_PRESENT equ 1
PAGE_RW equ 2
	
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
extern initialize_paging_long_mode				; initialize paging and switch to long mode
extern initrd_init						; initialize initial RAM disk
extern end							; end of kernel in memory
extern load_idt							; load IDT
extern unmap_lower_half						; unmap lower half
extern initialize_tss						; initialize tss
extern set_tss_rsp0						; set kernel mode stack address in TSS
extern load_modules_from_initrd					; load modules from initial RAM disk
extern kprintf_direct

; globals
;
global _asm_init
global MEMBUF_START
global gdt
global gdt_end

KERNEL_STACK_SIZE equ  65536*2				; size of initial kernel stack
INITIAL_KERNEL_STACK_ADDRESS equ 0x60000		; intial kernel stack address

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

section .text

[BITS 16]
use16

xchg	bx,bx

cli
mov	sp,0e000h				; temporary stack

xor	ax,ax
mov	ds,ax
mov	es,ax
mov	ss,ax

mov	esi,$-starting_ccp
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
; check for long mode support

xchg	bx,bx

mov	eax,0x80000000
cpuid
cmp	eax,0x80000001		; not extended support
jne	not_long_mode

mov	eax,0x80000001		; check for long mode
cpuid

and	edx,0x10000000
test	edx,edx
jnz	long_mode_supported

;
; long mode is not supported, display a message and halt
;
not_long_mode:
cli
hlt
jmp 	$
	
long_mode_supported:
;
; create tables
;
mov	edi,ROOT_PML4
xor	eax,eax
mov	ecx,1024
rep	stosd

mov	edi,ROOT_PDPT
xor	eax,eax
mov	ecx,1024
rep	stosd

mov	edi,ROOT_PAGEDIR
xor	eax,eax
mov	ecx,1024
rep	stosd

mov	edi,ROOT_PML4
mov	eax,ROOT_PDPT | PAGE_PRESENT
xor	edx,edx
mov	[edi],eax
mov	[edi+16],edx

mov	eax,ROOT_PML4		; set pdptphys and processid
mov	[edi+36],eax

mov	edi,ROOT_PDPT			; create pdpt
mov	eax,ROOT_PAGEDIR | PAGE_PRESENT
xor	edx,edx
mov	[edi],eax
mov	[edi+4],edx

; create page table

; find number of page tables to create

mov	ecx,(1024*1024)				; map first 1mb

;xchg	bx,bx

mov	esi,BOOT_INFO_KERNEL_SIZE
add	ecx,[esi]

mov	esi,BOOT_INFO_INITRD_START
add	ecx,[esi]

mov	esi,BOOT_INFO_SYMBOL_SIZE
add	ecx,[esi]

mov	esi,BOOT_INFO_MEMORY_SIZE			; memory map size
mov	eax,[esi]
shr	eax,12					; number of pages
shl	eax,3					; number of 8-byte entries
add	ecx,eax

and	ecx,0xfffffffffffff000

add	ecx,1000h
shr	ecx,12					; number of page table entries
mov	edi,ROOT_PAGETABLE
mov	eax,0+PAGE_PRESENT+PAGE_RW	; page+flags
xor	edx,edx

map_next_page:
mov	[edi],edx			; high dword
mov	[edi+4],eax			; low dword

add	edi,8
add	eax,1000h			; point to next page
loop	map_next_page

mov	edi,ROOT_PML4
mov	eax,ROOT_PML4+PAGE_RW+PAGE_PRESENT
xor	edx,edx

mov	[edi],edx
mov	[edi+4],eax			; map lower half

mov	[edi+(512*4)+4],edx
mov	[edi+(512*4)],eax		; map higher half

mov	[edi+PAGE_SIZE],edi		; PAGEDIRPHYS

mov	edi,ROOT_PML4+(255*8)	; map last entry to pml4
mov	eax,ROOT_PML4+PAGE_RW+PAGE_PRESENT
mov	[edi],eax

mov	eax,cr4				; enable pae paging
bt	eax,5
mov	cr4,eax

mov	ecx, 0xC0000080			; enable long mode

rdmsr
or	eax, (1 << 8)
wrmsr

mov	eax,ROOT_PML4
mov	cr3,eax				; load cr3

mov	edx,(1 << 31) | (1 << 0)
or	ebx,edx				; enable protected mode and paging
mov	cr0,ebx

mov	ax,KERNEL_DATA_SELECTOR	
mov	ds,ax
mov	es,ax
mov	fs,ax
mov	gs,ax

db	66h				; jmp dword 0x8:longmode
db	0eah
dd	$-offset longmode
dw	8

longmode:


;****************************************************
; 64 bit long mode
;****************************************************

[BITS 64]
use64

mov	rax,qword higher_half
jmp	rax				; jump to higher half

higher_half:
;
; from here, don't need to subtract KERNEL_HIGH
;

;
; place memory map after initrd and symbol table

xor	rdx,rdx

mov	rax,[rel BOOT_INFO_KERNEL_START]
add	rdx,rax

mov	rax,[rel BOOT_INFO_KERNEL_SIZE]		; Point to end of kernel
add	rdx,rax

mov	rax,[rel BOOT_INFO_SYMBOL_SIZE]
add	rdx,rax
mov	rax,[rel BOOT_INFO_INITRD_SIZE]
add	rdx,rax

mov	rax,qword KERNEL_HIGH	
add	rdx,rax

mov	rdi,qword MEMBUF_START-KERNEL_HIGH
mov	[rdi],rdx

mov	rsp,qword INITIAL_KERNEL_STACK_ADDRESS+KERNEL_HIGH+KERNEL_STACK_SIZE	; temporary 64-bit stack
mov	rbp,qword INITIAL_KERNEL_STACK_ADDRESS+KERNEL_HIGH

; Unmap lower half
mov	rdi,qword ROOT_PML4
xor	rax,rax
mov	[rdi],rax

mov	r11,gdtinfo_64
lgdt	[r11]				; load 64-bit gdt

call	initialize_interrupts		; initialize interrupts
call	load_idt			; load interrupts

call	processmanager_init		; intialize process manager
call	devicemanager_init		; intialize device manager

mov	rax,qword end			; calculate kernel size
mov	r11,rax
mov	rax,qword kernel_begin
sub	r11,rdx

mov	rbx,qword BOOT_INFO_MEMORY_SIZE	; get memory size
add	rbx,qword KERNEL_HIGH
mov	rbx,[rbx]

mov	rcx,qword kernel_begin		; get kernel start address

blah11:
mov	r11,MEMBUF_START
mov	rax,[r11]

blah12:
mov	r11,qword KERNEL_STACK_SIZE			; stack size
push	r11
mov	r11,qword INITIAL_KERNEL_STACK_ADDRESS	; stack address
push	r11
push	rax				; kernel size
push	rcx				; kernel start address
push	rbx				; memory size
push	rdx				; memory map start address
call	initialize_memory_map		; initialize memory map

mov	rax,qword DMA_BUFFER_SIZE

push	rax
call	memorymanager_init		; initalize memory manager

call	filemanager_init		; initialize file manager


; This block of code is a copy of the
; irq remapping code from drivers/pic/pic.c
i; translated into assembler.
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

call	init_multitasking
call	driver_init				; initialize built-in drivers and filesystems

call	initrd_init

mov	rsp,(INITIAL_KERNEL_STACK_ADDRESS+KERNEL_HIGH)+KERNEL_STACK_SIZE	; temporary stack
push	rsp
call	set_tss_rsp0

call	initialize_tss					; initialize tss

call	load_modules_from_initrd

sti

; jump to highlevel code
jmp	kernel

section .data
; 64-bit GDT

gdtinfo_64:
dw offset gdt_end - offset gdt-1
dq offset gdt-KERNEL_HIGH

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
MEMBUF_START dq 0
no_long_mode db 10,13,"This version of CCP requires a x86-64 CPU",10,13,0

