global set_gdt

extern gdt
extern gdt_end

%define offset

GDT_LIMIT_LOW	equ 0
GDT_BASE_LOW	equ 2
GDT_BASE_MIDDLE equ 4
GDT_ACCESS	equ 5
GDT_GRANULARITY equ 6
GDT_BASE_HIGH	equ 7

section .text
[BITS 32]
use32

;
; Set GDT entry
;
; In: 
; [esp+4]	GDT entry number
; [esp+8]	Base address
; [esp+12]	Limit
; [esp+16]	Access bits
; [esp+20]	Granularity bits
;
; Returns: -1 on error or 0 on success
;

set_gdt:
mov	edi,[esp+4]					; get gdt entry number
mov	edx,[esp+8]					; get base
mov	ecx,[esp+12]					; get limit
mov	ebx,[esp+16]					; get access
mov	eax,[esp+20]					; get granularity

shl	edi,3						; multiply by size of gdt entry (8)
add	edi,offset gdt					; point to gdt entry

cmp	edi,offset gdt_end				; check if past end of GDT
jl	gdt_is_ok

mov	eax,0xffffffff					; return error
ret

gdt_is_ok:
; put base
mov	esi,edx
and	edx,0xffff
mov	[edi+GDT_BASE_LOW],dx

mov	edx,esi
shr	edx,16
and	dx,0xff
mov	[edi+GDT_BASE_MIDDLE],dl

mov	edx,esi
shr	edx,24
and	dx,0xff
mov	[edi+GDT_BASE_HIGH],dl

; put limit
mov	esi,ecx
and	ecx,0xffff
mov	[edi+GDT_LIMIT_LOW],cx

; put granularity
mov	ecx,esi
shr	ecx,16
and	ecx,0x0f
and	eax,0xf0
or	ecx,eax
mov	[edi+GDT_GRANULARITY],cl

mov	[edi+GDT_ACCESS],bl		; put access

xor	eax,eax			; return success
ret

