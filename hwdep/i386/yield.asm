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

extern switch_task
global yield

yield:
cli

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
push	cs	
pushf
push	esp
pusha

call	switch_task				; switch to next task

add	esp,4

popa						; restore new registers
pop	esp
popf
sti
iret						; jump to cs:EIP
