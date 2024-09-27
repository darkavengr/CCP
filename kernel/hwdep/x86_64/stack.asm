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


%include "kernelselectors.inc"

KERNEL_STACK_SIZE equ  65536*3				; size of initial kernel stack
INITIAL_KERNEL_STACK_ADDRESS equ 0x70000		; intial kernel stack address

global initializestack
global initializekernelstack
global create_initial_stack_entries
global get_initial_kernel_stack_base
global get_initial_kernel_stack_top
global get_kernel_stack_size

extern irq_exit
extern get_kernel_stack_top
extern get_process_stack_size
extern save_kernel_stack_pointer
extern set_tss_rsp0

section .text
[BITS 64]
use64

;
; Initialize user-mode stack
;
; In:	Stack pointer address
;	Stack size
;
; Returns: Nothing
;
initializestack:
cli
mov	rax,[rsp]					; get rip
mov	r11,temprip
mov	[r11],rax

mov	rbx,[rsp+8]					; get stack size
mov	rax,[rsp+4]					; get stack pointer

mov	rbx,rax
sub	rbx,[rsp+8]					; minus stack size

mov	rsp,rax
mov	rbp,rbx

jmp	[r11]					; return without using stack

;
; Initialize kernel-mode stack
;
; In:	rdi=Stack pointer top
;	rsi=Stack bottom
;	rdx=Entry point
;

; Returns: Nothing
;
initializekernelstack:
mov	r11,rdi
sub	r11,17*8				; space for initial stack frame

mov	rax,irq_exit
mov	[r11],rax

; fill in zeroes for rax,rbx,rcx ,rdx,rsi,rdi,r10,r11,r12,r13,r14,r15

push	rdi

mov	rdi,r11
add	rdi,8

mov	rcx,12

xor	rax,rax
rep	stosq

pop	rdi

mov	[r11+104],rdx			; eip
mov	qword [r11+112],KERNEL_CODE_SELECTOR	; cs
mov	qword [r11+116],0x200			; rflags
mov	[r11+124],rdi			; rsp

;
; Adjust kernel stack pointer
; so it points to intial values
;
call	get_kernel_stack_top
sub	rax,17*4

push	rax
call	save_kernel_stack_pointer
add	rsp,8

; Set tss esp0 to ensure that the kernel stack is switched to next time the cpu switches to ring 0.

push	rdi
call	set_tss_rsp0
add	rsp,8
ret

get_initial_kernel_stack_base:
mov	rax,INITIAL_KERNEL_STACK_ADDRESS
ret

get_initial_kernel_stack_top:
mov	rax,INITIAL_KERNEL_STACK_ADDRESS
add	rax,KERNEL_STACK_SIZE
ret

get_kernel_stack_size:
mov	rax,KERNEL_STACK_SIZE
ret

section .data
temprip dq 0

