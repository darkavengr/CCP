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

global switch_to_next_process
global end_switch
global end_yield
global yield
global switch_task_process_descriptor
global IsDebug

extern is_multitasking_enabled			
extern save_kernel_stack_pointer
extern getpid
extern switch_address_space
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
extern GetCurrentProcessFlags
extern SetCurrentProcessFlags

PROCESS_NEW	equ 2
PROCESS_RUNNING	equ 8

%include "kernelselectors.inc"
;
; The functions switch_task_process_descriptor() and yield() should not be called from 
; an interrupt handler. Call switch_task() instead.
;

switch_task_process_descriptor:
push	eax

mov	eax,[esp+4]				; get process descriptor
mov	[save_descriptor],eax
pop	eax

; fall through to switch_to_next_process()

;
; Switch task
;
switch_to_next_process:
mov	eax,[esp+4]
mov	[OldContextPointer],eax			; save esp

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
call	reset_current_process_ticks		; reset number of process quantum ticks

call	getpid
test	eax,eax
jnz	skiptest

call	GetCurrentProcessFlags			; get process flags

and	eax,PROCESS_NEW				; get new process flag
test	eax,eax					; if new process, skip saving kernel stack pointer
jnz	not_first

skiptest:
push	dword [OldContextPointer]
call	save_kernel_stack_pointer		; save kernel stack pointer for current process
add	esp,4

not_first:
call	GetCurrentProcessFlags			; get process flags

and	eax,~PROCESS_NEW			; clear new process flag
or	eax,PROCESS_RUNNING			; set process running flag

push	eax
call	SetCurrentProcessFlags
add	esp,4

;
; Switch to next process
;

;mov	eax,[save_descriptor]				; get pointer to process entry
;test	eax,eax						; if not switching to a specific process
;jnz	have_descriptor					; update process now

call	find_next_process_to_switch_to			; get pointer to next process

;have_descriptor:
push	eax						; update current process pointer
call	update_current_process_pointer
add	esp,4

call	getpid
test	eax,eax
jz	no_debug

xchg	bx,bx
no_debug:

; switch address space

call	getpid

push	eax
call	switch_address_space
add	esp,4

; Patch ESP0 in the TSS. The scheduler will use the correct kernel stack on the next task switch
call	get_kernel_stack_top

push	eax
call	set_tss_esp0
add	esp,4

call	get_kernel_stack_pointer
; switch kernel stack

mov	esp,eax

;xor	eax,eax
;mov	[save_descriptor],eax				; clear descriptor
 
;
; Return directly to process. Does *not* return to the caller
popa						; restore registers
pop	gs
pop	fs
pop	es
pop	ds
iret

;
; No stack switch. Return to caller
;
no_stack_switch:
ret

OldContextPointer dd 0
save_descriptor dd 0
IsDebug dd 0

