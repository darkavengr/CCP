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
global initializestack
global initializekernelstack
global create_initial_stack_entries

extern irq_exit
extern get_kernel_stack_top
extern get_process_stack_size
extern save_kernel_stack_pointer

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
; In:	Stack pointer address
;
; Returns: Nothing
;
initializekernelstack:
mov	rsi,[rsp+4]				; kernel stack top
mov	rdx,[rsp+8]				; entry point
mov	rcx,[rsp+12]				; kernel stack bottom

mov	rdi,rsi
sub	rdi,12*8				; space for initial stack frame

mov	rax,irq_exit
mov	[rdi],rax

add	rdi,8

xor	rax,rax					; zero register values
mov	rcx,11
rep	stosq

mov	qword [rdi+104],rdx			; rip
mov	qword [rdi+112],KERNEL_CODE_SELECTOR	; cs
mov	qword [rdi+120],0x200			; rflags
mov	qword [rdi+128],rbx			; rsp

;
; Adjust kernel stack pointer
; so it points to intial values
;
call	get_kernel_stack_top
sub	rax,12*8

push	rax
call	save_kernel_stack_pointer
add	rsp,8
ret

section .data
temprip dq 0

