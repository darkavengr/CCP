;
;  CCP Version 0.0.1
;    (C) Matthew Boote 2020-2022

;    This file is part of CCP.

;    CCP is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation, either version 3 of the License, or
;    (at your option) any later version.

;    CCP is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.

;    You should have received a copy of the GNU General Public License
;    along with CCP.  If not, see <https://www.gnu.org/licenses/>.

;
; Hardware dependent code for i386 processors
;
; In: Information passed via boot information
; 
; Returns: Doesn't return
;

%define TRUE 1
%define FALSE 0
%define offset

extern kernel							; high-level kernel
extern dispatchhandler						; high-level dispatcher
extern exception						; exception handler
extern kernel_begin						; start of kernel in memory
extern openfiles_init						; intialize structs
extern end							; end of kernel in memory
extern row							; current row
extern col							; current column
extern exit
extern kprintf
extern init_multitasking					; initalize multitasking
extern driver_init						; initialize drivers
extern memorymanager_init					; intialize memory manager
extern processmanager_init					; intialize processes
extern devicemanager_init					; intialize devices
extern switch_task
extern initrd_init
extern paging_type
extern kernelalloc
extern disablemultitasking
extern enablemultitasking
extern initrd_init

;
; globals
;
global tss_esp0
global getstackpointer					; get stack pointer
global _asm_init
global _loadregisters					; load registers
global halt						; halt
global saveregisters					; save registers to buffer
global restart						; restart computer
global disable_interrupts				; disable interrupts
global enable_interrupts				; enable interrupts
global switch_to_usermode_and_call_process		; switch to usermode and call process
global initializestack					; intialize stack
global set_interrupt					; set interrupt
global get_interrupt					; get interrupt
global idttable
global MEMBUF_START
global switch_kernel_stack				; switch kernel stack
global get_stack_base_pointer					; get base pointer

global enablemultitasking
global disablemultitasking
global init_multitasking

PAGE_SIZE equ 4096					; size of page  DON'T CHANGE	
SYSTEM_USE equ 0FFFFFFFFh				; page marked for system used
KERNEL_STACK_SIZE equ  65536*2				; size of initial kernel stack

INITIAL_KERNEL_STACK_ADDRESS equ 0x60000
BASE_OF_SECTION equ 0x80100000
KERNEL_HIGH	equ 0x80000000
DMA_BUFFER_SIZE equ 32768	

ROOT_PDPT		equ	0x2000
ROOT_PAGEDIR		equ	0x1000
ROOT_PAGETABLE		equ	0x3000
ROOT_EMPTY		equ	0x4000

PAGE_PRESENT		equ	1
PAGE_RW			equ	2

KERNEL_CODE_SELECTOR	equ	8h			; kernel code selector
KERNEL_DATA_SELECTOR	equ	10h			; kernel data selector
USER_CODE_SELECTOR	equ	1Bh			; user code selector
USER_DATA_SELECTOR	equ	23h			; user data selector
TSS_SELECTOR 		equ	28h			; tss selector

BOOT_INFO_DRIVE		equ	0xAC
BOOT_INFO_CURSOR_ROW	equ	0xAD
BOOT_INFO_CURSOR_COL	equ	0xAE
BOOT_INFO_KERNEL_START	equ	0xAF
BOOT_INFO_KERNEL_SIZE	equ	0xB3
BOOT_INFO_INITRD_START	equ	0xB7
BOOT_INFO_INITRD_SIZE	equ	0xBB
BOOT_INFO_SYMBOL_START	equ	0xBF
BOOT_INFO_SYMBOL_SIZE	equ	0xC3
BOOT_INFO_NUM_SYMBOLS	equ	0xC7
BOOT_INFO_MEMORY_SIZE	equ	0xCB

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
in      al,0x64
test    al,2
jnz     %%a20
%endmacro

%macro a20wait2 0
%%a202:
in      al,0x64
test    al,1
jz      %%a202
%endmacro

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
lodsb
int	10h

test	al,al					; loop until end
jnz	next_banner_char

jmp	short	over_msg

starting_ccp db 10,13,"Starting CCP...",10,13,0
over_msg:

;
; enable a20 line
;
a20wait					; wait for a20 line
mov     al,0xAD					; disable keyboard
out     0x64,al
 
a20wait					; wait for a20 line

mov     al,0xD0
out     0x64,al
 
a20wait2
in      al,0x60
push    eax
 
a20wait
mov     al,0xD1
out     0x64,al
 
