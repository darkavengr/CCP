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
global yield
global switch_task_process_descriptor

extern save_kernel_stack_pointer
extern getpid
extern loadpagetable
extern get_kernel_stack_pointer
extern update_current_process_pointer
extern set_tss_rsp0
extern find_next_process_to_switch_to
extern is_current_process_ready_to_switch
extern reset_current_process_ticks
extern increment_tick_count
extern get_kernel_stack_top
extern increment_process_ticks
extern get_next_process_pointer
extern get_current_process_pointer
extern get_processes_pointer
extern enablemultitasking
extern disablemultitasking
extern getphysicaladdress
extern is_multitasking_enabled

%define offset

%include "kernelselectors.inc"
;
; The functions switch_task_process_descriptor and yield should not be called from 
; an interrupt handler. Instead, switch_task must be called.
;

switch_task_process_descriptor:
mov	[rel save_rax],rax

mov	rax,[rsp+4]				; get process descriptor
mov	[rel save_descriptor],rax
mov	rax,[rel save_rax]

; fall through to yield

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
; RIP (already on stack)
; CS  (already on stack)
; RFLAGS (already on stack)
; RBP
; RAX	
; RBX
; RCX
; RDX
; RSI
; RDI

; save rip, cs and rflags in the same way as an interrupt call

push	rax
mov	rax,[rsp+8]				; get rip
mov	[rel saverip],rax			; save it
pop	rax

sub	rsp,4

pushf
push	qword [rel saverip]

push	rax					; save registers
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

call	switch_task				; switch to next task

pop	r15					; restore registers
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

iretq						; jump to RIP

;
; Switch task
;
; This function is called by a function which is expected to save the registers onto the stack and call switch_task.
; After returning from switch_task, the calling function will then restore the registers and return, transferring control to the new process.
;
; You are not expected to understand this
;

switch_task:
mov	[rel save_rsp],rsp

call	is_multitasking_enabled			
test	rax,rax 				; return if multitasking is disabled
jnz	multitasking_enabled

jmp	end_switch

multitasking_enabled:
call	disablemultitasking

push	qword [rel save_rsp]			; save current task's stack pointer
call	save_kernel_stack_pointer
add	rsp,8

call	increment_process_ticks

mov	rax,[rel save_descriptor]			; get descriptor
test	rax,rax					; if switching to a specific process
jnz	task_time_slice_finished		; don't check if process is ready to switch

call	is_current_process_ready_to_switch
test	rax,rax					; if process not ready to switch, return
jnz	task_time_slice_finished

call	enablemultitasking

jmp	end_switch

task_time_slice_finished:
call	reset_current_process_ticks

call	getpid

cmp	rax,1
jne	no_debug

xchg	bx,bx
no_debug:

;
; Switch to next process
;

mov	rdi,[rel save_descriptor]			; get descriptor
test	rdi,rdi						; if not switching to a specific process
jnz	have_descriptor					; update process now

call	find_next_process_to_switch_to			; get pointer to next process to switch to
mov	rdi,rax

have_descriptor:
call	update_current_process_pointer
add	rsp,8

; load page tables

call	getpid

mov	rdi,rax
call	loadpagetable
add	rsp,4

call	get_kernel_stack_pointer
mov	rsp,rax						; switch kernel stack

; Patch rsp0 in the TSS. The scheduler will use the correct kernel stack on the next task switch
call	get_kernel_stack_top

mov	rdi,rax
call	set_tss_rsp0
add	rsp,8

call	enablemultitasking

end_switch:
xor	rax,rax
mov	[rel save_descriptor],rax				; clear descriptor
ret

save_rsp dq 0
save_rax dq 0
save_descriptor dq 0
saverip dq 0

