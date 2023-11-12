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
extern increment_tick_count

%define offset

section .text
[BITS 32]
use32

;
; Switch task
;
; In: Address of stack frame
;
; Returns: Never returns
;
; This function is called by a function which is expected to save the registers onto the stack and call switch_task.
; After returning from switch_task, the calling function will then restore the registers and return, transferring control to the new process.
;
; You are not expected to understand this
;
switch_task:
;xchg	bx,bx
mov	[save_esp],esp

call	increment_tick_count

call	is_multitasking_enabled			
test	eax,eax 				; return if multitasking is disabled
jnz	multitasking_enabled

jmp	end_switch

multitasking_enabled:
call	increment_process_ticks

call	is_process_ready_to_switch

test	eax,eax					; if process not ready to switch, return
jnz	task_time_slice_finished

jmp	end_switch

task_time_slice_finished:
mov	eax,[save_esp]

push	eax					; save stack pointer for current process
call	save_kernel_stack_pointer
add	esp,4

call	reset_process_ticks			; reset number of ticks for current process

call	getpid

cmp	eax,1
jne	not_debug

xchg	bx,bx

not_debug:
;
; Switch to next process
;

call	find_next_process_to_switch_to			; get pointer to next process to switch to

push	eax						; update current process pointer
call	update_current_process_pointer
add	esp,4

; switch address space

call	getpid

push	eax
call	loadpagetable					; load page tables
add	esp,4

; Patch ESP0 in the TSS. The scheduler will use the correct kernel stack on the next task switch

call	get_kernel_stack_top

push	eax
call	set_tss_esp0
add	esp,4

call	get_kernel_stack_pointer			; switch stack
mov	esp,eax

end_switch:
ret

section .data
save_esp dd 0

