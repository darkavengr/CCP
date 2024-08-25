global initialize_tss
global set_tss_rsp0
global get_stack_top

extern gdt

%include "init.inc"
%include "kernelselectors.inc"

TSS_GDT_ENTRY equ 5

section .text
[BITS 64]
use64

;
; Set TSS ring 0 ESP value
;
; In: New ring 0 ESP value
;
; Returns: Nothing
;

set_tss_rsp0:
mov	r11,rsp0
mov	[r11],rdi
ret

;
; Initialize TSS
;
; In: Nothing
;
; Returns: Nothing
;
initialize_tss:
; don't use set_gdt(), TSSs have a different format
; and TSS GDT entries take up two GDT entries

mov	rdi,qword gdt

mov	rax,qword TSS_GDT_ENTRY
shl	rax,3
add	rdi,rax

mov	rax,qword (end_tss-tss)+tss
mov	[rdi],ax			; put bits 0-15 of limit

shr	rax,16				
and	al,0fh				; get bottom 4 bits

mov	[rdi+4],al			; put bits 16-23 of limit

mov	rax,qword tss
mov	[rdi+2],ax			; put bits 0-15 of base

shr	rax,16
mov	[rdi+4],al			; put bits 16-23 of base

shr	rax,8
mov	[rdi+7],al			; put bits 24-31 of base

shr	rax,8
mov	[rdi+8],al			; put bits 32-63 of base

xor	eax,eax
mov	[rdi+12],eax			; put reserved bits

mov	al,(1 << 7) | (3 << 5) | 9	; access byte
mov	[rdi+5],al

mov	ax,TSS_SELECTOR + 3		; load tss for interrupt calls from ring 3
ltr	ax
ret

section .data
tss:
reserved db 0
iopb db 0
rsp0 dq 0
rsp1 dq 0
rsp2 dq 0
reserved2 dd 0
reserved3 dd 0
ist1 dq 0
ist2 dq 0
ist3 dq 0
ist4 dq 0
ist5 dq 0
ist6 dq 0
ist7 dq 0
end_tss:

