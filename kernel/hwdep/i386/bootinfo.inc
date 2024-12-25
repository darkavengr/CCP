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
BOOT_INFO_PHYSICAL_DRIVE	equ	0xAC
BOOT_INFO_DRIVE			equ	0xAD
BOOT_INFO_CURSOR_ROW		equ	0xAE
BOOT_INFO_CURSOR_COL		equ	0xAF
BOOT_INFO_KERNEL_START		equ	0xB0
BOOT_INFO_KERNEL_SIZE		equ	0xB4
BOOT_INFO_INITRD_START		equ	0xB8
BOOT_INFO_INITRD_SIZE		equ	0xBC
BOOT_INFO_SYMBOL_START		equ	0xC0
BOOT_INFO_SYMBOL_SIZE		equ	0xC4
BOOT_INFO_NUM_SYMBOLS		equ	0xC8
BOOT_INFO_MEMORY_SIZE		equ	0xCC
BOOT_INFO_BOOT_DRIVE_START_LBA	equ	0xD0

