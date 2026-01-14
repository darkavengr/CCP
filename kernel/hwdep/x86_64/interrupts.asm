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
%include "idtflags.inc"

IDT_ENTRY_OFFSET1	equ 0
IDT_ENTRY_SELECTOR	equ 2
IDT_ENTRY_IST		equ 4
IDT_ENTRY_FLAGS		equ 5
IDT_ENTRY_OFFSET2	equ 6
IDT_ENTRY_OFFSET3	equ 8
IDT_ZERO		equ 12

global disable_interrupts				; disable interrupts
global enable_interrupts				; enable interrupts
global set_interrupt					; set interrupt
global get_interrupt					; get interrupt
global load_idt
global initialize_interrupts
global int_common
global d_lowlevel

extern exception					; exception handler
extern exit
extern dispatchhandler					; high-level dispatcher
extern enablemultitasking
extern disablemultitasking

[BITS 64]
use64

%macro inthandler_noerr 1
int%1_handler:
push	rdi
push	rax
push	rbx

mov	rbx,qword exception_number	; save exception number
mov	rax,%1				; exception number
mov	[rbx],rax

mov	rax,[rsp+24]		; get rip from stack
mov	rbx,[rsp+24+16]		; get rflags from stack

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

mov	rbx,qword error_code
mov	rax,[rsp+24+32]			; save error code
mov	[rbx],rax

mov	rax,[rsp+24+8]		; get rip from stack
mov	rbx,[rsp+24+24]		; get rflags from stack

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

; Divide by zero

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int0_handler		; handler
xor	rcx,rcx				; interrupt number
call	set_interrupt

; Debug exception

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int1_handler		; handler
mov	rcx,qword 1			; interrupt number
call	set_interrupt

; Non-maskable interrupt
mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int2_handler		; handler
mov	rcx,qword 2			; interrupt number
call	set_interrupt

; Breakpoint exception

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int3_handler		; handler
mov	rcx,qword 3			; interrupt number
call	set_interrupt

;'Into detected overflow'
mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int4_handler		; handler
mov	rcx,qword 4			; interrupt number
call	set_interrupt

; Out of bounds exception

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int5_handler		; handler
mov	rcx,qword 5			; interrupt number
call	set_interrupt

;  Invalid opcode exception

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int6_handler		; handler
mov	rcx,qword 6			; interrupt number
call	set_interrupt

; No coprocessor exception

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int7_handler		; handler
mov	rcx,qword 7			; interrupt number
call	set_interrupt

; Double fault
mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int8_handler		; handler
mov	rcx,qword 8			; interrupt number
call	set_interrupt

; Coprocessor segment overrun

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int9_handler		; handler
mov	rcx,qword 9			; interrupt number
call	set_interrupt

; Invalid TSS
mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int10_handler		; handler
mov	rcx,qword 10			; interrupt number
call	set_interrupt

; Segment not present

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int11_handler		; handler
mov	rcx,qword 11			; interrupt number
call	set_interrupt

; Stack fault

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int12_handler		; handler
mov	rcx,qword 12			; interrupt number
call	set_interrupt

; General protection fault

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int13_handler		; handler
mov	rcx,qword 13			; interrupt number
call	set_interrupt

; Page fault

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int14_handler		; handler
mov	rcx,qword 14			; interrupt number
call	set_interrupt

; Invalid interrupt exception

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int15_handler		; handler
mov	rcx,qword 15			; interrupt number
call	set_interrupt

; Coprocessor fault

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int16_handler		; handler
mov	rcx,qword 16			; interrupt number
call	set_interrupt

; Alignment check exception

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int17_handler		; handler
mov	rcx,qword 17			; interrupt number
call	set_interrupt

; Machine check exception

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING0 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword int18_handler		; handler
mov	rcx,qword 18			; interrupt number
call	set_interrupt

;
; Set syscall interrupt
;

mov	rdi,qword IDT_ENTRY_PRESENT | IDT_RING3 | IDT_32BIT_64BIT_INTERRUPT_GATE	; flags
mov	rsi,qword KERNEL_CODE_SELECTOR	; selector
mov	rdx,qword d_lowlevel		; handler
mov	rcx,qword 0x21			; interrupt number
call	set_interrupt
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
mov	[rax+IDT_ZERO],ebx		; put 0
;mov	[rax+IDT_ENTRY_IST],bl

mov	[rax+IDT_ENTRY_OFFSET1],dx	; bits 0-15

shr	rdx,16				; get bits 16-31
mov	[rax+IDT_ENTRY_OFFSET2],dx

shr	rdx,16				; get bits 32-63
mov	[rax+IDT_ENTRY_OFFSET3],edx

mov	rbx,rsi
mov	[rax+IDT_ENTRY_SELECTOR],bx	; put selector

mov	rbx,rdi
mov	[rax+IDT_ENTRY_FLAGS],bl	; put flags

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

mov	rdi,qword regbuf

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
je	found_pushed_error_number

jmp	short next_check_exeception_number

jmp	exit_exception

found_pushed_error_number:
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

push	rax
push	rbx
push	rcx
push	rdx
push	rsi
push	rdi

call	disablemultitasking

pop	rdi
pop	rsi
pop	rdx
pop	rcx
pop	rbx
pop	rax

sti
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

