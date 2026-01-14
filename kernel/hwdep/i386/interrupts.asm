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
; Interrupt functions
;

%include "kernelselectors.inc"

global disable_interrupts				; disable interrupts
global enable_interrupts				; enable interrupts
global set_interrupt					; set interrupt
global get_interrupt					; get interrupt
global load_idt
global initialize_interrupts
global int_common
extern exception					; exception handler
extern exit
extern dispatchhandler					; high-level dispatcher
extern disablemultitasking
extern enablemultitasking
extern enableirq
extern disableirq
extern getpid
extern is_multitasking_enabled
extern getlasterror

%include "idtflags.inc"

[BITS 32]
use32

;
; Initialize interrupts
;
; In: Nothing
;
; Returns: Nothing

initialize_interrupts:

; Initialize exception interrupts

; Divide by zero

push	0				; interrupt number
push	int0_handler			; handler
push	KERNEL_CODE_SELECTOR		; selector
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Debug exception

push	1
push	int1_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Non-maskable interrupt

push	2
push	int2_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Breakpoint exception

push	3
push	int3_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

;'Into detected overflow'

push	4
push	int4_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Out of bounds exception

push	5
push	int5_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

;  Invalid opcode exception

push	6
push	int6_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; No coprocessor exception

push	7
push	int7_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Double fault

push	8
push	int8_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Coprocessor segment overrun

push	9
push	int9_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Invalid TSS

push	10
push	int10_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Segment not present

push	11
push	int11_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Stack fault

push	12
push	int12_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; General protection fault

push	13
push	int13_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Page fault

push	14
push	int14_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Unknown interrupt exception

push	15
push	int15_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Coprocessor fault

push	16
push	int16_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Alignment check exception

push	17
push	int17_handler
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16

; Machine check exception

push	18
push	int18_handler
push	KERNEL_CODE_SELECTOR
push	1
call	set_interrupt
add	esp,16

;
; Set syscall interrupt
;
push	0x21
push	d_lowlevel
push	KERNEL_CODE_SELECTOR
push	IDT_ENTRY_PRESENT | IDT_RING3 | IDT_32BIT_64BIT_INTERRUPT_GATE
call	set_interrupt
add	esp,16
ret

;
; Enable interrupts
;
; In: Nothing
;
; Returns: Nothing
;
enable_interrupts:
sti
ret

;
; Disable interrupts
;
; In: Nothing
;
; Returns: Nothing
;
disable_interrupts:
cli
ret

;
; Load IDT
;
; In: Nothing
;
; Returns: Nothing
;
load_idt:
nop
lidt	[idt]					; load interrupts
ret

;
; Set interrupt
;
; [ESP+36]			; Interrupt number
; [ESP+32]			; Address of interrupt handler
; [ESP+28]			; Selector
; [ESP+24]			; Flags
;
; Returns -1 on error or 0 on success
;

set_interrupt:
push	ebx		;+8
push	ecx		;+12
push	edx		;+16
push	esi		;+20
push	edi		;+24
nop
nop

mov	edx,[esp+36]			; get interrupt number
mov	eax,[esp+32]			; get address of interrupt handler
mov	ecx,[esp+28]			; get selector
mov	ebx,[esp+24]			; get flags

cmp	edx,256				; check if valid
jl	set_interrupt_number_is_ok

mov	eax,0xffffffff
jmp	end_set_interrupt

set_interrupt_number_is_ok:
shl	edx,3				; each interrupt is eight bytes long
mov	edi,edx
add	edi,IDT_table

mov	[edi],ax			; write low word
mov	[edi+2],cx			; put selector

xor	cl,cl
mov	[edi+4],cl			; put 0
mov	[edi+5],bl			; put flags

shr	eax,16
mov	[edi+6],ax			; put high word

;lidt	[IDT_table]

xor	eax,eax				; return success

end_set_interrupt:
pop	edi
pop	esi
pop	edx
pop	ecx
pop	ebx
ret

;
; Get address of interrupt handler
;
; In: Interrupt number
;
; Returns: -1 on error or address of interrupt handler
;

get_interrupt:
push	ebx
push	ecx
push	edx
push	esi
push	edi
nop
nop

mov	ecx,[esp+32]			; get address of interrupt handler
mov	ebx,[esp+28]			; get interrupt number

cmp	ebx,256				; check if valid
jl	get_interrupt_number_is_ok

mov	eax,0xffffffff			; return error
ret

get_interrupt_number_is_ok:
mov	eax,8				; each interrupt is eight bytes long
mul	ebx

add	ebx,IDT_table

xor	eax,eax
mov	ax,[ebx+6]
shl	eax,16
mov	ax,[ebx]

xor	eax,eax				; return success

pop	edi
pop	esi
pop	edx
pop	ecx
pop	ebx
ret

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
xchg	bx,bx
push	13
jmp	int_common	

int14_handler:
;xchg	bx,bx
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
mov	[regbuf+4],esp			; save other registers into buffer to pass to exception handler
mov	[regbuf+8],eax
mov	[regbuf+12],ebx
mov	[regbuf+16],ecx
mov	[regbuf+20],edx
mov	[regbuf+24],esi
mov	[regbuf+28],edi
mov	[regbuf+32],ebp

mov	eax,[esp+8]			; get interrupt handler EIP from stack
mov	[regbuf],eax			; save it

mov	eax,[esp+12]			; get flags
mov	[regbuf+40],eax			; save flags

push	regbuf			; call exception handler
call	exception
add	esp,12

exit_exception:
mov	eax,[regbuf+8]
mov	ebx,[regbuf+12]
mov	ecx,[regbuf+16]
mov	edx,[regbuf+20]
mov	esi,[regbuf+24]
mov	edi,[regbuf+28]
mov	ebp,[regbuf+32]
iret

end_process:
call exit
iret

;
; Get stack base pointer
;
; In: Nothing
;
; Returns: stack base pointer
;
get_stack_base_pointer:
mov	eax,ebp
ret

;
; Low level dispatcher
;
; In: Nothing
;
; Returns: Nothing
;
d_lowlevel:
push	ds
push	es
push	fs
push	gs
push	eax
push	ebx
push	ecx
push	edx
push	esi
push	edi

mov 	eax,KERNEL_DATA_SELECTOR				; load the kernel data segment descriptor
mov 	ds,eax
mov 	es,eax
mov 	fs,eax
mov 	gs,eax

call	is_multitasking_enabled	
test	eax,eax
jz	no_disable_multitasking

mov	dword [multitasking_was_enabled],1

;call	disablemultitasking

no_disable_multitasking:

sti
call	dispatchhandler
cli

cmp	dword [multitasking_was_enabled],1
jnz	no_enable_multitasking

;call	enablemultitasking

no_enable_multitasking:
pop	edi
pop	esi
pop	edx
pop	ecx
pop	ebx
add	esp,4				; skip eax without popping it
pop	gs
pop	fs
pop	es
pop	ds
iret  

idt:
dw 0x3FFF				; limit
dd IDT_table				; base

IDT_table:
times 256 db 0,0,0,0,0,0,0,0

tempone dd 0
temptwo dd 0
intnumber dd 0
multitasking_was_enabled dd 0
regbuf times 20 dd 0

