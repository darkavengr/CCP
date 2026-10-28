;
;  CCP Version 0.0.1
;
;  (C) Matthew Boote 2020-2023
;
;  This file is part of CCP.
;
;  CCP is free software: you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation, either version 3 of the License, or
;  (at your option) any later version.
;  CCP is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;  You should have received a copy of the GNU General Public License
;  along with CCP.  If not, see <https://www.gnu.org/licenses/>.

global switch_task
global end_switch
global end_yield
global yield
global switch_task_process_descriptor

extern save_kernel_stack_pointer
extern getpid
extern switch_address_space
extern get_kernel_stack_pointer
extern set_tss_rsp0
extern get_kernel_stack_top
extern GetCurrentProcessFlags
extern SetCurrentProcessFlags
extern find_next_process_to_switch_to
extern update_current_process_pointer

PROCESS_NEW	equ 2
PROCESS_RUNNING	equ 8

%include "kernelselectors.inc"

;
; Force switch to next process
;
; In: Nothing
;
; Returns: Nothing
;
yield:
cli

;
; Stack format:
;
; RIP      
; CS
; RFLAGS
; RAX	
; RBX
; RCX
; RDX
; RSI
; RDI
; R8
; R9
; R10
; R11
; R12
; R13
; R14
; R15
; RBP

; save rip, cs and rflags in the same way as an interrupt call

push	rax
mov	rax,[rsp+8]				; get rip
mov	[rel save_rip],rax				; save it
pop	rax

; create fake interrupt return stack
pushf						; eflags
push	qword KERNEL_CODE_SELECTOR		; cs
push	qword [rel save_rip]			; rip

push	rax				; save registers
push	rbx
push	rcx
push	rdx
push	rsi
push	rdi
push	r8
push	r9
push	r10
push	r11
push	r12
push	r13
push	r14
push	r15
push	rbp

mov	[rel OldContextPointer],rsp

call	find_next_process_to_switch_to
test	rax,rax					; no processes
jz	no_processes

mov	rdi,rax
mov	rsi,[rel OldContextPointer]
call	switch_task				; switch to next task

; never reaches here
no_processes:
add	rsp,19*8				; move stack back to where it was before the fake interrupt return stack was created
xchg	bx,bx
iretq

;
; Switch task
;
switch_task:
mov	[rel OldContextPointer],rdi
mov	[rel NextProcessPointer],rsi

call	getpid
test	rax,rax
jnz	skiptest

call	GetCurrentProcessFlags			; get process flags

and	rax,PROCESS_NEW				; get new process flag
test	rax,rax					; if new process, skip saving kernel stack pointer
jnz	not_first

skiptest:
mov	rdi,[rel OldContextPointer]
call	save_kernel_stack_pointer		; save kernel stack pointer for current process

not_first:
call	GetCurrentProcessFlags			; get process flags

and	rax,~PROCESS_NEW			; clear new process flag
or	rax,PROCESS_RUNNING			; set process running flag

mov	rdi,rax
call	SetCurrentProcessFlags

;
; Switch to next process
;

mov	rdi,[rel NextProcessPointer]		; update current process pointer
call	update_current_process_pointer

; switch address space

call	getpid
test	rax,rax
jz	no_debug

xchg	bx,bx
no_debug:

call	getpid

mov	rdi,rax
call	switch_address_space

; Patch RSP0 in the TSS. The scheduler will use the correct kernel stack on the next task switch
call	get_kernel_stack_top

mov	rdi,rax
call	set_tss_rsp0

call	get_kernel_stack_pointer
mov	rsp,rax					; switch kernel stack

;
; Return directly to process. Does *not* return to the caller
;

pop	rbp				; restore registers
pop	r15
pop	r14
pop	r13
pop	r12
pop	r11
pop	r10
pop	r9
pop	r8
pop	rdi
pop	rsi
pop	rdx
pop	rcx
pop	rbx
pop	rax
iretq
nop
nop

OldContextPointer dq 0
NextProcessPointer dq 0
save_rip dq 0

