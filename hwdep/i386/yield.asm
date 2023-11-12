;  CCP Version 0.0.1
;    (C) Matthew Boote 2020

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
; Yield timeslice
;
; [esp+4] pointer to register buffer
;
; returns: nothing
;

extern switch_task
global yield

section .text
[BITS 32]
use32

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
; EIP ( already on stack)
; CS  
; EFLAGS
; ESP 
; EAX	
; ECX
; EDX
; EBX
; Useless ESP
; EBP
; ESI
; EBP

; save eip, cs and eflags in the same way as an interrupt call

push	eax
mov	eax,[esp+4]				; get eip
mov	[saveeip],eax				; save it
pop	eax

sub	esp,4

pushf
push	cs
push	dword [saveeip]

; save 
push	ds

mov	ax,0x10
mov	ds,ax
mov	es,ax
mov	fs,ax
mov	gs,ax

pusha						; save general registers

call	switch_task				; switch to next task
add	esp,4

popa						; restore new registers

;sti
iret						; jump to cs:EIP

section .data
saveeip dd 0

