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

DMA_BUFFER_SIZE equ 32768	

GDT_LIMIT equ 10

PAGE_PRESENT equ 1
PAGE_RW equ 2
PAGE_SIZE equ 4096

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
extern set_tss_rsp0						; set kernel mode stack address in TSS
extern load_modules_from_initrd					; load modules from initial RAM disk
extern get_initial_kernel_stack_base				; get initial kernel stack base
extern get_initial_kernel_stack_top				; get initial kernel stack top
extern get_kernel_stack_size					; get kernel stack size
extern set_initial_process_paging_end				; set initial process paging end

;
; globals
;
global _asm_init
global MEMBUF_START
global gdt
global gdt_end
global gdtinfo_high_64
global get_initial_kernel_stack_base
global get_initial_kernel_stack_top
global get_kernel_stack_size

%include "init.inc"
%include "kernelselectors.inc"
%include "bootinfo.inc"
%include "gdtflags.inc"

_asm_init:
;
; 16-bit initialisation code
;
; Here the computer is still in real mode,
; the kernel will enable the A20 line, detect memory
; load gdt and idt and jump to long mode.
;
; From here to jump_high, the code is located in the lower
; half of physical and virtual memory.
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
cli
jmp	over

output16:
next_char:
mov	ah,0xE					; output character
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
call	output16				; display using BIOS

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
call	output16				; display using BIOS

cli
hlt
jmp 	$
	
long_mode_supported:
;
; create paging tables
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

shr	eax,12
shl	eax,8

add	ecx,eax

shr	ecx,12					; number of page table entries

mov	edi,ROOT_PAGETABLE
mov	eax,0 | PAGE_PRESENT | PAGE_RW	; page+flags

cld

mov	edx,PAGE_SIZE
xor	ebx,ebx

map_next_page:
mov	[edi],eax			; low dword
mov	[edi+4],ebx			; high dword

add	edi,8
add	eax,edx				; point to next page
loop	map_next_page

mov	eax,cr4				; enable PAE paging
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
db	0x66
lgdt 	[ds:edi]			; load GDT

mov	ax,KERNEL_DATA_SELECTOR	
mov	ds,ax
mov	es,ax
mov	fs,ax
mov	gs,ax
mov	ss,ax

db	0x66				; jmp dword KERNEL_CODE_SELECTOR:longmode
db	0xEA
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

call	get_initial_kernel_stack_top
add	rax,KERNEL_HIGH
mov	rsp,rax					; temporary stack

call	get_initial_kernel_stack_base
add	rax,KERNEL_HIGH
mov	rbp,rax

;
; place memory map after kernel,initrd and symbol table
;
xor	rdx,rdx

mov	r11,BOOT_INFO_KERNEL_START+KERNEL_HIGH
mov	edx,[r11]

mov	r11,BOOT_INFO_KERNEL_SIZE+KERNEL_HIGH
add	edx,[r11]

mov	r11,BOOT_INFO_SYMBOL_SIZE+KERNEL_HIGH
add	edx,[r11]

mov	r11,BOOT_INFO_INITRD_SIZE+KERNEL_HIGH
add	edx,[r11]

mov	r11,MEMBUF_START-KERNEL_HIGH
mov	[r11],rdx

mov	rax,KERNEL_HIGH
add	rdx,rax

mov	rdi,MEMBUF_START
mov	[rdi],rdx

call	get_initial_kernel_stack_top
mov	rcx,qword KERNEL_HIGH
add	rax,rcx
mov	rsp,rax					; temporary stack

call	get_initial_kernel_stack_base
mov	rcx,qword KERNEL_HIGH
add	rax,rcx
mov	rbp,rax

; Unmap lower half

mov	rsi,qword ROOT_PML4
xor	rax,rax
mov	[rsi],rax

call	initialize_interrupts		; initialize interrupts
call	load_idt			; load interrupts

call	set_initial_process_paging_end ; set initial processpaging end to