a20wait
pop     eax
or      al,2
out     0x60,al
 
a20wait
mov     al,0xAE
out     0x64,al
 
a20wait
		
a20done:
xor	ax,ax
mov	ds,ax
mov	es,ax
mov	ss,ax

mov 	edi,offset gdtinfo
add	edi,KERNEL_HIGH
db	66h
lgdt 	[ds:edi]					; load gdt

mov 	eax,cr0   				; switch to pmode
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
	
mov	esp,(INITIAL_KERNEL_STACK_ADDRESS+KERNEL_HIGH)+KERNEL_STACK_SIZE	; temporary stack
mov	ebp,INITIAL_KERNEL_STACK_ADDRESS+KERNEL_HIGH

;
; place memory map after initrd and symbol table

mov	eax,[BOOT_INFO_KERNEL_START]
add	eax,[BOOT_INFO_KERNEL_SIZE]		; Point to end of kernel
add	eax,[BOOT_INFO_SYMBOL_SIZE]
add	eax,[BOOT_INFO_INITRD_SIZE]
add	eax,KERNEL_HIGH

mov	[MEMBUF_START-KERNEL_HIGH],eax

lea	esi,[paging_type-KERNEL_HIGH]
mov	eax,[esi]

cmp	eax,1
je	legacy_paging

cmp	eax,2
je	pae_paging

jmp	legacy_paging

;
; enable pae paging
;
pae_paging:
mov	edi,ROOT_PDPT
xor	eax,eax
mov	ecx,4096
rep	stosb

mov	edi,ROOT_PDPT			; create pdpt
mov	eax,ROOT_PAGEDIR | PAGE_PRESENT
xor	edx,edx
mov	[edi],eax
mov	[edi+4],edx
mov	[edi+16],eax			; map higher half
mov	[edi+16+4],edx

mov	eax,ROOT_PDPT		; set pdptphys and processid
mov	[edi+36],eax

xor	eax,eax
mov	[edi+44],eax

mov	edi,ROOT_PAGEDIR
xor	eax,eax
mov	ecx,1024
rep	stosd

mov	edi,ROOT_PAGEDIR
mov	eax,ROOT_PAGETABLE | PAGE_RW | PAGE_PRESENT
xor	edx,edx
mov	[edi],eax
mov	[edi+4],edx
mov	cr3,edi

; create page table

mov	ecx,(1024*1024)*4			; map first 4mb
shr	ecx,12					; number of page table entries
shl	ecx,4

mov	edi,ROOT_PAGETABLE
xor	eax,eax
rep	stosd

mov	ecx,(1024*1024)*4			; map first 4mb
shr	ecx,12					; number of page table entries
shl	ecx,4

mov	edi,ROOT_PAGETABLE
mov	eax,0+PAGE_PRESENT+PAGE_RW	; page+flags

map_next_page:
or	eax,PAGE_PRESENT
mov	[edi],eax
mov	dword [edi+4],0

add	eax,1000h
add	edi,8

sub	ecx,1
sbb	edx,0

mov	ebx,ecx
add	ebx,edx
test	ebx,ebx
jnz	map_next_page

mov	eax,cr4				; enable pae paging
bts	eax,5
mov	cr4,eax

mov	eax,ROOT_PDPT			; create pdpt
mov	cr3,eax

mov	eax,cr0
or	eax,80000000h			; enable paging
mov	cr0,eax

mov	eax,jump_high			; jump to higher half
jmp	eax

;
; legacy paging
;
legacy_paging:
mov	edi,ROOT_PAGEDIR
xor	eax,eax
mov	ecx,1024
rep	stosd

; create page table

; find number of page tables to create

mov	ecx,(1024*1024)				; map first 1mb

mov	ebx,end					; map CCP kernel
sub	ebx,KERNEL_HIGH
sub	ebx,100000h
and	ebx,0fffff000h
add	ebx,1000h
add	ecx,ebx

add	ecx,[BOOT_INFO_INITRD_SIZE]
add	ecx,[BOOT_INFO_SYMBOL_SIZE]

; map memory map

add	ecx,[BOOT_INFO_MEMORY_SIZE]				; get memory size
and	ecx,0fffff000h
add	ecx,1000h

shr	ecx,12					; number of page table entries
shl	ecx,2

mov	edi,ROOT_PAGETABLE
mov	eax,0+PAGE_PRESENT+PAGE_RW	; page+flags

map_next_page_legacy:
stosd
add	eax,1000h			; point to next page
loop	map_next_page_legacy

