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
global int_common
global d_lowlevel

extern exception					; exception handler
extern exit
extern dispatchhandler					; high-level dispatcher
extern enablemultitasking
extern disablemultitasking

[BITS 64]
use64

%macro initializeinterrupt 4
mov	rdi,qword %1			; flags
mov	rsi,qword %2			; selector
mov	rdx,qword %3			; address
mov	rcx,qword %4			; interrupt number
call	set_interrupt
%endmacro

%macro inthandler_noerr 1
int%1_handler:
push	rdi
push	rax
push	rbx

mov	rbx,qword exception_number	; save exception number
mov	rax,%1				; exception number
mov	[rbx],rax

mov	rax,[rsp+32]		; get rip from stack
mov	rbx,[rsp+32+16]		; get rflags from stack

jmp	int_common
%endmacro

%macro inthandler_err 1
int%1_handler:
push	rdi
push	rax
push	rbx

mov	rbx,qword exception_number	; save exception number
mov	rax,%1				; exception number
mov	[rbx],rax

mov	rbx,[rsp+32+32]			; save error code
mov	rax,qword error_code
mov	[rbx],rax

mov	rax,[rsp+32+8]		; get rip from stack
mov	rbx,[rsp+32+24]		; get rflags from stack

jmp	int_common
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

inthandler_noerr 0
inthandler_noerr 1
inthandler_noerr 2
inthandler_noerr 3
inthandler_noerr 4
inthandler_noerr 5
inthandler_noerr 6
inthandler_noerr 7
inthandler_err 8
inthandler_noerr 9
inthandler_err 10
inthandler_err 11
inthandler_err 12
inthandler_err 13
inthandler_err 14
inthandler_noerr 15
inthandler_noerr 16
inthandler_err 17
inthandler_noerr 18
inthandler_noerr 19
inthandler_noerr 20
inthandler_err 21
inthandler_noerr 22
inthandler_noerr 23
inthandler_noerr 24
inthandler_noerr 25
inthandler_noerr 26
inthandler_noerr 27
inthandler_noerr 28
inthandler_err 29
inthandler_err 30
inthandler_noerr 31

int_common:
; save other registers into buffer to pass to exception handler

mov	rdi,qword regbuf

mov	[rdi],rax			; save eip
mov	[rdi+8],rsp
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
mov	[rdi+120],rbx			; save flags

pop	rax				; get rax from stack
mov	[rdi+16],rax
pop	rax				; get rbx from stack
mov	[rdi+24],rax
pop	rax				; get rdi from stack
mov	[rdi+56],rax

mov	rdi,rax

mov	r11,qword exception_number
mov	rsi,[r11]			; exception number
call	exception			; call exception handler

;  exceptions 8, 10, 11, 12, 13,14,17,21,29 and 30 push an error code

mov	rdx,qword exception_number
mov	rdx,[rdx]

mov	rsi,qword error_number_exceptions
mov	rcx,10

next_check_exeception_number:
lodsq

cmp	rax,rdx
je	skip_pushed_error_number

jmp	short next_check_exeception_number

jmp	exit_exception

skip_pushed_error_number:
add	rsp,8

exit_exception:
mov	rdi,qword regbuf

mov	rax,[rdi+16]			; restore registers
mov	rbx,[rdi+24]
mov	rcx,[rdi+32]
mov	rdx,[rdi+40]
mov	rsi,[rdi+48]
mov	r11,[rdi+64]
mov	r12,[rdi+72]
mov	r13,[rdi+80]
mov	r14,[rdi+88]
mov	r15,[rdi+96]
mov	rbp,[rdi+64]
mov	rdi,[rdi+56]
iretq
;
; low level dispatcher
;
;
; In: Nothing
;
; Returns: Nothing
;
d_lowlevel:
mov	r9,rax
mov	r8,rbx
; rcx and rdx are unchanged
mov	rsi,rbx
mov	rdi,rax

;push	rax
;push	rbx
;push	rcx
;push	rdx
;push	rsi
;push	rdi

;call	disablemultitasking

;pop	rdi
;pop	rsi
;pop	rdx
;pop	rcx
;pop	rbx
;pop	rax

sti

;xchg	bx,bx
call	dispatchhandler

;call	enablemultitasking
cli

mov	r11,qword tempone
mov	[r11],rax

mov	r11,qword 0xffffffffffffffff
cmp	rdx,r11					; if error ocurred
je	iretq_error

mov	r11,qword tempone
mov	rax,[r11]				; then return old rax

iretq_error:
iretq  

idt:
dw 0x3FFF					; limit
dq idttable				; base

idttable:
times 256 db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
idt_end:

error_number_exceptions dq 8,10,11,12,13,14,17,21,29,30
end_error_number_exceptions:
tempone dq 0
temptwo dq 0
intnumber dq 0
exception_number dq 0
error_code dq 0
regbuf times 120 dq 0

