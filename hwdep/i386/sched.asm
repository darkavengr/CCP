;  CCP Version 0.0.1
;  (C) Matthew Boote 2020-2023
;  This file is part of CCP.
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

X86_NUMBER_OF_REGISTERS equ	11		; 8 registers saved by pusha and 3 by the interrupt

;
; Switch task
;
; Returns nothing
; 
; This is called by a function which is expected to save the registers onto
; the stack, call switch_task and restore the new registers from the stack,
; the calling function will then restore the registers and return, transferring control to the
; new process
;
; You are not expected to understand this
;
global switch_task
global increment_tick_count

extern is_multitasking_enabled			
extern save_kernel_stack_pointer
extern getpid
extern loadpagetable						; load page tables
extern get_kernel_stack_pointer
extern update_current_process_pointer
extern tss_esp0
extern find_next_process_to_switch_to
extern is_process_ready_to_switch
extern reset_process_ticks

switch_task:
mov	eax,esp

push	eax
call	save_kernel_stack_pointer
add	esp,4

;call	increment_tick_count

call	is_multitasking_enabled			
test	eax,eax 				; return if multitasking is disabled
jnz	multitasking_enabled

jmp	end_switch

multitasking_enabled:
call	is_process_ready_to_switch
test	eax,eax
jnz	not_finished

jmp	end_switch

not_finished:
call	reset_process_ticks

call	getpid

cmp	eax,1
jne	not_debug

xchg	bx,bx
not_debug:
call	find_next_process_to_switch_to			; pointer to next process to switch to

push	eax						; switch to process
call	update_current_process_pointer
add	esp,4

call	getpid

push	eax
call	loadpagetable						; load page tables
add	esp,4

; load esp

call	get_kernel_stack_pointer
mov	esp,eax

; Patch ESP0 in TSS. The scheduler will use the correct kernel stack on the next task switch

end_switch:
mov	[tss_esp0],esp
ret

;regsptr dd 0

