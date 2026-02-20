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

KERNEL_STACK_SIZE equ  65536*5			; size of initial kernel stack
INITIAL_KERNEL_STACK_ADDRESS equ 0x20000		; intial kernel stack address

global initialize_current_process_user_mode_stack
global initialize_current_process_kernel_stack
global create_initial_stack_entries
global get_initial_kernel_stack_base
global get_initial_kernel_stack_top
global get_kernel_stack_size

extern irq_exit
extern get_kernel_stack_top
extern get_process_stack_size
extern save_kernel_stack_pointer
extern set_tss_rsp0

[BITS 64]
use64

;
; Initialize user-mode stack
;
; In:	rdi	Stack pointer address
;	rsi	Stack size
;
; Returns: Nothing
;
initialize_current_process_user_mode_stack:
mov	r11,[rsp]					; get RIP

mov	rax,rdi
sub	rax,rsi					; minus stack size

mov	rsp,rdi
mov	rbp,rax

jmp	r11					; return without using stack

;
; Initialize kernel-mode stack
;
; In:	rdi=Kenrel stack pointer top
;	rsi=Stack bottom
;	rdx=Entry point
;	rcx=User-mode stack top

; Returns: Nothing
;
initialize_current_process_kernel_stack:
sub	rsi,17*8

mov	qword [rsi],USER_DATA_SELECTOR			; user ss
mov	qword [rsi-8],ecx				; user esp
mov	qword [rsi-16],0x200			; user eflags; enable interrupts on iret
mov	qword [rsi-32],USER_CODE_SELECTOR	; user cs
mov	qword [rsi-40],rdx			; user eip

mov	qword [rsi-48],USER_DATA_SELECTOR	; ds
mov	qword [rsi-56],USER_DATA_SELECTOR	; es
mov	qword [rsi-64],USER_DATA_SELECTOR	; fs
mov	qword [rsi-72],USER_DATA_SELECTOR	; gs
mov	qword [rsi-80],0xFFFFFFFFFFFFFFFF	; eax
mov	qword [rsi-88],0xEEEEEEEEEEEEEEEE	; ecx
mov	qword [rsi-96],0xDDDDDDDDDDDDDDDD	; edx
mov	qword [rsi-104],0xCCCCCCCCCCCCCCCC	; ebx
mov	[rsi-112],rdi				; esp
mov	[rsi-120],rsi				; ebp
mov	qword [rsi-128],0xBBBBBBBBBBBBBBBB	; esi
mov	qword [rsi-134],0xAAAAAAAAAAAAAAAA	; edi

;
; Adjust kernel stack pointer
; so it points to intial values
;
call	get_kernel_stack_top
sub	rax,17*8

mov	rdi,rax
call	save_kernel_stack_pointer

; Set TSS ESP0 to ensure that the kernel stack is switched to next time the cpu switches to ring 0.

mov	rdi,rax
call	set_tss_rsp0
ret

initial_exit:
pop	r15
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
iretq

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

temprip dq 0

