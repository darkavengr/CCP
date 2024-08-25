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
; Initialization for x86_64 processors
;
; In: Information passed via boot information
; 
; Returns: Doesn't return
;

ROOT_PML4		equ	0x1000
ROOT_PDPT		equ	0x3000
ROOT_PAGEDIR		equ	0x4000
ROOT_PAGETABLE		equ	0x5000

KERNEL_STACK_SIZE equ  65536*3				; size of initial kernel stack
INITIAL_KERNEL_STACK_ADDRESS equ 0x70000		; intial kernel stack address
DMA_BUFFER_SIZE equ 32768	
GDT_LIMIT equ 10


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
extern devicemanager_init					; intialize device manager
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
global gdtinfo_high_64

%include "init.inc"
%include "kernelselectors.inc"
%include "bootinfo.inc"


_asm_init:
;
; 16-bit initialisation code
;
; here the computer is still in real mode,
; the kernel will enable the a20 line, detect memory
; load gdt and idt and jump to long mode
;
;
; from here to jump_high, the code is located in the lower
; half of memory.
;
; Addresses should be position-independent until label longmode
			
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
jmp	over

output16:
next_char:
mov	ah,0xe					; output character
a32	lodsb
int	10h

test	al,al					; loop until end
jnz	next_char
ret

over:
cli
mov	sp,0e000h				; temporary stack

xor	ax,ax
mov	ds,ax
mov	es,ax
mov	ss,ax

mov	esi,(starting_ccp-_asm_init)+phys_start_address
call	output16				; display using bios

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

mov	eax,0x80000000
cpuid

cmp	eax,0x80000001		; no extended support
jbe	not_long_mode

mov	eax,0x80000001		; check for long mode
cpuid

and	edx,1 << 29
test	edx,edx
jnz	long_mode_supported

;
; if long mode is not supported, display a message and halt
;
not_long_mode:
mov	esi,(no_long_mode-_asm_init)+phys_start_address
call	output16				; display using bios

cli
hlt
jmp 	$
	
long_mode_supported:
;
; create tables
;
mov	edi,ROOT_PML4
xor	eax,eax
mov	ecx,8196/4
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
mov	eax,ROOT_PDPT+PAGE_RW+PAGE_PRESENT

mov	[edi],eax			; map lower half
mov	[edi+(510*8)],eax		; map higher half

mov	eax,ROOT_PML4+PAGE_RW+PAGE_PRESENT	; map pml4 to itself
mov	[edi+(511*8)],eax

mov	edi,ROOT_PDPT			; create pdpt
mov	eax,ROOT_PAGEDIR+PAGE_RW+PAGE_PRESENT
mov	[edi],eax

mov	edi,ROOT_PAGEDIR			; create page directory
mov	eax,ROOT_PAGETABLE+PAGE_RW+PAGE_PRESENT
mov	[edi],eax

mov	eax,ROOT_PML4			; set pml4phys
mov	[eax+4096],eax

; create page table

; find number of page tables to create

mov	ecx,(1024*1024)				; map first 1MB

mov	esi,BOOT_INFO_KERNEL_SIZE
add	ecx,[esi]

mov	esi,BOOT_INFO_INITRD_START
add	ecx,[esi]

mov	esi,BOOT_INFO_SYMBOL_SIZE
add	ecx,[esi]

mov	esi,BOOT_INFO_MEMORY_SIZE			; memory map size
mov	eax,[esi]
and	eax,0xfffff000

shr	eax,12					; number of pages
shl	eax,3					; number of 8-byte entries

add	ecx,eax
shr	eax,12					; number of page table entries

mov	edi,ROOT_PAGETABLE
mov	eax,0+PAGE_PRESENT+PAGE_RW	; page+flags

cld

map_next_page:
mov	[edi],eax			; low dword

add	edi,8
add	eax,PAGE_SIZE			; point to next page
loop	map_next_page

mov	eax,cr4				; enable pae paging
or	eax,(1 << 5)
mov	cr4,eax

mov	ecx, 0xC0000080			; enable long mode

rdmsr
or	eax, (1 << 8)
wrmsr

