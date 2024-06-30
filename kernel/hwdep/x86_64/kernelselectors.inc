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

KERNEL_CODE_SELECTOR	equ	8h			; kernel code selector
KERNEL_DATA_SELECTOR	equ	10h			; kernel data selector
USER_CODE_SELECTOR	equ	1Bh			; user code selector
USER_DATA_SELECTOR	equ	23h			; user data selector
TSS_SELECTOR 		equ	28h			; tss selector

