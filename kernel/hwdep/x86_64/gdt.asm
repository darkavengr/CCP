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
[BITS 64]
use64

;
; Set GDT entry
;
; In: 
; [rsp+8]	GDT entry number
; [rsp+16]	Base address
; [rsp+24]	Limit
; [rsp+32]	Access bits
; [rsp+40]	Granularity bits
;
; Returns: -1 on error or 0 on success
;

set_gdt:
mov	rdi,[rsp+4]					; get gdt entry number
mov	edx,[rsp+8]					; get base
mov	rcx,[rsp+12]					; get limit
mov	rbx,[rsp+16]					; get access
mov	rax,[rsp+20]					; get granularity

shl	rdi,4						; multiply by size of gdt entry (16)
mov	r11,qword gdt
add	rdi,r11						; point to gdt entry

mov	r11,qword gdt_end
cmp	rdi,r11						; check if past end of GDT
jl	gdt_is_ok

mov	rax,0xffffffffffffffff				; return error
ret

gdt_is_ok:
; put base
mov	rsi,rdx
and	rdx,0xffff
mov	[rel rdi+GDT_BASE_LOW],dx

mov	rdx,rsi
shr	rdx,16
and	rdx,0xff
mov	[rel rdi+GDT_BASE_MIDDLE],dl

mov	rdx,rsi
shr	rdx,24
and	rdx,0xff
mov	[rel rdi+GDT_BASE_HIGH],dl

; put limit
mov	rsi,rcx
and	rcx,0xffff
mov	[rel rdi+GDT_LIMIT_LOW],cx

; put granularity
mov	rcx,rsi
shr	rcx,16
and	rcx,0x0f
and	rax,0xf0
or	rcx,rax
mov	[rel rdi+GDT_GRANULARITY],cl

mov	[rel rdi+GDT_ACCESS],bl		; put access

xor	rax,rax			; return success
ret

