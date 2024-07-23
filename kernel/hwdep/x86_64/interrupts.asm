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
push	dword %1			; interrupt number
mov	rax,qword %2			; handler address
push	rax
push	dword %3			; selector
push	dword %4			; flags
call	set_interrupt
add	rsp,32				; fix stack pointer
%endmacro

;
; Initialize interrupts
;
; In: Nothing
;
; Returns: Nothing

initialize_interrupts:

; Initialize exception interrupts

initializeinterrupt 0,int0_handler,8,0x8e00		; Divide by zero
initializeinterrupt 1,int1_handler,8,0x8e00		; Debug exception
initializeinterrupt 2,int2_handler,8,0x8e00		; Non-maskable interrupt
initializeinterrupt 3,int3_handler,8,0x8e00		; Breakpoint exception
initializeinterrupt 4,int4_handler,8,0x8e00		;'Into detected overflow'
initializeinterrupt 5,int5_handler,8,0x8e00		; Out of bounds exception
initializeinterrupt 6,int6_handler,8,0x8e00		;  Invalid opcode exception
initializeinterrupt 7,int7_handler,8,0x8e00		; No coprocessor exception
initializeinterrupt 8,int8_handler,8,0x8e00		; Double fault
initializeinterrupt 9,int9_handler,8,0x8e00		; Coprocessor segment overrun
initializeinterrupt 10,int10_handler,8,0x8e00	; Invalid TSS
initializeinterrupt 11,int11_handler,8,0x8e00	; Segment not present
initializeinterrupt 12,int12_handler,8,0x8e00	; Stack fault
initializeinterrupt 13,int13_handler,8,0x8e00	; General protection fault
initializeinterrupt 14,int14_handler,8,0x8e00	; Page fault
initializeinterrupt 15,int15_handler,8,0x8e00	; Unknown interrupt exception
initializeinterrupt 16,int16_handler,8,0x8e00	; Coprocessor fault
initializeinterrupt 17,int17_handler,8,0x8e00	; Alignment check exception
initializeinterrupt 18,int18_handler,8,0x8e00	; Machine check exception
initializeinterrupt 0x21,d_lowlevel,8,0xee00	; Syscall interrupt
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
; [rsp+32]			; Interrupt number
; [rsp+24]			; Address of interrupt handler
; [rsp+16]			; Selector
; [rsp+8]			; Flags
;
; Returns -1 on error or 0 on success
;
set_interrupt:
mov	rax,[rsp+32]
cmp	rax,256				; check if valid interrupt number
jl	set_interrupt_number_is_ok

mov	rax,0xffffffffffffffff
jmp	end_set_interrupt

set_interrupt_number_is_ok:
mov	rdi,rax				; point to entry in the IDT
shl	rdi,4				; each interrupt is 16 bytes long

mov	rax,qword idttable
add	rdi,rax

xor	edx,edx
mov	[rdi+12],edx

mov	rax,[rsp+24]			; get address
mov	[rdi],ax			; bits 0-15

shr	rax,16				; get bits 16-31
mov	[rdi+6],ax

shr	rax,16				; get bits 32-63
mov	[rdi+8],eax

mov	rax,[rsp+16]
mov	[rdi+2],ax			; put selector

mov	rax,[rsp+8]
mov	[rdi+4],ax			; put flags

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

