;
; CCP Version 0.0.1
; (C) Matthew Boote 2020-2022

; This file is part of CCP.

; CCP is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.

; CCP is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.

; You should have received a copy of the GNU General Public License
; along with CCP.  If not, see <https://www.gnu.org/licenses/>.

;
; Initialize memory map
;

global initialize_memory_map

%include "init.inc"
%include "bootinfo.inc"

SYSTEM_USE equ 0FFFFFFFFh				; page marked for system use

initialize_memory_map:
mov	eax,[esp+4]			; get memory map address
mov	[memory_map_address],eax

mov	eax,[esp+8]			; get memory size
mov	[memory_size],eax

mov	eax,[esp+12]			; get kernel start address
mov	[kernel_begin],eax

mov	eax,[esp+16]			; get kernel size
mov	[kernel_size],eax

mov	eax,[esp+20]			; get stack address
mov	[stack_address],eax

mov	eax,[esp+24]			; get stack size
mov	[stack_size],eax

; clear memory map area before filling it

mov	edi,[memory_map_address]

mov	ecx,[memory_size]
and	ecx,0fffff000h
shr	ecx,12					; get number of 4096-byte pages
xor	eax,eax
rep	stosd

mov	edi,[memory_map_address]
mov	ecx,(1024*1024)/PAGE_SIZE		; map first 1mb
mov	eax,SYSTEM_USE
rep	stosd					; real mode idt and data area

mov	edi,[kernel_begin]
sub	edi,KERNEL_HIGH
and	edi,0fffff000h
shr	edi,12					; get number of 4096-byte pages
shl	edi,2
add	edi,[memory_map_address]

mov	ecx,[kernel_size]
add	ecx,PAGE_SIZE

mov	eax,[memory_size]
and	eax,0fffff000h
shr	eax,12					; get number of 4096-byte pages
shr	eax,2
add	ecx,eax

shr	ecx,12					; get number of 4096-byte pages
mov	eax,SYSTEM_USE
rep	stosd					; page reserved

mov	edi,[stack_address]
shr	edi,12					; get number of 4096-byte pages
shl	edi,2					; multiply by 4,each 4096 byte page is represented by 4 byte entries
add	edi,[memory_map_address]

mov	ecx,[stack_size]
shr	ecx,12					; get number of 4096-byte pages
mov	eax,SYSTEM_USE
rep	stosd

mov	edi,[memory_map_address]
sub	edi,KERNEL_HIGH
shr	edi,12					; get number of 4096-byte pages
shl	edi,2					; multiply by 4,each 4096 byte page is represented by 4 byte entries
add	edi,[memory_map_address]

mov	ecx,[stack_size]
shr	ecx,12					; get number of 4096-byte pages
mov	eax,SYSTEM_USE
rep	stosd

mov	edi,[BOOT_INFO_INITRD_START+KERNEL_HIGH]
shr	edi,12					; get number of 4096-byte pages
shl	edi,2					; multiply by 4,each 4096 byte page is represented by 4 byte entries
add	edi,[memory_map_address]

mov	ecx,[BOOT_INFO_INITRD_SIZE+KERNEL_HIGH]
shr	ecx,12					; get number of 4096-byte pages
mov	eax,SYSTEM_USE
rep	stosd

mov	edi,[BOOT_INFO_SYMBOL_START+KERNEL_HIGH]
shr	edi,12					; get number of 4096-byte pages
shl	edi,2					; multiply by 4,each 4096 byte page is represented by 4 byte entries
add	edi,[memory_map_address]

mov	ecx,[BOOT_INFO_SYMBOL_SIZE+KERNEL_HIGH]
shr	ecx,12					; get number of 4096-byte pages
mov	eax,SYSTEM_USE
rep	stosd

ret

stack_size dd 0
stack_address dd 0
kernel_size dd 0
kernel_begin dd 0
memory_size dd 0
memory_map_address dd 0

