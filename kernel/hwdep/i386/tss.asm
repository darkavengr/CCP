global initialize_tss
global set_tss_esp0
global get_stack_top

extern set_gdt

%include "init.inc"
%include "kernelselectors.inc"
%include "gdtflags.inc"

TSS_GDT_ENTRY equ 5
TSS_SELECTOR 		equ	TSS_GDT_ENTRY * GDT_ENTRY_SIZE			; TSS selector

[BITS 32]
use32

;
; Set TSS ring 0 ESP value
;
; In: New ring 0 ESP value
;
; Returns: Nothing
;

set_tss_esp0:
mov	eax,[esp+4]					; get address
mov	[tss_esp0],eax
ret

;
; Get top of kernel mode stack
;
; In: Nothing
;
; Returns: top of kernel mode stack
;
get_tss_esp0:
mov	eax,[tss_esp0]
ret

;
; Initialize TSS
;
; In: Nothing
;
; Returns: Nothing
;
initialize_tss:
push	dword 0						; granularity							; access
push	SEGMENT_PRESENT | RING3_SEGMENT | TSS_SEGMENT | SEGMENT_32BIT_TSS_AVALIABLE
push	(end_tss-tss)+tss		; limit
push	tss					; base
push	dword TSS_GDT_ENTRY				; gdt entry
call	set_gdt
add	esp,20

mov	eax,KERNEL_DATA_SELECTOR			; kernel selector
mov	[reg_es],eax
mov	[reg_ss],eax
mov	[reg_ds],eax
mov	[reg_fs],eax
mov	[reg_gs],eax
mov	[reg_cs],eax
mov	[tss_ss0],eax

mov	ax,TSS_SELECTOR + 3				; load TSS for interrupt calls from ring 3
ltr	ax
ret

section .data
tss:
prev dd 0
tss_esp0 dd 0
tss_ss0 dd 0
tss_esp1 dd 0
tss_ss1 dd 0 
tss_esp2 dd 0
tss_ss2 dd 0 
tss_cr3 dd 0
tss_eip dd 0
tss_eflags dd 0
reg_eax dd 0
reg_ebx dd 0
reg_ecx dd 0
reg_edx dd 0
reg_esp dd 0
reg_ebp dd 0
reg_esi dd 0
reg_edi dd 0
reg_es dd 0 
reg_cs dd 0 
reg_ss dd 0 
reg_ds dd 0 
reg_fs dd 0 
reg_gs dd 0 
reg_ldt dd 0
tss_trap dw 0					
tss_iomap_base dw 0
end_tss:
