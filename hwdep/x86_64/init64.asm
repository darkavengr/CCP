;  CCP Version 0.0.1
;    (C) Matthew Boote 2021

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
; Hardware dependent code for x86_64 processors
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
extern page_init_first_time					; initialize paging for first time
extern row							; current row
extern col							; current column
extern exit
extern kprintf
extern init_multitasking					; initalize multitasking
extern driver_init						; initialize drivers
extern dma_alloc_init						; intialize dma allocator
extern readconsole						
extern outputconsole
extern currentprocess
extern processes
extern characterdevs
extern blockdevices
extern switch_task
extern initrd_init

;
; globals
;
global tss_esp0
global getstackpointer					; get stack pointer
global switchtonextprocess				;switch to next process
global _asm_init
global _loadregisters					; load registers
global halt						; halt
global loadregisters					; load registers from buffer
global saveregisters					; save registers to buffer
global restart						; restart computer
global disable_interrupts				; disable interrupts
global enable_interrupts				; enable interrupts
global switchtousermode_and_call_process		; switch to usermode and call process
global MEM_SIZE						; size of memory
global initializestack					; intialize stack
global tickcount					; tick counter
global set_interrupt					; set interrupt
global get_interrupt					; get interrupt
global yield
global idttable
PAGE_SIZE equ 4096					; size of page  DON'T CHANGE	
SYSTEM_USE equ 0FFFFFFFFFFFFFFFFh				; page marked for system used
KERNEL_STACK_SIZE equ  65536				; size of initial kernel stack
BASE_ADDR_LOW		equ 0
BASE_ADDR_HIGH		equ 4
LENGTH_LOW		equ 8
LENGTH_HIGH		equ 12
BLOCK_TYPE		equ 16

INITIAL_KERNEL_STACK_ADDRESS equ 0x80000
BASE_OF_SECTION equ 0x8000000000100000
KERNEL_HIGH	equ 0x8000000000000000
DMA_BUFFER_SIZE equ 32768	

ROOT_PAGEDIR		equ	0x60000
ROOT_PAGETABLE		equ	0x70000
PAGE_PRESENT		equ	1
PAGE_RW			equ	2

KERNEL_CODE_SELECTOR	equ	8h			; kernel code selector
KERNEL_DATA_SELECTOR	equ	10h			; kernel data selector
USER_CODE_SELECTOR	equ	1Bh			; user code selector
USER_DATA_SELECTOR	equ	23h			; user data selector
TSS_SELECTOR 		equ	28h			; tss selector

BOOT_INFO_DRIVE		equ	0xAC
BOOT_INFO_CURSOR	equ	0xAD
BOOT_INFO_INITRD_START	equ	0xAE
BOOT_INFO_SYMBOL_START	equ	0xB2
BOOT_INFO_MEMBUF_START	equ	0xB6
BOOT_INFO_END		equ	0xBA

_asm_init:
;
; 16-bit initialisation code
;
; here the computer is still in real mode,
; the kernel will enable the a20 line, detect memory
; load gdt and idt and jump to protected mode
; then jump to long mode
			

[BITS 16]
use16
cli
mov	sp,0e000h				; temporary stack

xor	ax,ax
mov	ds,ax
mov	es,ax
mov	ss,ax

;
; enable a20 line
;
call    a20wait					; wait for a20 line
mov     al,0xAD					; disable keyboard
out     0x64,al
 
call    a20wait					; wait for a20 line

mov     al,0xD0
out     0x64,al
 
call    a20wait2
in      al,0x60
push    eax
 
call    a20wait
mov     al,0xD1
out     0x64,al
 
call    a20wait
pop     eax
or      al,2
out     0x60,al
 
call    a20wait
mov     al,0xAE
out     0x64,al
 
call    a20wait
sti
jmp 	short a20done
 
a20wait:
in      al,0x64
test    al,2
jnz     a20wait
ret
 
a20wait2:
in      al,0x64
 test    al,1
jz      a20wait2
ret					

