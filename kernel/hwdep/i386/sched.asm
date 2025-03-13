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

extern is_multitasking_enabled			
extern save_kernel_stack_pointer
extern getpid
extern loadpagetable
extern get_kernel_stack_pointer
extern update_current_process_pointer
extern set_tss_esp0
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

%define offset

%include "kernelselectors.inc"
;
; The functions switch_task_process_descriptor() and yield() should not be called from 
; an interrupt handler. Instead, switch_task() must be called.
;

switch_task_process_descriptor:
mov	[save_eax],eax

mov	eax,[esp+4]				; get process descriptor
mov	[save_descriptor],eax
mov	eax,[save_eax]

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
; return-to-yield() address
; EIP      
; CS
; EFLAGS
; EAX	
; ECX
; EDX
; EBX
; Useless ESP
; EBP
; ESI
; EDI

; save eip, cs and eflags in the same way as an interrupt call

push	eax
mov	eax,[esp+4]				; get eip
mov	[save_eip],eax				; save it
pop	eax

; create fake interrupt return stack
pushf						; eflags
push	dword KERNEL_CODE_SELECTOR		; cs
push	dword [save_eip]			; eip

pusha						; save registers
push	dword end_yield

push	esp
call	switch_task				; switch to next task

end_yield:
add	esp,4

popa						; restore registers
iret

;
; Switch task
;
; The caller is expected to save the registers onto the stack and call switch_task().
; After returning from switch_task(), the caller function will then restore the registers and return,
; transferring control to the new process.
;
; You are not expected to understand this
;

; void switch_task(size_t *regs);

switch_task:
mov	eax,[esp+4]				; get pointer to saved context
mov	[save_esp],eax

; return if there are no processes

call	get_processes_pointer
test	eax,eax
jnz	have_processes

jmp	no_stack_switch

have_processes:
call	is_multitasking_enabled			
test	eax,eax 				; return if multitasking is disabled
jnz	multitasking_enabled

jmp	no_stack_switch

multitasking_enabled:
inc	byte [0x800b8000]

call	is_current_process_ready_to_switch
test	eax,eax					; return if process is not ready to switch
jnz	task_time_slice_finished

jmp	no_stack_switch

task_time_slice_finished:
push	dword [save_esp]			; save current tasks stack pointer
call	save_kernel_stack_pointer
add	esp,4

call	reset_current_process_ticks

;
; Switch to next process
;

;mov	eax,[save_descriptor]				; get descriptor
;test	eax,eax						; if not switching to a specific process
;jnz	have_descriptor					; update process now

call	getpid

test	eax,eax
jz	no_debug

xchg	bx,bx
no_debug:
call	find_next_process_to_switch_to			; get pointer to next process

;have_descriptor:
push	eax						; update current process pointer
call	update_current_process_pointer
add	esp,4

; load page tables

call	getpid

push	eax
call	loadpagetable
add	esp,4

; Patch ESP0 in the TSS. The scheduler will use the correct kernel stack on the next task switch
call	get_kernel_stack_top

push	eax
call	set_tss_esp0
add	esp,4

call	get_kernel_stack_pointer
mov	esp,eax						; switch kernel stack

no_stack_switch:
;xor	eax,eax
;mov	[save_descriptor],eax				; clear descriptor
ret

save_esp dd 0
save_eax dd 0
save_descriptor dd 0
save_eip dd 0

