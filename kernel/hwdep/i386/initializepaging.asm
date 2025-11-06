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
;
;  CCP is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with CCP.  If not, see <https://www.gnu.org/licenses/>.
;

global initializepaging
global unmap_lower_half

extern paging_type
extern processpaging
extern processpaging_end

%include "init.inc"
%include "bootinfo.inc"

ROOT_PDPT		equ	0x2000
ROOT_PAGEDIR		equ	0x1000
ROOT_PAGETABLE		equ	0x3000
ROOT_EMPTY		equ	0x4000

PAGE_PRESENT		equ	1
PAGE_RW			equ	2

PAGE_SIZE		equ	4096

[BITS 32]
use32
;
; Intialize paging
;
; Maps memory to higher and lower halves of memory
;
; In: Nothing
;
; Returns: Nothing
;
initializepaging:
; subtract KERNEL_HIGH from addresses

mov	eax,[esp+4]			; get kernel size
mov	esi,kernel_size
sub	esi,KERNEL_HIGH
mov	[esi],eax

mov	eax,[esp+8]			; get initrd size
mov	esi,initrd_size
sub	esi,KERNEL_HIGH
mov	[esi],eax

mov	eax,[esp+12]			; get symbol table size
mov	esi,symtab_size
sub	esi,KERNEL_HIGH
mov	[esi],eax

mov	eax,[esp+16]			; get memory size
mov	esi,memory_size
sub	esi,KERNEL_HIGH
mov	[esi],eax

; set end of page table list

mov	esi,processpaging
sub	esi,KERNEL_HIGH

mov	eax,[esi]
mov	esi,processpaging_end
sub	esi,KERNEL_HIGH

mov	[esi],eax

lea	esi,[paging_type-KERNEL_HIGH]
mov	eax,[esi]

cmp	eax,1
je	legacy_paging

cmp	eax,2
je	pae_paging

jmp	legacy_paging

;
; enable pae paging
;
pae_paging:
mov	edi,ROOT_PDPT
xor	eax,eax
mov	ecx,4096
rep	stosd

mov	edi,ROOT_PDPT			; create pdpt
mov	eax,ROOT_PAGEDIR | PAGE_PRESENT
xor	edx,edx
mov	[edi],eax
mov	[edi+4],edx
mov	[edi+16],eax			; map higher half
mov	[edi+16+4],edx

mov	eax,ROOT_PDPT		; set pdptphys and processid
mov	[edi+36],eax

xor	eax,eax
mov	[edi+44],eax

mov	edi,ROOT_PAGEDIR
xor	eax,eax
mov	ecx,1024
rep	stosd

mov	edi,ROOT_PAGEDIR
mov	eax,ROOT_PAGETABLE | PAGE_RW | PAGE_PRESENT
xor	edx,edx
mov	[edi],eax
mov	[edi+4],edx
mov	cr3,edi

; create page table

mov	ecx,(1024*1024)*4			; map first 4mb
shr	ecx,12					; number of page table entries

mov	edi,ROOT_PAGETABLE
xor	eax,eax
rep	stosd

mov	ecx,(1024*1024)*4			; map first 4mb
shr	ecx,12					; number of page table entries

mov	edi,ROOT_PAGETABLE
mov	eax,0+PAGE_PRESENT+PAGE_RW	; page+flags

map_next_page:
or	eax,PAGE_PRESENT
mov	[edi],eax
mov	dword [edi+4],0

add	eax,1000h
add	edi,8

sub	ecx,1
sbb	edx,0

mov	ebx,ecx
add	ebx,edx
test	ebx,ebx
jnz	map_next_page

mov	eax,cr4				; enable pae paging
bts	eax,5
mov	cr4,eax

mov	eax,ROOT_PDPT			; create pdpt
mov	cr3,eax

mov	eax,cr0
or	eax,80000000h			; enable paging
mov	cr0,eax
ret

;
; legacy paging
;
legacy_paging:
mov	edi,ROOT_PAGEDIR
xor	eax,eax
mov	ecx,1024*2
rep	stosd

; create page table

; find number of page tables to create

mov	ecx,(1024*1024)				; map first 1mb

mov	esi,kernel_size
sub	esi,KERNEL_HIGH
add	ecx,[esi]

mov	esi,initrd_size
sub	esi,KERNEL_HIGH
add	ecx,[esi]

mov	esi,symtab_size
sub	esi,KERNEL_HIGH
add	ecx,[esi]

mov	esi,memory_size			; memory map size
sub	esi,KERNEL_HIGH

mov	eax,[esi]
;shr	eax,12					; number of pages
;shl	eax,2					; number of 4-byte entries
add	ecx,eax

and	ecx,0fffff000h

add	ecx,PAGE_SIZE

shr	ecx,12					; number of page table entries
mov	edi,ROOT_PAGETABLE
mov	eax,0+PAGE_PRESENT+PAGE_RW	; page+flags

cld
map_next_page_legacy:
stosd
add	eax,PAGE_SIZE			; point to next page
loop	map_next_page_legacy

mov	edi,ROOT_PAGEDIR
mov	eax,ROOT_PAGETABLE+PAGE_RW+PAGE_PRESENT
mov	[edi],eax			; map lower half
mov	[edi+(512*4)],eax			; map higher half

; store physical address of the page directory

mov	[edi+PAGE_SIZE],edi		; pagedirphys

mov	edi,ROOT_PAGEDIR+(1023*4)	; map last entry to page directory - page tables will be present at 0xfc000000 - 0xfffff000
mov	eax,ROOT_PAGEDIR+PAGE_RW+PAGE_PRESENT
mov	[edi],eax

mov	eax,ROOT_PAGEDIR		; load cr3
mov	cr3,eax

mov	eax,cr0
or	eax,80000000h			; enable paging
mov	cr0,eax

over:
ret

;
; Unmap lower half
;
unmap_lower_half:
mov	eax,[paging_type]

cmp	eax,1
je	unmap_pd

mov	edi,ROOT_EMPTY
xor	eax,eax
mov	ecx,1024
rep	stosd

mov	edi,ROOT_PDPT
xor	eax,eax
mov	dword [edi],ROOT_EMPTY
mov	[edi+4],eax

mov	edi,ROOT_PDPT+(3*8)	; map last entry to pdpt[0] - page tables will be present at 0xc0000000 - 0xfffff000
mov	edx,ROOT_PDPT | PAGE_PRESENT
mov	[edi],edx	
mov	[edi+4],eax	

mov	edx,ROOT_PDPT
mov	cr3,edx

jmp	short endunmap	

unmap_pd:
mov	edi,ROOT_PAGEDIR
xor	eax,eax

mov	[edi],eax
endunmap:
ret

kernel_size dd 0
initrd_size dd 0
symtab_size dd 0
memory_size dd 0