;
; detect memory
; this must be done in real mode
;
; using these functions:
;

; INT 0x15, AX = 0xE820 *
; INT 0x15, AX = 0xE801 *
; INT 0x15, AX = 0xDA88 *
; INT 0x15, AH = 0x88   *
; INT 0x15, AH = 0x8A   *
;
a20done:
xor	ebx,ebx

xor	ax,ax				; segment:offset address for buffer
mov	es,ax
mov	di,8000h				; segment:offset address for buffer

xor	ecx,ecx					; clear memory count
xor	edx,edx					; clear memory count

push	ecx
push	edx

not_end_e820:
mov	eax,0e820h				; get memory block
mov	edx,534d4150h
mov	ecx,20h					; size of memory block
int	15h
jc	no_e820					; 0e820 not supported

pop	edx					; get count
pop	ecx					; get count


add	ecx,[es:di+LENGTH_LOW]			; add size
adc	edx,[es:di+LENGTH_HIGH]			; add size
push	ecx
push	edx

add	edi,20h

test	ebx,ebx					; if at end
jnz	not_end_e820				; loop if not
jmp	end_detect_memory

no_e820:
mov	ax,0e801h				; get memory info
int	15h
jc	no_e801					; e801 not supported

cmp	ah,86h					; not supported
je	no_e801

cmp	ah,80h					; not supported
je	no_e801

test	eax,eax					; use eax,ebx
jnz	use_ebx_eax

add	ecx,1024				; plus conventional memory
shl	ecx,16					; multiply by 65536 because edx is in 64k blocks
shl	edx,10					; multiply by 1024 because eax is in kilobytes
mov	ecx,edx
jmp	end_detect_memory

use_ebx_eax:
add	eax,1024				; plus conventional memory
shl	ebx,16					; multiply by 65536 because edx is in 64k blocks
shl	eax,10					; multiply by 1024 because eax is in kilobytes
add	eax,ebx					; add memory count below 16M
mov	ecx,eax
jmp	end_detect_memory

no_e801:
mov	ah,88h					; get extended memory count
int	15h
jc	no_88h					; not supported

test	ax,ax					; error
jz	no_88h					; not supported

cmp	ah,86h					; not supported
je	no_88h

cmp	ah,80h					; not supported
je	no_88h

add	eax,1024				; plus conventional memory
shl	eax,10					; multiply by 1024, size is in kilobytes
mov	ecx,eax
jmp	end_detect_memory

no_88h:
xor	ecx,ecx
xor	ebx,ebx

mov	ax,0da88h				; get extended memory size
int	15h
jc	no_da88h				; not supported

; returned in cl:bx
shr	ecx,16					; shift and add
add	ecx,ebx
pop	edx
add	ecx,1024					; add conventional memory size
jmp	end_detect_memory

no_da88h:
mov	ah,8ah					; get extended memory size
int	15h
jc	no_8ah
push	ax					; returned in dx:ax
push	dx
pop	ecx

add	ecx,1024*1024				; add conventional memory size
add	ecx,eax
jmp	end_detect_memory

no_8ah:
hlt						; halt

end_detect_memory:
;
; from here to pmode: don't change ecx or edx
;
xor	ax,ax
mov	ds,ax
mov	es,ax
mov	ss,ax

cli	
mov 	edi,offset 32bgdtinfo
db	66h
lgdt 	[ds:edi]					; load gdt

mov 	eax,cr0   				; switch to pmode
or	al,1
mov  	cr0,eax

jmp	 dword 0x8:pmode			; flush instruction cache

[BITS 32]
use32

pmode:
cli
;****************************************************
; 32 bit protected mode
;****************************************************

mov	ax,KERNEL_DATA_SELECTOR			; selector
mov	ds,ax					; put selectors
ov	es,ax					
mov	fs,ax					
mov	gs,ax					
mov	ss,ax					

mov	[MEM_SIZE_HIGH],edx				; save memory size
mov	[MEM_SIZE],ecx				; save memory size

