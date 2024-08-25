;
;  CCP Version 0.0.1
;    (C) Matthew Boote 2020-2022

;    This file is part of CCP.

;    CCP is free software: you can rrdistribute it and/or modify
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
%include "init.inc"

global disable_interrupts				; disable interrupts
global enable_interrupts				; enable interrupts
global set_interrupt					; set interrupt
global get_interrupt					; get interrupt
global load_idt
global initialize_interrupts
global int0_handler
global int1_handler
global int2_handler
global int3_handler
global int4_handler
global int5_handler
global int6_handler
global int7_handler
global int8_handler
global int9_handler
global int10_handler
global int11_handler
global int12_handler
global int13_handler
global int14_handler
global int15_handler
global int16_handler
global int17_handler
global int18_handler
global d_lowlevel

extern exception					; exception handler
extern exit
extern dispatchhandler					; high-level dispatcher
extern disablemultitasking
extern enablemultitasking
extern enableirq
extern disableirq

[BITS 64]
use64
section .text

%macro initializeinterrupt 4
mov	rdi,qword %1			; flags
mov	rsi,qword %2			; selector
mov	rdx,qword %3			; address
mov	rcx,qword %4			; interrupt number
call	set_interrupt
%endmacro

;
; Initialize interrupts
;
; In: Nothing
;
; Returns: Nothing

initialize_interrupts:

; Initialize exception interrupts

initializeinterrupt 0x8e00,8,int0_handler,0		; Divide by zero
initializeinterrupt 0x8e00,8,int1_handler,1		; Debug exception
initializeinterrupt 0x8e00,8,int2_handler,2		; Non-maskable interrupt
initializeinterrupt 0x8e00,8,int3_handler,3		; Breakpoint exception
initializeinterrupt 0x8e00,8,int4_handler,4		;'Into detected overflow'
initializeinterrupt 0x8e00,8,int5_handler,5		; Out of bounds exception
initializeinterrupt 0x8e00,8,int6_handler,6		; Invalid opcode exception
initializeinterrupt 0x8e00,8,int7_handler,7		; No coprocessor exception
initializeinterrupt 0x8e00,8,int8_handler,8		; Double fault
initializeinterrupt 0x8e00,8,int9_handler,9		; Coprocessor segment overrun
initializeinterrupt 0x8e00,8,int10_handler,10		; Invalid TSS
initializeinterrupt 0x8e00,8,int11_handler,11		; Segment not present
initializeinterrupt 0x8e00,8,int12_handler,12		; Stack fault
initializeinterrupt 0x8e00,8,int13_handler,13		; General protection fault
initializeinterrupt 0x8e00,8,int14_handler,14		; Page fault
initializeinterrupt 0x8e00,8,int15_handler,15		; Unknown interrupt exception
initializeinterrupt 0x8e00,8,int16_handler,16		; Coprocessor fault
initializeinterrupt 0x8e00,8,int17_handler,17		; Alignment check exception
initializeinterrupt 0x8e00,8,int18_handler,18		; Machine check exception
initializeinterrupt 0xee00,8,d_lowlevel,0x21		; Syscall interrupt
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
mov	rax,qword idt
lidt	[rax]					; load interrupts
ret

;
; Set interrupt
;
; RDI			; Flags
; RSI			; Selector
; RDX			; Address of interrupt handler
; RCX			; Interrupt number
; 
; Returns -1 on error or 0 on success
;

set_interrupt:
mov	rbx,qword 256
cmp	rcx,rbx					; check if valid interrupt number
jl	set_interrupt_number_is_ok

mov	rax,qword 0xffffffffffffffff	; return error
jmp	end_set_interrupt

set_interrupt_number_is_ok:
mov	rax,rcx				; point to entry in the IDT
shl	rax,4				; each interrupt is 16 bytes long

mov	rbx,qword idttable		; point to IDT
add	rax,rbx				; point to IDT entry

xor	ebx,ebx
mov	[rax+12],ebx			; put 0

mov	[rax],dx			; bits 0-15

shr	rdx,16				; get bits 16-31
mov	[rax+6],dx