call	processmanager_init		; intialize process manager
call	devicemanager_init		; intialize device manager

; rdi, rsi, rdx, rcx, r8, r9

;void initialize_memory_map(size_t memory_map_address,size_t memory_size,size_t kernel_begin,size_t kernel_size,size_t stack_address,size_t stack_size) {

mov	r11,qword MEMBUF_START
mov	rdi,[r11]

mov	r11,qword BOOT_INFO_MEMORY_SIZE+KERNEL_HIGH
mov	rsi,[r11]			; memory_size

mov	rdx,qword kernel_begin		; kernel_begin

mov	rcx,qword end
mov	rax,qword kernel_begin
sub	rcx,rax				; kernel_size

call	get_initial_kernel_stack_top
mov	r8,rax				; stack_address

call	get_kernel_stack_size
mov	r9,rax				; stack_size

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

call	init_multitasking

call	driver_init				; initialize built-in modules
call	initrd_init

call	initialize_tss					; initialize TSS

call	get_initial_kernel_stack_top

mov	rcx,qword KERNEL_HIGH
add	rax,rcx
mov	rdi,rax
call	set_tss_rsp0

call	load_modules_from_initrd

sti

jmp	kernel		; jump to high g7level code


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
db 0
db 0
db 0

; ring 0 code segment

dw 0xFFFF					; low word of limit
dw 0						; low word of base
db 0						; middle byte of base
						; access byte
db SEGMENT_PRESENT | RING0_SEGMENT | CODE_OR_DATA_SEGMENT | EXECUTABLE_SEGMENT | CODE_SEGMENT_NON_CONFORMING | CODE_SEGMENT_READABLE
db FLAG_GRANULARITY_PAGE | FLAG_LONG_MODE | 0xF ; Flags (High nibble) and low four bits of limit (low nibble)
db 0						; last byte of base

; ring 0 data segment

dw 0xFFFF					; low word of limit
dw 0		 				; low word of base
db 0	 					; middle byte of base
						; access byte
db SEGMENT_PRESENT | RING0_SEGMENT | CODE_OR_DATA_SEGMENT | NON_EXECUTABLE_SEGMENT | DATA_SEGMENT_GROWS_UP | DATA_SEGMENT_WRITEABLE
db FLAG_GRANULARITY_PAGE | FLAG_32BIT_SEGMENT | 0xF ; Flags (High nibble) and low four bits of limit (low nibble)
db 0						; last byte of base

; ring 3 code segment

dw 0xFFFF					; low word of limit
dw 0						; low word of base
db 0						; middle byte of base
						; access byte
db SEGMENT_PRESENT | RING3_SEGMENT | CODE_OR_DATA_SEGMENT | EXECUTABLE_SEGMENT | CODE_SEGMENT_NON_CONFORMING | CODE_SEGMENT_READABLE
db FLAG_GRANULARITY_PAGE | FLAG_LONG_MODE | 0xF ; Flags (High nibble) and low four bits of limit (low nibble)
db 0						; last byte of base

; ring 3 data segment

dw 0xFFFF					; low word of limit
dw 0		 				; low word of base
db 0	 					; middle byte of base
						; access byte
db SEGMENT_PRESENT | RING3_SEGMENT | CODE_OR_DATA_SEGMENT | NON_EXECUTABLE_SEGMENT | DATA_SEGMENT_GROWS_UP | DATA_SEGMENT_WRITEABLE
db FLAG_GRANULARITY_PAGE | FLAG_32BIT_SEGMENT | 0xF ; Flags (High nibble) and low four bits of limit (low nibble)
db 0						; last byte of base

times GDT_LIMIT-4 db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0	; extra entries for TSS and other things
gdt_end:

MEMBUF_START dq 0
starting_ccp db 10,13,"Starting CCP...",10,13,0
no_long_mode db 10,13,"This version of CCP requires a x86-64 CPU",10,13,0