mov	esp,INITIAL_KERNEL_STACK_ADDRESS+KERNEL_STACK_SIZE	; temporary stack
mov	ebp,INITIAL_KERNEL_STACK_ADDRESS

lidt	[idt]					; load interrupts
ck long mode support

mov	eax,80000000h
cpuid
cmp	eax,80000001h		; not extended support
jne	not_long_mode

mov	eax,80000001h		; check for long mode
cpuid

and	edx,10000000h
test	edx,edx
jnz	long_mode_supported

;
; long mode is not supported, display a message and halt
;
not_long_mode:
push	0 
push	offset no_long_mode
call 	kprintf

cli
hlt
jmp 	$
	
long_mode_supported:
; switch to long mode enable PAE paging, set long mode bit in efer

;
; create 32-bit PAE paging first; long mode requires this be
; enabled before switching to long mode
;

; create PML4
mov	rdi,ROOT_PML4
mov	rax,ROOT_PAGEDIR+PAGE_RW+PAGE_PRESENT
mov	[rdi],rax			; map lower half
mov	[rdi+(384*4)],rax			; map higher half

; create temporary page table and jump to higher half

mov	rdi,ROOT_PAGEDIR
mov	rax,ROOT_PAGETABLE+PAGE_RW+PAGE_PRESENT
mov	[rdi],rax			; map lower half
mov	[rdi+(384*4)],rax			; map higher half

mov	rdi,ROOT_PAGETABLE
mov	rax,0+PAGE_PRESENT+PAGE_RW	; page+flags
mov	rcx,1024			;map 4mb
	
map_next_page:
stosq
add	rax,1000h			; point to next page
loop	map_next_page


mov	eax,ROOT_PAGEDIR		; load cr3
mov	cr3,eax
mov	eax,cr0
or	eax,80000000h			; enable paging
mov	cr0,eax

mov	eax,cr4					; set pae paging bit
and	eax,10h
mov	cr4,eax

rmsr						; enable long mode
or	eax,512
wmsr

mov	eax,cr0					; set pmode and paging
and	eax,800000001h
mov	cr0,eax

; load gdt and idt and
; switch to long mode

[USE64]
bits 64

lgdt	[gdt]					; load 64 bit gdt

jmp	8:longmode

; **************************************************
; in long mode here
; **************************************************

longmode:
mov	eax,jump_high			; jump to higher half
jmp	eax

jump_high:
mov	rsp,INITIAL_KERNEL_STACK_ADDRESS+KERNEL_STACK_SIZE	; temporary stack
mov	rbp,INITIAL_KERNEL_STACK_ADDRESS

push	0 
push	offset startingccp				; display banner
call 	kprintf

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

mov	rdi,ROOT_PML4			; unmap lower  half
xor	rax,rax
mov	[rdi],rax

endunmap:
call	processmanager_init
call	devicemanager_init

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
; 100001h -(100000+(MEM_SIZE/4096)*4)				CCP alloc map
; ((MEM_SIZE/4096)*4)+1						FREE depending on memory size

; place memory map after the initial ram disk

; map memory

mov	rbx,8000000000080000h						; point to buffer

next_mark:
mov	al,[rbx+BLOCK_TYPE]					; get type

test	al,al
jz	end_mark

cmp	al,1							; mark as usable
je	mark_usable

cmp	al,2							; mark as not usable
jge	mark_notusable

mark_usable:
xor	rax,rax
jmp	short mark_mem

mark_notusable:
mov	rax,SYSTEM_USE

mark_mem:
mov	rdi,[rbx+MEM_SIZE]			; start addresss
shr	rdi,12					; get number of 4096-byte pages
add	rdi,[MEMBUF_START]

rep	stosq
	
add	rbx,20h					; point to next
jmp	next_mark

end_mark:
mov	rdi,[MEMBUF_START]
mov	rcx,0x7000/PAGE_SIZE			; 20kb
mov	rax,SYSTEM_USE
rep	stosq					; real mode idt and data area

