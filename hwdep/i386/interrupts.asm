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

%define offset

%include "kernelselectors.inc"

global disable_interrupts				; disable interrupts
global enable_interrupts				; enable interrupts
global set_interrupt					; set interrupt
global get_interrupt					; get interrupt
global load_idt
global initialize_interrupts

extern exception					; exception handler
extern exit
extern dispatchhandler					; high-level dispatcher
extern disablemultitasking
extern enablemultitasking

section .text
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

push	0
push	offset int0_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Debug exception

push	1
push	offset int1_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Non-maskable interrupt

push	2
push	offset int2_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Breakpoint exception

push	3
push	offset int3_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

;'Into detected overflow'

push	4
push	offset int4_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Out of bounds exception

push	5
push	offset int5_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

;  Invalid opcode exception

push	6
push	offset int6_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; No coprocessor exception

push	7
push	offset int7_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Double fault

push	8
push	offset int8_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Coprocessor segment overrun

push	9
push	offset int9_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Bad TSS

push	10
push	offset int10_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Segment not present

push	11
push	offset int11_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Stack fault

push	12
push	offset int12_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; General protection fault

push	13
push	offset int13_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Page fault

push	14
push	offset int14_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Unknown interrupt exception

push	15
push	offset int15_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Coprocessor fault

push	16
push	offset int16_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Alignment check exception

push	17
push	offset int17_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

; Machine check exception

push	18
push	offset int18_handler
push	8				; selector
push	8eh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer

;
; Set syscall interrupt
;
push	0x21
push	offset d_lowlevel
push	8				; selector
push	0eeh				; flags
call	set_interrupt
add	esp,16				; fix stack pointer
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
add	edi,offset idttable

mov	[edi],ax			; write low word
mov	[edi+2],cx			; put selector

xor	cl,cl
mov	[edi+4],cl			; put 0
mov	[edi+5],bl			; put flags

shr	eax,16
mov	[edi+6],ax			; put high word

;lidt	[idttable]

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

add	ebx,offset idttable

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
mov	[regbuf+4],esp			; save other registers into buffer to pass to exception handler
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
; low level dispatcher
;
; In: Nothing
;
; Returns: Nothing
;
d_lowlevel:
;xchg	bx,bx

push	eax
push	ebx
push	ecx
push	edx
push	esi
push	edi

mov 	ax,KERNEL_DATA_SELECTOR				; load the kernel data segment descriptor
mov 	ds,ax
mov 	es,ax
mov 	fs,ax
mov 	gs,ax

call	disablemultitasking

sti
call	dispatchhandler
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

cmp	eax,0ffffffffh					; if error ocurred
je	iret_error

mov	eax,[tempone]					; then return old eax

iret_error:

call	enablemultitasking
iret  

section .data
idt:
dw 0x3FFF					; limit
dd offset idttable				; base

idttable:
times 256 db 0,0,0,0,0,0,0,0
idt_end:

tempone dd 0
temptwo dd 0
intnumber dd 0
regbuf times 20 dd 0

