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

extern kernel_begin
extern end

%include "init.inc"
%include "bootinfo.inc"

SYSTEM_USE equ 0FFFFFFFFFFFFFFFFh				; page frame marked for system use

section .text
[BITS 64]
use64

;
; Initialize memory map
;
; In:	
;	rdi=Memory map address
;	rsi=Initial kernel stack address
;	rdx=Initial kernel stack size
;	rcx=Memory size
;
; Returns: Nothing
;
initialize_memory_map:
mov	r11,qword memory_map_address
mov	[r11],rdi

mov	r11,qword stack_address
mov	[r11],rsi

mov	r11,qword stack_size
mov	[r11],rdx

mov	r11,qword memory_size
mov	[r11],rcx

; clear memory map area before filling it

shr	rcx,12					; get number of 4096-byte pages
xor	rax,rax
rep	stosq

; map first 1MB

mov	r11,memory_map_address
mov	rdi,[r11]
mov	rcx,(1024*1024)/PAGE_SIZE		; map first 1mb

mov	rax,qword SYSTEM_USE
rep	stosq					; real mode idt and data area

; map kernel

mov	rdi,kernel_begin
mov	rax,qword KERNEL_HIGH
sub	rdi,rax					

mov	rax,qword 0xfffffffffffff000
and	rdi,rax					; round down to multiple of 4096
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,3					; nunber of 8-byte entries
mov	r11,memory_map_address
add	rdi,[r11]				

mov	rcx,end
mov	rax,kernel_begin
sub	rcx,rax					; get kernel size
shr	rcx,12					; get number of 4096-byte pages

rep	stosq

; map memory map

mov	r11,memory_map_address			; get address of memory map
mov	rdx,[r11]

mov	rax,qword 0xfffffffffffff000
and	rdx,rax					; round down to multiple of 4096

mov	rax,KERNEL_HIGH
sub	rdx,rax

shr	rdx,12					; get number of 4096-byte pages
shr	rdx,12					; number of bytes

mov	rdi,[r11]
add	rdi,rdx

mov	r11,memory_size				; get memory size
xor	rcx,rcx

mov	ecx,[r11]
shr	rcx,12					; get number of 4096-byte pages
shr	rcx,3

mov	rax,qword SYSTEM_USE
rep	stosq					; page reserved

; map initial kernel stack

mov	r11,stack_address			; get stack address
mov	rdi,[r11]
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,3					; multiply by 8,each 4096 byte page is represented by 8 byte entries

mov	r11,memory_map_address			; add memory map address
mov	rax,[r11]
add	rdi,rax

mov	r11,stack_size				; get stack size
xor	rcx,rcx

mov	ecx,[r11]
shr	ecx,12					; get number of 4096-byte pages

mov	rax,qword SYSTEM_USE
rep	stosq

; map initrd

mov	r11,BOOT_INFO_INITRD_START+KERNEL_HIGH
mov	rdi,[r11]
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,3					; multiply by 8,each 4096 byte page is represented by 8 byte entries
mov	r11,memory_map_address
add	rdi,[r11]				

mov	r11,BOOT_INFO_INITRD_SIZE+KERNEL_HIGH

xor	rcx,rcx
mov	ecx,[r11]
shr	ecx,12					; get number of 4096-byte pages

mov	rax,qword SYSTEM_USE
rep	stosq

; map kernel symbols

mov	r11,BOOT_INFO_SYMBOL_START+KERNEL_HIGH
movsx	rdi,dword [r11]
shr	rdi,12					; get number of 4096-byte pages
shl	rdi,3					; multiply by 8,each 4096 byte page is represented by 8 byte entries
mov	r11,memory_map_address			; add memory map address
mov	rax,[r11]
add	rdi,rax

mov	r11,BOOT_INFO_SYMBOL_SIZE+KERNEL_HIGH	; get symbol data size
xor	rcx,rcx
mov	ecx,[r11]
shr	ecx,12					; get number of 4096-byte pages

mov	rax,qword SYSTEM_USE
rep	stosq

ret

.data
stack_size dq 0
stack_address dq 0
kernel_size dq 0
memory_size dq 0
memory_map_address dq 0

