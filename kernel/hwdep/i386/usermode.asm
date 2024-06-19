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

global switch_to_usermode_and_call_process

%include "kernelselectors.inc"

section .text
[BITS 32]
use32

;
; Jump to address and switch to user mode
;
; In:	Address to jump to
;
; Return: Does not return
;
; Warning: the address must be accessible from user-mode or it will cause a general protection fault.
;
switch_to_usermode_and_call_process:
mov	ebx,[esp+4]				; get entry point

mov	ax,USER_DATA_SELECTOR			; load user-mode selector
mov	ds,ax
mov	es,ax
mov	fs,ax
mov	gs,ax

; Create a stack frame to transfer to user-mode

mov	eax,esp

push	USER_DATA_SELECTOR			; user ss
push	eax					; user esp

pushf						; user eflags
pop	eax
or	eax,200h				; enable interrupts
push	eax

push	USER_CODE_SELECTOR			; user cs
push	ebx				; user eip
iret

