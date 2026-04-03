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
; EIP      
; CS
; EFLAGS
; DS
; ES
; GS
; FS
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
push	ds					; save segment selector registers
push	es
push	gs
push	fs
pusha						; save registers
mov	[OldContextPointer],esp

call	find_next_process_to_switch_to
push	eax
push	dword [OldContextPointer]
call	switch_task				; switch to next task

; shouldn't be here

;
; Switch task
;
; In:	[esp+4]		Pointer to current process context
;	[esp+8]		New process descriptor
;
; Returns: Doesn't return
;
switch_task:
mov	eax,[esp+4]
mov	[OldContextPointer],eax
mov	eax,[esp+8]
mov	[NextProcessPointer],eax

inc	byte [0x800b8000]

call	getpid
test	eax,eax
jnz	skiptest

call	GetCurrentProcessFlags			; get process flags
mov	[ProcessFlags],eax

and	eax,PROCESS_NEW				; get new process flag
test	eax,eax					; if new process, skip saving kernel stack pointer
jnz	not_first

skiptest:
push	dword [OldContextPointer]
call	save_kernel_stack_pointer		; save kernel stack pointer for current process
add	esp,4

not_first:
mov	eax,[ProcessFlags]
and	eax,~PROCESS_NEW			; clear new process flag

push	eax
call	SetCurrentProcessFlags
add	esp,4

;
; Switch to next process
;

push	dword [NextProcessPointer]		; update current process pointer
call	update_current_process_pointer
add	esp,4

;call	getpid
;test	eax,eax
;jz	no_debug

;xchg	bx,bx
;no_debug:

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

; switch kernel stack
call	get_kernel_stack_pointer

;xchg	bx,bx
mov	esp,eax

; Return directly to process. Does *not* return to the caller
popa						; restore registers
pop	gs
pop	fs
pop	es
pop	ds
iret

OldContextPointer dd 0
NextProcessPointer dd 0
save_eip dd 0
ProcessFlags dd 0