shr	rdx,16				; get bits 32-63
mov	[rax+8],edx

mov	rbx,rsi
mov	[rax+2],bx			; put selector

mov	[rax+4],di			; put flags

xor	rax,rax				; return success

end_set_interrupt:
ret

;
;
; Interrupt handlers
;

int0_handler:
mov	rdi,qword regbuf
mov	rsi,qword 0
jmp	int_common

int1_handler:
mov	rdi,qword regbuf
mov	rsi,qword 1
jmp	int_common	

int2_handler:
mov	rdi,qword regbuf
mov	rsi,qword 2
jmp	int_common	

int3_handler:
mov	rdi,qword regbuf
mov	rsi,qword 3
jmp	int_common	

int4_handler:
mov	rdi,qword regbuf
mov	rsi,qword 4
jmp	int_common	

int5_handler:
mov	rdi,qword regbuf
mov	rsi,qword 5
jmp	int_common	

int6_handler:
mov	rdi,qword regbuf
mov	rsi,qword 6
jmp	int_common	

int7_handler:
mov	rdi,qword regbuf
mov	rsi,qword 7
jmp	int_common	

int8_handler:
mov	rdi,qword regbuf
mov	rsi,qword 8
jmp	int_common	

int9_handler:
mov	rdi,qword regbuf
mov	rsi,qword 9
jmp	int_common	

int10_handler:
mov	rdi,qword regbuf
mov	rsi,qword 10
jmp	int_common	

int11_handler:
mov	rdi,qword regbuf
mov	rsi,qword 11
jmp	int_common	

int12_handler:
mov	rdi,qword regbuf
mov	rsi,qword 12
jmp	int_common	

int13_handler:
mov	rdi,qword regbuf
mov	rsi,qword 13
jmp	int_common	

int14_handler:
mov	rdi,qword regbuf
mov	rsi,qword 14
jmp	int_common	

int15_handler:
mov	rdi,qword regbuf
mov	rsi,qword 15
jmp	int_common	

int16_handler:
mov	rdi,qword regbuf
mov	rsi,qword 16
jmp	int_common	

int17_handler:
mov	rdi,qword regbuf
mov	rsi,qword 17
jmp	int_common	

int18_handler:
mov	rdi,qword regbuf
mov	rsi,qword 18
;jmp	int_common	

int_common:
push	rdi
mov	rdi,regbuf
mov	[rdi+8],rsp			; save other registers into buffer to pass to exception handler
mov	[rdi+16],rax
mov	[rdi+24],rbx
mov	[rdi+32],rcx
mov	[rdi+40],rdx
mov	[rdi+48],rsi
mov	[rdi+64],rbp
mov	[rdi+72],r10
mov	[rdi+80],r11
mov	[rdi+88],r12
mov	[rdi+96],r13
mov	[rdi+104],r14
mov	[rdi+112],r15

pop	rax
mov	[rdi+56],rax

mov	rax,[rsp+16]			; get flags
mov	[rdi+120],rax			; save flags

mov	rax,[rsp+8]			; get RIP of interrupt handler from stack
mov	[rdi],rax			; save it

call	exception			; call exception handler
add	rsp,16

exit_exception:
xchg	bx,bx
iretq

;
; low level dispatcher
;
; In: Nothing
;
; Returns: Nothing
;
d_lowlevel:
mov	r10,rsi				; pass registers
mov	r11,rdi

mov	rdi,rax
mov	rsi,rbx
mov	r8,rcx
mov	r9,rdx

call	disablemultitasking

sti
call	dispatchhandler
cli

mov	r11,tempone
mov	[r11],rax

call	enablemultitasking

cmp	rdx,qword 0xffffffffffffffff					; if error ocurred
je	iretq_error

mov	r11,tempone
mov	rax,[r11]				; then return old rax

iretq_error:
iretq  

idt:
dw 0x3FFF					; limit
dq idttable				; base

idttable:
times 256 db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
idt_end:

tempone dq 0
temptwo dq 0
intnumber dq 0
regbuf times 20 dq 0

