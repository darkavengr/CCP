;
;  CCP Version 0.0.1
;    (C) Matthew Boote 2020-2022

;    This file is part of CCP.

;    CCP is free software: you can redistribute it and/or modify
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

;
; Stack functions
;
%include "init.inc"
%include "kernelselectors.inc"

KERNEL_STACK_SIZE equ  65536*2				; size of initial kernel stack
INITIAL_KERNEL_STACK_ADDRESS equ 0x60000		; intial kernel stack address

global initialize_current_process_user_mode_stack
global initialize_current_process_kernel_stack
global create_initial_stack_entries
global get_initial_kernel_stack_base
global get_initial_kernel_stack_top
global get_kernel_stack_size
global get_stack_pointer
global updatekernelstack

extern set_tss_esp0
extern get_process_stack_size
extern save_kernel_stack_pointer
extern get_kernel_stack_top
extern get_usermode_stack_pointer

[BITS 32]
use32

;
; Initialize user-mode stack
;
; In:	Stack pointer address
;	Stack size
;
; Returns: Nothing
;

initialize_current_process_user_mode_stack:
cli
mov	eax,[esp]					; get eip
mov	[tempeip],eax

mov	ebx,[esp+8]					; get stack size
mov	eax,[esp+4]					; get stack pointer

mov	ebx,eax
sub	ebx,[esp+8]					; minus stack size

mov	esp,eax
mov	ebp,ebx

jmp	[tempeip]					; return without using stack

;
; Initialize kernel-mode stack for process
;
; In:	esi=Kernel stack top
;	edx=Process entry point
;	ecx=Kernel stack bottom
;
; Returns: Nothing
;

initialize_current_process_kernel_stack:
mov	esi,[esp+4]				; kernel stack top
mov	edx,[esp+8]				; entry point
mov	ecx,[esp+12]				; kernel stack bottom
mov	ebx,[esp+16]				; user-mode kernel stack top

mov	eax,esi
sub	esi,16*4

mov	dword [esi],USER_DATA_SELECTOR			; user ss
mov	dword [esi-4],ebx				; user esp
mov	dword [esi-8],0x200			; user eflags; enable interrupts on iret
mov	dword [esi-12],USER_CODE_SELECTOR	; user cs
mov	dword [esi-16],edx			; user eip

mov	dword [esi-20],USER_DATA_SELECTOR	; ds
mov	dword [esi-24],USER_DATA_SELECTOR	; es
mov	dword [esi-28],USER_DATA_SELECTOR	; fs
mov	dword [esi-32],USER_DATA_SELECTOR	; gs
mov	dword [esi-36],0xFFFFFFFF		; eax
mov	dword [esi-40],0xEEEEEEEE		; ecx
mov	dword [esi-44],0xDDDDDDDD		; edx
mov	dword [esi-48],0xCCCCCCCC		; ebx
mov	[esi-52],esi				; esp
mov	[esi-56],ecx				; ebp
mov	dword [esi-60],0xBBBBBBBB		; esi
mov	dword [esi-64],0xAAAAAAAA		; edi

;
; Adjust kernel stack pointer to point to intial values
;

sub	esi,16*4

push	esi
call	save_kernel_stack_pointer
add	esp,4

; Set ESP0 field in the TSS to ensure that the kernel stack is switched to next time the CPU switches to ring 0.

push	esi
call	set_tss_esp0
add	esp,4
ret

get_stack_pointer:
mov	eax,esp
ret

get_initial_kernel_stack_base:
mov	eax,INITIAL_KERNEL_STACK_ADDRESS
ret

get_initial_kernel_stack_top:
mov	eax,INITIAL_KERNEL_STACK_ADDRESS
add	eax,KERNEL_STACK_SIZE
ret

tempeip dd 0