mov	rdi,0xA0000				; video memory+ROMs
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,2
add	rdi,[MEMBUF_START]
mov	rcx,((0x100000-0xA0000)/PAGE_SIZE)		; 0x100000-0xA0000
mov	rax,SYSTEM_USE
rep	stosq					; page reserved

mov	rdi,kernel_begin
sub	rdi,KERNEL_HIGH
and	rdi,0fffff000h
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,2
add	rdi,[MEMBUF_START]

mov	rcx,end					; size
add	rcx,PAGE_SIZE
sub	rcx,kernel_begin
shr	rcx,12					; get number of 4096-byte pages
mov	rax,SYSTEM_USE
rep	stosq					; page reserved

mov	rdi,INITIAL_KERNEL_STACK_ADDRESS
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,2					; multiply by 4,each 4096 byte page is represented by 4 byte entries
add	rdi,[MEMBUF_START]

mov	rcx,KERNEL_STACK_SIZE
shr	rcx,12					; get number of 4096-byte pages
mov	rax,SYSTEM_USE
rep	stosq

push	qword DMA_BUFFER_SIZE
call	dma_alloc_init				; initalize dma allocation

push	qword offset outputconsole
push	qword offset readconsole
call	openfiles_init				; initialize device manager
add	esp,12

push	qword 0 
push	qword offset startingccp				; display banner
call 	kprintf

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

mov	rsp,(INITIAL_KERNEL_STACK_ADDRESS+KERNEL_STACK_SIZE)+KERNEL_HIGH ; temporary stack

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
push	0
push	8
jmp	int_common	

int9_handler:
push	0
push	9
jmp	int_common	

int10_handler:
push	0
push	10
jmp	int_common	

int11_handler:
push	0
push	11
jmp	int_common	

int12_handler:
push	0
push	12
jmp	int_common	

int13_handler:
push	0
push	13
jmp	int_common	

int14_handler:
push	0
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
push	rax
mov	rax,[rsp+24]			; get eip of interrupt handler from stack
mov	[regbuf],rax			; save it
pop	rax

push	rax
pushfq
pop	rax
mov	[regbuf+80],rax			; save flags
pop	rax

mov	[regbuf+8],rsp			; save other registers
mov	[regbuf+16],rax
mov	[regbuf+24],rbx
mov	[regbuf+32],rcx
mov	[regbuf+40],rdx
mov	[regbuf+48],rsi
mov	[regbuf+56],rdi
mov	[regbuf+64],rbp
mov	[regbuf+72],r8
mov	[regbuf+80],r9
mov	[regbuf+88],r10
mov	[regbuf+96],r11
mov	[regbuf+104],r12
mov	[regbuf+112],r13
mov	[regbuf+120],r14
mov	[regbuf+128],r15

push	offset regbuf			; call exception handler
call	exception
add	rsp,40
iretq

end_process:
call exit
ret

;
; low level dispatcher
;
d_lowlevel:
nop

sti
push	rax
push	rbx
push	rcx
push	rdx
push	rsi
push	rdi

call	dispatchhandler

mov	[tempone],rax
	
pop	rdi
pop	rsi
pop	rdx
pop	rcx
pop	rbx
pop	rax

cmp	rax,0ffffffffh					; if error ocurred
je	iretq_error

mov	rax,[tempone]					; then return old eax

iretq_error:
sti							; interrupts back on
iretq  

;
; initialize stack
;
;
initializestack:
cli
mov	rax,[rsp]					; get eip
mov	[tempone],rax

mov	rbx,[rsp+4]					; get esp
mov	rax,rbx						; get stack size
sub	rax,[rsp+8]

sub	rbx,4096
mov	rsp,rbx
mov	rbp,rax
pusha

jmp	[tempone]					; return without using stack

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
push	rax				; +8
push	rbx				;+16
push	rcx				;+24
push	rdx				;+32
push	rsi				;+40
push	rdi				;+48
nop
nop

