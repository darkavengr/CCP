;
; CCP Version 0.0.1
; (C) Matthew Boote 2020-2022

; This file is part of CCP.

; CCP is free software: you can rrdistribute it and/or modify
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

SYSTEM_USE equ 0FFFFFFFFFFFFFFFFh				; page frame marked for system use

section .text
[BITS 64]
use64

;
; Initialize memory map
;
; In:	Initial kernel stack size
;	Initial kernel stack address
;	Kernel size
;	Kernel start address
;	Memory size
;	Memory map address
;
; Returns: Nothing
;
initialize_memory_map:
mov	rax,qword [rsp+8]			; get memory map address

mov	[rel memory_map_address],rax
mov	rax,qword [rsp+16]			; get memory size
mov	[rel memory_size],rax
mov	rax,qword [rsp+24]			; get kernel start address
mov	qword [rel kernel_begin],rax

mov	rax,qword [rsp+32]			; get kernel size
mov	[rel kernel_size],rax

mov	rax,qword [rsp+40]			; get stack address
mov	[rel stack_address],rax

mov	rax,qword [rsp+48]			; get stack size
mov	[rel stack_size],rax

; clear memory map area before filling it

mov	rax,[rel memory_map_address]
mov	rdi,rax

mov	rax,[rel memory_size]
mov	rax,qword 0xffffffffff000
mov	rcx,rax
and	rcx,rdx

shr	rcx,12					; get number of 4096-byte pages
xor	rax,rax
rep	stosq

mov	rax,[rel memory_map_address]
mov	rdi,rax
mov	rcx,(1024*1024)/PAGE_SIZE		; map first 1mb
mov	rax,qword SYSTEM_USE
rep	stosq					; real mode idt and data area

mov	rax,qword [rel kernel_begin]
mov	rdi,rax
mov	rax,qword KERNEL_HIGH
sub	rdi,rax
mov	rax,qword 0xfffffffffffff000
and	rdi,rax
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,2
add	rdi,[rel memory_map_address]

mov	rcx,[rel kernel_size]
add	rcx,PAGE_SIZE

mov	rax,[rel memory_size]
mov	rdx,0xfffffffffffff000
and	rax,rdx

shr	rax,12					; get number of 4096-byte pages
shr	rax,2
add	rcx,rax

shr	rcx,12					; get number of 4096-byte pages
mov	rax,qword SYSTEM_USE
rep	stosq					; page reserved

mov	rdi,[rel stack_address]
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,3					; multiply by 8,each 4096 byte page is represented by 8 byte entries
add	rdi,[rel memory_map_address]

mov	rcx,[rel stack_size]
shr	rcx,12					; get number of 4096-byte pages
mov	rax,qword SYSTEM_USE
rep	stosq

mov	rdi,[rel memory_map_address]
mov	rax,qword KERNEL_HIGH
sub	rdi,rax
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,3					; multiply by 8,each 4096 byte page is represented by 8 byte entries
add	rdi,[rel memory_map_address]

mov	rcx,[rel stack_size]
shr	rcx,12					; get number of 4096-byte pages
mov	rax,qword SYSTEM_USE
rep	stosq

mov	rdi,[rel BOOT_INFO_INITRD_START+KERNEL_HIGH]
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,3					; multiply by 8,each 4096 byte page is represented by 8 byte entries
add	rdi,qword [rel memory_map_address]

mov	rcx,qword [rel BOOT_INFO_INITRD_SIZE+KERNEL_HIGH]
shr	rcx,12					; get number of 4096-byte pages
mov	rax,qword SYSTEM_USE
rep	stosq

mov	rdi,[rel BOOT_INFO_SYMBOL_START+KERNEL_HIGH]
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,3					; multiply by 8,each 4096 byte page is represented by 8 byte entries
add	rdi,[rel memory_map_address]

mov	rcx,[rel BOOT_INFO_SYMBOL_SIZE+KERNEL_HIGH]
shr	rcx,12					; get number of 4096-byte pages
mov	rax,qword SYSTEM_USE
rep	stosq
ret

.data
stack_size dq 0
stack_address dq 0
kernel_size dq 0
kernel_begin dq 0
memory_size dq 0
memory_map_address dq 0

