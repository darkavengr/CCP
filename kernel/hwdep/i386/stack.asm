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

global initializestack
global initializekernelstack
global create_initial_stack_entries
global get_initial_kernel_stack_base
global get_initial_kernel_stack_top
global get_kernel_stack_size
global get_stack_pointer
global updatekernelstack

extern set_tss_esp0
extern irq_exit
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
initializestack:
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
initializekernelstack:
mov	esi,[esp+4]				; kernel stack top
mov	edx,[esp+8]				; entry point
mov	ecx,[esp+12]				; kernel stack bottom

mov	dword [esi],0x200			; eflags
mov	dword [esi-4],KERNEL_CODE_SELECTOR	; cs
mov	[esi-8],edx				; eip
mov	dword [esi-12],0xFFFFFFFF		; eax
mov	dword [esi-16],0xEEEEEEEE		; ecx
mov	dword [esi-20],0xDDDDDDDD		; edx
mov	dword [esi-24],0xCCCCCCCC		; ebx
mov	[esi-28],esi				; esp
mov	[esi-32],ecx				; ebp
mov	dword [esi-36],0xBBBBBBBB		; esi
mov	dword [esi-40],0xAAAAAAAA		; edi
mov	dword [esi-44],irq_exit			; return address

;
; Adjust kernel stack pointer to point to intial values
;

sub	esi,11*4

push	esi
call	save_kernel_stack_pointer
add	esp,4

; Set tss esp0 to ensure that the kernel stack is switched to next time the CPU switches to ring 0.

push	esi
call	set_tss_esp0
add	esp,4
ret

get_kernel_stack_size:
mov	eax,KERNEL_STACK_SIZE
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