mov	edi,ROOT_PAGEDIR
mov	eax,ROOT_PAGETABLE+PAGE_RW+PAGE_PRESENT
mov	[edi],eax			; map lower half
mov	[edi+(512*4)],eax			; map higher half
mov	[edi+PAGE_SIZE],edi		; pagedirphys

mov	edi,ROOT_PAGEDIR+(1023*4)	; map last entry to page directory - page tables will be present at 0xfc000000 - 0xfffff000
mov	eax,ROOT_PAGEDIR+PAGE_RW+PAGE_PRESENT
mov	[edi],eax

mov	eax,ROOT_PAGEDIR		; load cr3
mov	cr3,eax

mov	eax,cr0
or	eax,80000000h			; enable paging
mov	cr0,eax

over:
mov	eax,jump_high			; jump to higher half
jmp	eax

jump_high:
mov	eax,[paging_type]

cmp	eax,1
je	unmap_pd

mov	edi,ROOT_EMPTY
xor	eax,eax
mov	ecx,1024
rep	stosd

mov	edi,ROOT_PDPT
xor	eax,eax
mov	dword [edi],ROOT_EMPTY
mov	[edi+4],eax

mov	edi,ROOT_PDPT+(3*8)	; map last entry to page directory - page tables will be present at 0xc0000000 - 0xfffff000
mov	edx,ROOT_PDPT | PAGE_PRESENT
mov	[edi],edx	
mov	[edi+4],eax	

mov	edx,ROOT_PDPT
mov	cr3,edx

jmp	short endunmap	

unmap_pd:
mov	edi,ROOT_PAGEDIR
xor	eax,eax
mov	[edi],eax
endunmap:

;
; from here, don't need to subtract KERNEL_HIGH
;
lgdt	[gdt_high]			; load high gdt

push	0
push	offset int0_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	1
push	offset int1_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	2
push	offset int2_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	3
push	offset int3_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	4
push	offset int4_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	5
push	offset int5_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	6
push	offset int6_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	7
push	offset int7_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	8
push	offset int8_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	9
push	offset int9_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	10
push	offset int10_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	11
push	offset int11_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	12
push	offset int12_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	13
push	offset int13_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	14
push	offset int14_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	15
push	offset int15_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	16
push	offset int16_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	17
push	offset int17_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	18
push	offset int18_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt

push	0x21
push	offset d_lowlevel
push	8				; selector
push	0eeh				; flags
call	set_interrupt

lidt	[idt]					; load interrupts

call	processmanager_init		; intialize process manager
call	devicemanager_init		; intialize device manager

; create page frames

; mark reserved memory

; common code

; 00000000h				 			real mode idt and bios data area
; 00001000h							page directory for system
; 00003000h-9F9FFF						FREE
; 0009FA00h-FFFFF						ROMs, Video memory 
; 100000h							CCP 
; 100000h + CCP.SYS size
; 100000h + CCP.SYS size + INITRD.SYS size
; 100001h -(100000+(BOOT_INFO_MEMORY_SIZE/4096)*4)				CCP alloc map
; ((BOOT_INFO_MEMORY_SIZE/4096)*4)+1						FREE depending on memory size

; place memory map after the initial ram disk

; map memory

; clear memory map area before filling it

mov	edi,[MEMBUF_START]

mov	ecx,[BOOT_INFO_MEMORY_SIZE+KERNEL_HIGH]
and	ecx,0fffff000h
shr	ecx,12					; get number of 4096-byte pages
shl	ecx,2

xor	eax,eax
rep	stosd

mov	edi,[MEMBUF_START]
mov	ecx,(1024*1024)/PAGE_SIZE		; map first 1mb
mov	eax,SYSTEM_USE
rep	stosd					; real mode idt and data area

mov	edi,kernel_begin
sub	edi,KERNEL_HIGH
and	edi,0fffff000h
shr	edi,12					; get number of 4096-byte pages
shl	edi,2
add	edi,[MEMBUF_START]

mov	ecx,end					; size
sub	ecx,kernel_begin
add	ecx,PAGE_SIZE

mov	eax,[BOOT_INFO_MEMORY_SIZE+KERNEL_HIGH]
and	eax,0fffff000h
shr	eax,12					; get number of 4096-byte pages
shr	eax,2
add	ecx,eax

shr	ecx,12					; get number of 4096-byte pages
mov	eax,SYSTEM_USE
rep	stosd					; page reserved

