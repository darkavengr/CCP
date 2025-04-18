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
[BITS 64]
use64

;
; Jump to address and switch to user mode
;
; In:	rdi	Address to jump to
;
; Returns: Does not return
;
; Warning: the address must be accessible from user-mode or it will cause a general protection fault.
;
switch_to_usermode_and_call_process:

mov	ax,USER_DATA_SELECTOR			; load user-mode selector
mov	ds,ax
mov	es,ax
mov	fs,ax
mov	gs,ax

; Create a stack frame to transfer to user-mode

pushfq						; user rflags
pop	rax
or	rax,200h				; enable interrupts
push	rax

push	qword USER_DATA_SELECTOR
push	rsp
push	rax
push	qword USER_CODE_SELECTOR
push	rdi					; user rip
iretq


