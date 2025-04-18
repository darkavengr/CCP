;
;  CCP Version 0.0.1
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
;

;
; CPU state functions
;
global halt
global restart

section .text
[BITS 64]
use64
;
; Halt CPU
;
; In: Nothing
;
; Returns: Nothing
;
halt:
hlt

;
; Restart CPU
;
; In: Nothing
;
; Returns: Nothing
;
restart:
in	al,64h					; get status

and	al,2					; get keyboard buffer status
jnz	restart

mov	al,0feh					; reset
out 	64h,al
ret