mov	edi,INITIAL_KERNEL_STACK_ADDRESS
shr	edi,12					; get number of 4096-byte pages
shl	edi,2					; multiply by 4,each 4096 byte page is represented by 4 byte entries
add	edi,[MEMBUF_START]

mov	ecx,KERNEL_STACK_SIZE
shr	ecx,12					; get number of 4096-byte pages
mov	eax,SYSTEM_USE
rep	stosd

mov	edi,[MEMBUF_START]
sub	edi,KERNEL_HIGH
shr	edi,12					; get number of 4096-byte pages
shl	edi,2					; multiply by 4,each 4096 byte page is represented by 4 byte entries
add	edi,[MEMBUF_START]

mov	ecx,KERNEL_STACK_SIZE
shr	ecx,12					; get number of 4096-byte pages
mov	eax,SYSTEM_USE
rep	stosd

push	dword DMA_BUFFER_SIZE
call	memorymanager_init			; initalize memory manager

push	1
push	2
call	openfiles_init				; initialize device manager
add	esp,12


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

call	init_multitasking

call	driver_init				; initialize drivers and filesystems
;call	initrd_init

; intialize tss

mov	eax,KERNEL_DATA_SELECTOR			; kernel selector
mov	[reg_es],eax
mov	[reg_ss],eax
mov	[reg_ds],eax
mov	[reg_fs],eax
mov	[reg_gs],eax
mov	[reg_cs],eax
mov	[tss_ss0],eax

mov	ax,TSS_SELECTOR + 3				; load tss for interrupt calls from ring 3
ltr	ax

mov	esp,(INITIAL_KERNEL_STACK_ADDRESS+KERNEL_HIGH)+KERNEL_STACK_SIZE	; temporary stack
mov	[tss_esp0],esp
sti

; jump to highlevel code
jmp	kernel

;
; Interrupt handlers
;

int0_handler:
push	0
push	0
jmp	int_common

int1_handler:
push	0
push	1
jmp	int_common	

int2_handler:
push	0
push	2
jmp	int_common	

int3_handler:
push	0
push	3
jmp	int_common	

int4_handler:
push	0
push	4
jmp	int_common	

int5_handler:
push	0
push	5
jmp	int_common	

int6_handler:
push	0
push	6
jmp	int_common	

int7_handler:
push	0
push	7
jmp	int_common	

int8_handler:
push	8
jmp	int_common	

int9_handler:
push	0
push	9
jmp	int_common	

int10_handler:
push	10
jmp	int_common	

int11_handler:
push	11
jmp	int_common	

int12_handler:
push	12
jmp	int_common	

int13_handler:
push	13
mov	dword [intnumber],13
jmp	int_common	

int14_handler:
push	14
jmp	int_common	

int15_handler:
push	0
push	15
jmp	int_common	

int16_handler:
push	0
push	16
jmp	int_common	

int17_handler:
push	0
push	17
jmp	int_common	

int18_handler:
push	0
push	18
jmp	int_common	

int_common:
mov	[regbuf+4],esp			; save other registers
mov	[regbuf+8],eax
mov	[regbuf+12],ebx
mov	[regbuf+16],ecx
mov	[regbuf+20],edx
mov	[regbuf+24],esi
mov	[regbuf+28],edi
mov	[regbuf+32],ebp

mov	eax,[esp+8]			; get eip of interrupt handler from stack
mov	[regbuf],eax			; save it

mov	eax,[esp+12]			; get flags
mov	[regbuf+40],eax			; save flags

push	offset regbuf			; call exception handler
call	exception
add	esp,12

exit_exception:
xchg	bx,bx
iret

end_process:
call exit
iret

get_stack_base_pointer:
mov	eax,ebp
ret

;
; low level dispatcher
;
d_lowlevel:
cli
nop

push	eax
push	ebx
push	ecx
push	edx
push	esi
push	edi

sti

mov 	ax,KERNEL_DATA_SELECTOR				; load the kernel data segment descriptor
mov 	ds,ax
mov 	es,ax
mov 	fs,ax
mov 	gs,ax

;call	disablemultitasking

call	dispatchhandler
;call	enablemultitasking

cli	
mov	[tempone],eax

mov 	ax,USER_DATA_SELECTOR				; load the kernel data segment descriptor
mov 	ds,ax
mov 	es,ax
mov 	fs,ax
mov 	gs,ax

pop	edi
pop	esi
pop	edx
pop	ecx
pop	ebx
pop	eax
sti
cmp	eax,0ffffffffh					; if error ocurred
je	iret_error

