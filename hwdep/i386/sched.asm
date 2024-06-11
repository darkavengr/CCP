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

extern is_multitasking_enabled			
extern save_kernel_stack_pointer
extern getpid
extern loadpagetable
extern get_kernel_stack_pointer
extern update_current_process_pointer
extern set_tss_esp0
extern find_next_process_to_switch_to
extern is_process_ready_to_switch
extern reset_process_ticks
extern increment_tick_count
extern get_kernel_stack_top
extern increment_process_ticks
extern get_next_process_pointer
extern get_current_process_pointer
extern get_processes_pointer

%define offset

%include "kernelselectors.inc"
;
; The functions switch_task_process_descriptor and yield should not be called from 
; an interrupt handler. Instead, switch_task must be called.
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
; EIP ( already on stack)
; CS  
; EFLAGS
; ESP 
; EAX	
; ECX
; EDX
; EBX
; Useless ESP
; EBP
; ESI
; EDI
; cs
; ds
; es
; fs
; gs

; save eip, cs and eflags in the same way as an interrupt call

push	eax
mov	eax,[esp+4]				; get eip
mov	[saveeip],eax				; save it
pop	eax

sub	esp,4

pushf
push	cs
push	dword [saveeip]

pusha						; save registers

push	ds					; save segment registers
push	es
push	fs
push	gs

mov	ax,KERNEL_DATA_SELECTOR			; kernel data selector
mov	ds,ax
mov	es,ax
mov	fs,ax
mov	gs,ax

call	switch_task				; switch to next task

pop	gs					; restore segments
pop	fs
pop	es
pop	ds

popa						; restore registers
iret						; jump to cs:EIP

;
; Switch task
;
; This function is called by a function which is expected to save the registers onto the stack and call switch_task.
; After returning from switch_task, the calling function will then restore the registers and return, transferring control to the new process.
;
; You are not expected to understand this
;

switch_task:
;xchg	bx,bx

mov	[save_esp],esp

;inc	byte [0x800b8000]

call	is_multitasking_enabled			
test	eax,eax 				; return if multitasking is disabled
jnz	multitasking_enabled

jmp	end_switch

multitasking_enabled:
push	dword [save_esp]			; save current task's stack pointer
call	save_kernel_stack_pointer
add	esp,4

call	increment_process_ticks

mov	eax,[save_descriptor]			; get descriptor
test	eax,eax					; if not switching to a specific process
jnz	task_time_slice_finished		; don't check if process is ready to switch

call	is_process_ready_to_switch
test	eax,eax					; if process not ready to switch, return
jnz	task_time_slice_finished

jmp	end_switch

task_time_slice_finished:
call	reset_process_ticks

call	getpid

cmp	eax,1
jne	not_debug

xchg	bx,bx

not_debug:
;
; Switch to next process
;

mov	eax,[save_descriptor]				; get descriptor
test	eax,eax						; if not switching to a specific process
jnz	have_descriptor					; update process now

call	find_next_process_to_switch_to			; get pointer to next process to switch to

have_descriptor:
push	eax						; update current process pointer
call	update_current_process_pointer
add	esp,4

; load page tables

call	getpid

push	eax
call	loadpagetable
add	esp,4

call	get_kernel_stack_pointer
mov	esp,eax						; switch kernel stack

sub	esp,4

; Patch ESP0 in the TSS. The scheduler will use the correct kernel stack on the next task switch
call	get_kernel_stack_top

push	eax
call	set_tss_esp0
add	esp,4

end_switch:
xor	eax,eax
mov	[save_descriptor],eax				; clear descriptor
ret

save_esp dd 0
save_eax dd 0
save_descriptor dd 0
saveeip dd 0

