global set_gdt

extern gdt
extern gdt_end
extern gdtinfo_high_64

%define offset

GDT_LIMIT_LOW	equ 0
GDT_BASE_LOW	equ 2
GDT_BASE_MIDDLE equ 4
GDT_ACCESS	equ 5
GDT_FLAGS	equ 6
GDT_BASE_HIGH	equ 7

section .text
[BITS 64]
use64

;
; Set GDT entry
;
; In: 
; RDI	GDT entry number
; RSI	Base address
; RDX	Limit
; RCX	Access bits
; R8	Flags
;
; Returns: -1 on error or 0 on success
;

set_gdt:
xchg	bx,bx

shl	rdi,3						; multiply by size of gdt entry (8)

mov	r11,qword gdt
add	rdi,r11						; point to gdt entry

mov	r11,qword gdt_end
cmp	rdi,r11						; check if past end of GDT
jl	gdt_is_ok

mov	rax,0xffffffffffffffff				; return error
ret

gdt_is_ok:
; put base

xchg	bx,bx
mov	rax,rsi
and	rax,0xffff
mov	[rdi+GDT_BASE_LOW],ax

mov	rax,rsi
shr	rax,16
and	rax,0xff
mov	[rdi+GDT_BASE_MIDDLE],al

mov	rax,rsi
shr	rax,24
and	rax,0xff
mov	[rdi+GDT_BASE_HIGH],al

; put limit
mov	rax,rdx
and	rax,0xffff
mov	[rdi+GDT_LIMIT_LOW],ax


; put flags

mov	rax,rdx				; get limit
shr	rax,16				; get bits 16-19
and	rax,0x0f			; move them to lower nibble

or	rax,r8				; copy flags+limit
mov	[rdi+GDT_FLAGS],al
mov	[rdi+GDT_ACCESS],cl		; put access byte

xor	rax,rax			; return success
ret

