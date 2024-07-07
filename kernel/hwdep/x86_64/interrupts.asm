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

%define offset

%macro pushinterruptnumber_dummy 1
xor	rax,rax
push	rax
mov	rax,qword %1
push	rax
%endmacro

%macro pushinterruptnumber_no_dummy 1
xor	rax,rax
push	rax
mov	rax,qword %1
push	rax
%endmacro

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
extern enableirq
extern disableirq

[BITS 64]
use64
section .text

;
; Initialize interrupts
;
; In: Nothing
;
; Returns: Nothing

initialize_interrupts:

; Initialize exception interrupts

; Divide by zero

xor	rax,rax
push	rax
mov	rax,qword int0_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; Debug exception

mov	qword rax,1
push	rax
mov	rax,qword int1_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; Non-maskable interrupt

mov	qword rax,2
push	rax
mov	rax,qword int2_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; Breakpoint exception

mov	qword rax,3
push	rax
mov	rax,qword int3_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

;'Into detected overflow'

mov	qword rax,4
push	rax
mov	rax,qword int4_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector
mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; Out of bounds exception
mov	qword rax,5
push	rax
mov	rax,qword int5_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

;  Invalid opcode exception
mov	qword rax,6
push	rax
mov	rax,qword int6_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; No coprocessor exception
mov	qword rax,7
push	rax
mov	rax,qword int7_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; Double fault

mov	qword rax,8
push	rax
mov	rax,qword int8_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; Coprocessor segment overrun
mov	qword rax,9
push	rax
mov	rax,qword int9_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer


; Bad TSS
mov	qword rax,10
push	rax
mov	rax,qword int10_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; Segment not present
mov	qword rax,11
push	rax
mov	rax,qword int11_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; Stack fault
mov	qword rax,12
push	rax
mov	rax,qword int12_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; General protection fault
mov	qword rax,13
push	rax
mov	rax,qword int13_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer


; Page fault
mov	qword rax,14
push	rax
mov	rax,qword int14_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer


; Unknown interrupt exception
mov	qword rax,15
push	rax
mov	rax,qword int15_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; Coprocessor fault
mov	qword rax,16
push	rax
mov	rax,qword int16_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer


; Alignment check exception
mov	qword rax,17
push	rax
mov	rax,qword int17_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

; Machine check exception
mov	qword rax,18
push	rax
mov	rax,qword int18_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0x8e			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer

;
; Set syscall interrupt
;
mov	qword rax,0x21
push	rax
mov	rax,qword int0_handler
push	qword rax
mov	rax,qword 8
push	qword rax			; selector

mov	rax,qword 0xee			; flags
push	qword rax
call	set_interrupt
add	rsp,32				; fix stack pointer
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
mov	rax,qword idt
lidt	[rax]					; load interrupts
ret

;
; Set interrupt
;
; [rsp+16]			; Interrupt number
; [rsp+24]			; Address of interrupt handler
; [rsp+40]			; Selector
; [rsp+48]			; Flags
;
; Returns -1 on error or 0 on success
;

set_interrupt:
push	rbx		;+16
push	rcx		;+24
push	rdx		;+32
push	rsi		;+40
push	rdi		;+48
nop
nop

mov	rdx,[rsp+16]			; get interrupt number
mov	rax,[rsp+24]			; get address of interrupt handler
mov	rcx,[rsp+40]			; get selector
mov	rbx,[rsp+48]			; get flags

cmp	rdx,256				; check if valid interrupt number
jl	set_interrupt_number_is_ok

mov	rax,0xffffffffffffffff
jmp	end_set_interrupt

set_interrupt_number_is_ok:
shl	rdx,4				; each interrupt is 16 bytes long
mov	rdi,rdx

mov	rax,qword idttable
add	rdi,rax

mov	[rdi],ax			; bits 15-0

;mov	rsi,qword 0xffffffff0000ffff
and	rax,rsi				; get bits 31-16
shr	rax,16
mov	[rdi+6],ax

mov	rsi,0x00000000ffffffff		; get bits 32-64
and	rax,rsi				; get bits 32-64
shr	rax,32
mov	[rdi+8],rax

mov	[rsi+2],cx			; put selector
mov	[rsi+5],bx			; put flags
;lidt	[idttable]

xor	rax,rax				; return success

end_set_interrupt:
pop	rdi
pop	rsi
pop	rdx
pop	rcx
pop	rbx
ret

;
; Get address of interrupt handler
;
; In: Interrupt number
;
; Returns: -1 on error or address of interrupt handler
;

