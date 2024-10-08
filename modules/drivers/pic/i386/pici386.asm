[BITS 32]
use32

%include "kernelselectors.inc"

extern irq_handlers

global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15
global irq_exit

;
; IRQ handlers
;
irq0:
mov	dword [irqnumber],0
jmp	irq

irq1:
mov	dword [irqnumber],1
jmp	irq

irq2:
mov	dword [irqnumber],2
jmp	irq

irq3:
mov	dword [irqnumber],3
jmp	irq

irq4:
mov	dword [irqnumber],4
jmp	irq

irq5:
mov	dword [irqnumber],5
jmp	irq

irq6:
mov	dword [irqnumber],6
jmp	irq

irq7:
mov	dword [irqnumber],7
jmp	irq

irq8:
mov	dword [irqnumber],8
jmp	irq

irq9:
mov	dword [irqnumber],9
jmp	irq

irq10:
mov	dword [irqnumber],10
jmp	irq

irq11:
mov	dword [irqnumber],11
jmp	irq

irq12:
mov	dword [irqnumber],12
jmp	irq

irq13:
mov	dword [irqnumber],13
jmp	irq

irq14:
mov	dword [irqnumber],14
jmp	irq

irq15:
mov	dword [irqnumber],15
jmp	irq

irq:
pusha						; save registers

mov	dword [esp-4],0				; clear before pushing, pushing segment registers does not modify upper 16 bits of stack entry
push	ds					; save segment registers
mov	dword [esp-4],0
push	es
mov	dword [esp-4],0
push	fs
mov	dword [esp-4],0
push	gs

mov	ax,KERNEL_DATA_SELECTOR			; kernel data selector
mov	ds,ax
mov	es,ax
mov	fs,ax
mov	gs,ax

mov	eax,[irqnumber]
shl	eax,2					; multiply by four
add	eax,irq_handlers			; add start of irq handlers
mov	eax,[eax]				; get irq handler address

test	eax,eax					; if no handler, return
jz	irq_exit

push	esp
call	eax					; call irq handler
add	esp,4

irq_exit:
nop
mov	al,20h				        ; reset master
out	020h,al

mov	ebx,[irqnumber]			        ; get interrupt number
cmp	ebx,7			     	        ; if slave irq
jle	nslave				        ; continue if not

mov	al,20h				        ; reset slave			     
out	0a0h,al

nslave:
pop	gs					; restore segments
pop	fs
pop	es
pop	ds

popa						; restore registers
iret						; return

irqnumber dd 0

