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
global get_stack_pointer

extern set_tss_esp0
extern irq_exit
extern get_kernel_stack_top
extern get_process_stack_size

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

mov	ebx,esi
sub	ebx,12*4				; space for initial stack frame

; Create intial stack frame

mov	eax,irq_exit
mov	[ebx],eax
mov	dword [ebx+4],0xFFFFFFFF		; eax
mov	dword [ebx+8],0xEEEEEEEE		; ecx
mov	dword [ebx+12],0xDDDDDDDD		; edx
mov	dword [ebx+16],0xCCCCCCCC		; ebx
mov	dword [ebx+20],esi			; esp
mov	dword [ebx+24],ecx			; ebp
mov	dword [ebx+28],0xBBBBBBBB		; esi
mov	dword [ebx+32],0xAAAAAAAA		; edi
mov	dword [ebx+36],edx			; eip
mov	dword [ebx+40],KERNEL_CODE_SELECTOR	; cs
mov	dword [ebx+44],0x200			; eflags

; Set tss esp0 to ensure that the kernel stack is switched to next time the cpu switches to ring 0.

push	esi
call	set_tss_esp0
add	esp,4

ret

;
; Get stack pointer
;
; In:	Nothing
;
; Returns: Stack pointer
;
get_stack_pointer:
mov	eax,esp
ret

section .data
tempeip dd 0