mov	eax,[tempone]					; then return old eax

iret_error:
sti
iret  

;
; initialize stack
;
;
initializestack:
cli
mov	eax,[esp]					; get eip
mov	[tempone],eax

mov	ebx,[esp+4]					; get esp
mov	eax,ebx
;sub	eax,[esp+8]					; minus stack size

mov	esp,ebx
mov	ebp,eax

jmp	[tempone]					; return without using stack

switch_kernel_stack:
mov	ebx,[esp+4]					; get address
mov	[tss_esp0],ebx					; patch tss esp0
ret

restart:
in	al,64h					; get status

and	al,2					; get keyboard buffer status
jnz	restart

mov	al,0feh					; reset
out 	64h,al

halt:
hlt
ret

enable_interrupts:
sti
ret

disable_interrupts:
cli
ret

set_interrupt:
push	eax		;+4
push	ebx		;+8
push	ecx		;+12
push	edx		;+16
push	esi		;+20
push	edi		;+24
nop
nop

mov	edx,[esp+40]			; getnumber
mov	eax,[esp+36]			; get address of interrupt handler
mov	ecx,[esp+32]			; get selector
mov	ebx,[esp+28]			; get flags

shl	edx,3				; each interrupt is eight bytes long
mov	edi,edx
add	edi,offset idttable

mov	[edi],ax			; write low word
mov	[edi+2],cx			; put selector

xor	cl,cl
mov	[edi+4],cl			; put 0
mov	[edi+5],bl			; put flags

shr	eax,16
mov	[edi+6],ax			; put high word

;lidt	[idttable]

pop	edi
pop	esi
pop	edx
pop	ecx
pop	ebx
pop	eax
ret

get_interrupt:
push	ebx
push	ecx
push	edx
push	esi
push	edi
nop
nop

mov	ecx,[esp+32]			; get address of interrupt handler
mov	ebx,[esp+28]			; get irq number

mov	eax,8				; each interrupt is eight bytes long
mul	ebx

add	ebx,offset idttable

xor	eax,eax
mov	ax,[ebx+6]
shl	eax,16
mov	ax,[ebx]

pop	edi
pop	esi
pop	edx
pop	ecx
pop	ebx
ret

;
; switch to user mode
;

switch_to_usermode_and_call_process:
mov	ebx,[esp+4]				; get entry point

mov	ax,USER_DATA_SELECTOR
mov	ds,ax
mov	es,ax
mov	fs,ax
mov	gs,ax

mov	eax,esp

push	USER_DATA_SELECTOR			; user ss
push	eax					; user esp

pushf						; user eflags
pop	eax
or	eax,200h				; enable interrupts
push	eax

push	USER_CODE_SELECTOR			; user cs
push	ebx				; user eip
iret

null_interrupt:
iret

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
db 0					; last byte of base

dw 0ffffh					; low word of limit
dw 0		 				; low word of base
db 0	 					; middle byte of base
db 0f2h,0cfh					; Data segment
db 0						; last byte of base

dw 0x68						; tss segment
dw (BASE_OF_SECTION + prev_tss - $$) & 0xffff		; low word of base
db ((BASE_OF_SECTION + prev_tss - $$) >> 16) & 0xff ; middle of base
db 0xE9,0
db ((BASE_OF_SECTION + prev_tss - $$) >> 24) & 0xff			 ; middle of base

gdt_end:
db 0

idt:
dw 0x3FFF					; limit
dd offset idttable				; base

idttable:
times 256 db 0,0,0,0,0,0,0,0

prev_tss dd 0
tss_esp0 dd 0
tss_ss0 dd 0
tss_esp1 dd 0
tss_ss1 dd 0 
tss_esp2 dd 0
tss_ss2 dd 0 
tss_cr3 dd 0
tss_eip dd 0
tss_eflags dd 0
reg_eax dd 0
reg_ebx dd 0
reg_ecx dd 0
reg_edx dd 0
reg_esp dd 0
reg_ebp dd 0
reg_esi dd 0
reg_edi dd 0
reg_es dd 0 
reg_cs dd 0 
reg_ss dd 0 
reg_ds dd 0 
reg_fs dd 0 
reg_gs dd 0 
reg_ldt dd 0
tss_trap dw 0					
tss_iomap_base dw 0
end_tss:

tempone dd 0
temptwo dd 0
MEMBUF_START dd offset end
regbuf times 20 dd 0
intnumber dd 0
