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
; initialize stack
;

%include "kernelselectors.inc"
global initializestack
global initializekernelstack
global create_initial_stack_entries

extern set_tss_esp0
extern irq_exit
extern get_kernel_stack_top
extern get_process_stack_size
extern save_kernel_stack_pointer

section .text
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
; Initialize kernel-mode stack
;
; In:	Stack pointer address
;
; Returns: Nothing
;
initializekernelstack:
mov	esi,[esp+4]				; kernel stack top
mov	edx,[esp+8]				; entry point
mov	ecx,[esp+12]				; kernel stack bottom

mov	edi,esi
sub	edi,17*4				; space for initial stack frame

mov	eax,irq_exit
mov	[edi],eax
mov	dword [edi+4],KERNEL_DATA_SELECTOR	; ds
mov	dword [edi+8],KERNEL_DATA_SELECTOR	; es
mov	dword [edi+12],KERNEL_DATA_SELECTOR	; fs
mov	dword [edi+16],KERNEL_DATA_SELECTOR	; gs
mov	dword [edi+20],0xAAAAAAAA		; edi
mov	dword [edi+24],0xBBBBBBBB		; esi
mov	[edi+28],edx				; ebp
mov	[edi+32],esi				; esp
mov	dword [edi+36],0xCCCCCCCC		; ebx
mov	dword [edi+40],0xDDDDDDDD		; edx
mov	dword [edi+44],0xEEEEEEEE		; ecx
mov	dword [edi+48],0xFFFFFFFF		; eax
mov	dword [edi+52],edx			; eip
mov	dword [edi+56],KERNEL_CODE_SELECTOR	; cs
mov	dword [edi+60],0x200			; eflags
mov	dword [edi+64],ebx			; esp

;
; Adjust kernel stack pointer
; so it points to intial values
;
call	get_kernel_stack_top
sub	eax,17*4

push	eax
call	save_kernel_stack_pointer
add	esp,4

; Set tss esp0 to ensure that the kernel stack is switched to next time the cpu switches to ring 0.

push	edi
call	set_tss_esp0
add	esp,4
ret


section .data
tempeip dd 0