get_interrupt:
push	rbx
push	rcx
push	rdx
push	rsi
push	rdi
nop
nop

mov	rcx,[rsp+48]			; get address of interrupt handler
mov	rbx,[rsp+48]			; get interrupt number

cmp	rbx,256				; check if valid
jl	get_interrupt_number_is_ok

mov	rax,0xffffffffffffffff			; return error
ret

get_interrupt_number_is_ok:
mov	rbx,rax

shr	rbx,4				; each interrupt is eight bytes long
mov	rax,qword idttable
add	rbx,rax

xor	rax,rax
mov	eax,dword [rbx+8]			; bits 32-63
shr	rax,32

movzx	rbx,word [rbx+6]		; bits 16-31
shr	rbx,16
add	rax,rbx

movzx	rbx,word [rbx]			; bits 0-15
add	rax,rbx

xor	rax,rax				; return success

pop	rdi
pop	rsi
pop	rdx
pop	rcx
pop	rbx
ret

;
; Interrupt handlers
;

int0_handler:
pushinterruptnumber_dummy 0
jmp	int_common

int1_handler:
pushinterruptnumber_dummy 1
jmp	int_common	

int2_handler:
pushinterruptnumber_dummy 2
jmp	int_common	

int3_handler:
pushinterruptnumber_dummy 3
jmp	int_common	

int4_handler:
pushinterruptnumber_dummy 4
jmp	int_common	

int5_handler:
pushinterruptnumber_dummy 5
jmp	int_common	

int6_handler:
pushinterruptnumber_dummy 6
jmp	int_common	

int7_handler:
pushinterruptnumber_dummy 7
jmp	int_common	

int8_handler:
pushinterruptnumber_no_dummy 8
jmp	int_common	

int9_handler:
pushinterruptnumber_dummy 9
jmp	int_common	

int10_handler:
pushinterruptnumber_no_dummy 10
jmp	int_common	

int11_handler:
pushinterruptnumber_no_dummy 11
jmp	int_common	

int12_handler:
pushinterruptnumber_no_dummy 12
jmp	int_common	

int13_handler:
pushinterruptnumber_no_dummy 13
jmp	int_common	

int14_handler:
pushinterruptnumber_no_dummy 14
jmp	int_common	

int15_handler:
pushinterruptnumber_dummy 15
jmp	int_common	

int16_handler:
pushinterruptnumber_dummy 16
jmp	int_common	

int17_handler:
pushinterruptnumber_dummy 17
jmp	int_common	

int18_handler:
pushinterruptnumber_dummy 18
jmp	int_common	

int_common:
mov	r11,regbuf
mov	[r11+8],rsp			; save other registers into buffer to pass to exception handler
mov	[r11+16],rax
mov	[r11+24],rbx
mov	[r11+32],rcx
mov	[r11+40],rdx
mov	[r11+48],rsi
mov	[r11+56],rdi
mov	[r11+64],rbp

mov	rax,[rsp+16]			; get rip of interrupt handler from stack
mov	[r11],rax			; save it

mov	rax,[rsp+24]			; get flags
mov	[r11+68],rax			; save flags

mov	rax,qword regbuf

push	rax			; call exception handler
call	exception
add	rsp,12

exit_exception:
xchg	bx,bx
iretq

end_process:
call exit
iretq

;
; Get stack base pointer
;
; In: Nothing
;
; Returns: stack base pointer
;
get_stack_base_pointer:
mov	rax,rbp
ret

;
; low level dispatcher
;
; In: Nothing
;
; Returns: Nothing
;
d_lowlevel:
push	rax						; save registers
push	rbx
push	rcx
push	rdx
push	rsi
push	rdi
push	r10
push	r11
push	r12
push	r13
push	r14
push	r15

call	disablemultitasking

sti
call	dispatchhandler
cli

mov	r11,tempone
mov	[r11],rax

call	enablemultitasking

pop	r15						; restore registers
pop	r14
pop	r13
pop	r12
pop	r11
pop	r10
pop	rdi
pop	rsi
pop	rdx
pop	rcx
pop	rbx
pop	rax

cmp	rax,qword 0xffffffffffffffff					; if error ocurred
je	iretq_error

mov	r11,tempone
mov	rax,[r11]				; then return old rax

iretq_error:
iretq  

section .data
idt:
dw 0x3FFF					; limit
dq offset idttable				; base

idttable:
times 256 db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
idt_end:

tempone dq 0
temptwo dq 0
intnumber dq 0
regbuf times 20 dq 0

