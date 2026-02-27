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
extern set_tss_esp0
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
; RCX
; RDX
; RBX
; Useless RSP
; RBP
; RSI
; RDI

; save rip, cs and rflags in the same way as an interrupt call

push	rax
mov	rax,[rsp+8]				; get eip
mov	[save_rip],rax				; save it
pop	rax

; create fake interrupt return stack
pushf						; eflags
push	qword KERNEL_CODE_SELECTOR		; cs
push	qword [rel save_rip]			; eip
push	rax					; save registers
push	rcx
push	rdx
push	rbx
push	rsp
push	rbp
push	rsi
push	rdi
mov	[rel OldContextPointer],rsp

call	find_next_process_to_switch_to

mov	rdi,rax
mov	rsi,[rel OldContextPointer]
call	switch_task				; switch to next task

pop	rdi					; restore registers
pop	rsi
pop	rbp
pop	rsp
pop	rbx
pop	rdx
pop	rcx
pop	rax
iret

;
; Switch task
;
switch_task:
mov	[OldContextPointer],rdi
mov	[NextProcessPointer],rsi

inc	byte [0x800b8000]

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

call	getpid
test	rax,rax
jz	no_debug

xchg	bx,bx
no_debug:

; switch address space

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
pop	rdi					; restore registers
pop	rsi
pop	rbp
pop	rsp
pop	rbx
pop	rdx
pop	rcx
pop	rax

pop	gs
pop	fs
pop	es
pop	ds
iret

OldContextPointer dq 0
NextProcessPointer dq 0
save_rip dq 0