mov	eax,ROOT_PML4
mov	cr3,eax				; load cr3

mov	eax,cr0
or	eax,(1 << 31) | (1 << 0)
mov	cr0,eax

mov 	edi,(gdtinfo_64-_asm_init)+phys_start_address
db	66h
lgdt 	[ds:edi]			; load GDT

mov	ax,KERNEL_DATA_SELECTOR	
mov	ds,ax
mov	es,ax
mov	fs,ax
mov	gs,ax
mov	ss,ax

db	66h				; jmp dword KERNEL_CODE_SELECTOR:longmode
db	0eah
dd	(longmode-_asm_init)+phys_start_address
dw	KERNEL_CODE_SELECTOR

;****************************************************
; 64 bit long mode
;****************************************************

[BITS 64]
use64
longmode:
mov	rax,qword higher_half
jmp	rax				; jump to higher half

higher_half:
mov	rax,qword gdtinfo_high_64
lgdt	[rax]				; load higher-half GDT

;
; place memory map after initrd and symbol table
;
xor	rdx,rdx
xor	rax,rax

mov	rsi,BOOT_INFO_KERNEL_START
mov	eax,[rsi]
add	rdx,rax

mov	rsi,qword BOOT_INFO_KERNEL_SIZE		; Point to end of kernel
mov	eax,[rsi]
add	rdx,rax

mov	rsi,qword BOOT_INFO_SYMBOL_SIZE
mov	eax,[rsi]
add	rdx,rax

mov	rsi,qword BOOT_INFO_INITRD_SIZE
mov	eax,[rsi]
add	rdx,rax

mov	rax,qword KERNEL_HIGH	
add	rdx,rax

mov	rdi,MEMBUF_START
add	[rdi],rdx

mov	rsp,qword INITIAL_KERNEL_STACK_ADDRESS+KERNEL_HIGH+KERNEL_STACK_SIZE	; temporary 64-bit stack
mov	rbp,qword INITIAL_KERNEL_STACK_ADDRESS+KERNEL_HIGH

; Unmap lower half

mov	rsi,qword ROOT_PML4
xor	rax,rax
mov	[rsi],rax

call	initialize_interrupts		; initialize interrupts
call	load_idt			; load interrupts

call	processmanager_init		; intialize process manager
call	devicemanager_init		; intialize device manager

;	rdi=Memory map address
;	rsi=Initial kernel stack address
;	rdx=Initial kernel stack size
;	rcx=Memory size
;
mov	r11,MEMBUF_START		; memory map start address
mov	rdi,[r11]
mov	rsi,INITIAL_KERNEL_STACK_ADDRESS
mov	rdx,KERNEL_STACK_SIZE		; kernel

mov	r11,BOOT_INFO_MEMORY_SIZE+KERNEL_HIGH
mov	rcx,[r11]

call	initialize_memory_map		; initialize memory map

mov	rdi,qword DMA_BUFFER_SIZE
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

mov	rsp,(INITIAL_KERNEL_STACK_ADDRESS+KERNEL_HIGH)+KERNEL_STACK_SIZE	; temporary stack
push	rsp
call	set_tss_rsp0

call	initialize_tss					; initialize tss

call	load_modules_from_initrd

sti

; jump to highlevel code
jmp	kernel


; 64-bit GDT

gdtinfo_64:
dw (gdt_end-gdt)-1
dq (gdt-_asm_init)+phys_start_address

gdtinfo_high_64:
dw (gdt_end-gdt)-1
dq gdt

; null entry, don't modify
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
db 09ah,0afh					; Code segment
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
db 0FAh,0afh					; Code segment
db 0						; last byte of base

dw 0ffffh					; low word of limit
dw 0		 				; low word of base
db 0	 					; middle byte of base
db 0f2h,0cfh					; Data segment
db 0						; last byte of base

times GDT_LIMIT-4 db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0	; extra entries for TSS and other things
gdt_end:

MEMBUF_START dq 0
starting_ccp db "Starting CCP...",10,13,0
no_long_mode db "This version of CCP requires a x86-64 CPU",10,13,0

