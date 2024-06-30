global initialize_tss
global set_tss_rsp0
global get_stack_top

extern set_gdt

%define offset
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
mov	rax,[rsp+8]					; get address
mov	r11,rsp0
mov	[r11],rax
ret

;
; Initialize TSS
;
; In: Nothing
;
; Returns: Nothing
;
initialize_tss:
;xchg	bx,bx
xor	rax,rax
push	rax
mov	rax,0xe9
push	rax
mov	rax,qword (offset end_tss-offset tss)+offset tss	; limit
push	rax
mov	rax,qword offset tss				; base
push	rax
mov	rax,qword TSS_GDT_ENTRY				; gdt entry
push	rax
call	set_gdt
add	rsp,40

mov	ax,TSS_SELECTOR + 3				; load tss for interrupt calls from ring 3
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

