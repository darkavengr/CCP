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

global initializestack
global initializekernelstack
extern set_tss_esp0

initializestack:
cli
mov	eax,[esp]					; get eip
mov	[tempeip],eax

mov	ebx,[esp+4]					; get esp
mov	eax,ebx
;sub	eax,[esp+8]					; minus stack size

mov	esp,ebx
mov	ebp,eax

jmp	[tempeip]					; return without using stack

initializekernelstack:
mov	eax,[esp+4]					; get stack pointer value

push	eax
call	set_tss_esp0
add	esp,4

ret
tempeip dd 0