mov	rdx,[rsp+80]			; getnumber
mov	rax,[rsp+72]			; get address of interrupt handler
mov	rcx,[rsp+64]			; get selector
mov	rbx,[rsp+56]			; get flags


mov	rdi,rbx
shr	rdi,4				; each interrupt is 16 bytes long
add	rdi,offset idttable

and	rax,0xffff
mov	[rdi],ax			; write word
shr	rax,16
mov	[rdi+6],ax			; write word
shr	rax,16
mov	[rdi+8],eax			; write dword

mov	[rdi+2],cx			; selector
mov	[rdi+5],bl			; flags

xor	rbx,rbx
mov	[rdi+4],bl
mov	[rdi+12],ebx

;lidt	[idttable]

pop	rdi
pop	rsi
pop	rdx
pop	rcx
pop	rbx
pop	rax
ret

get_interrupt:
push	rbx
push	rcx
push	rdx
push	rsi
push	rdi
nop
nop

mov	rcx,[rsp+56]			; get interrupt number

mov	rdi,rbx
shl	rdi,4				; each interrupt is 16 bytes long
add	rdi,offset idttable

mov	rax,dword [rdi+8]		; high 32 bits
shl	rax,32

movsx	rbx,word [rdi+6]		; middle 16bits
shr	rbx,16
add	rax,rbx

movsx	rbx,word [rdi]			; low 16bits
add	rax,rbx

pop	rdi
pop	rsi
pop	rdx
pop	rcx
pop	rbx
ret

;
; switch to user mode
;

switch_to_usermode_and_call_process:
mov	rbx,[rsp+4]				; get entry point

mov	ax,USER_DATA_SELECTOR
mov	ds,ax
mov	es,ax
mov	fs,ax
mov	gs,ax

mov	rax,rsp

push	USER_DATA_SELECTOR			; user ss
push	rax					; user esp

pushfq						; user eflags
pop	rax
or	rax,200h				; enable interrupts
push	rax

push	USER_CODE_SELECTOR			; user cs
push	rbx					; user rip
iretq


;
; 32-bit structures

32bit_gdt:
dw offset 32bgdt_end - offset 32bgdt-1
dd offset 32bgdt

; null entries, don't modify
32bgdt dw 0
dw 0
db 0
db 0,0
db 0

; intial 32 bit gdt

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

32bgdt_end:
db 0

32bidt:
dw 0x3FFF					; limit
dd offset 32bidttable				; base

32bidttable:
times 256 dw (BASE_OF_SECTION + null_32bhandler - $$) & 0xFFFF,8h,8eh,(BASE_OF_SECTION +  null_32b_handler - $$)>> 16

;
; 64-bit structures
;
gdtinfo:
dd offset gdt
dw offset gdt_end-offset gdt

gdt:
dw 0xFFFF             	      ; low word of limit
dw 0                         ; low word of base
db 0                         ; middle byte of base
db 0                  
db 1
db 0                         ; high byte of base

; ring 0 code segment

dw 0                         ; low word of limit
dw 0                         ; low word of base
db 0                         ; middle byte of base
db 9ah               
db 0afh
db 0        		; high byte of base

; ring 0 data segment
dw 0                        
dw 0                 
db 0
db 92h              
db 0afh
db 0

; ring 3 code segment

dw 0                         ; low word of limit
dw 0                         ; low word of base
db 0                         ; middle byte of base
db 9ah               
db 0afh
db 0        		; high byte of base

; ring 3 data segment
dw 0                        
dw 0                 
db 0
db 92h              
db 0afh
db 0


idt:
dw offset end_idt-offset idttable		; limit
dq offset idttable				; base

idttable:
times 256 db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

tempone dq 0
temptwo dq 0
MEMBUF_START dq offset end
MEM_SIZE dd 0
MEM_SIZE_HIGH dd 0

banner db "Starting CCP...",13,0
long_mode_not_supported db "CCP requires a x86_64 CPU to run, system halted.",13,0

