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
;xchg	bx,bx
mov	qword [rel irqnumber],0
jmp	irq

irq1:
mov	qword [rel irqnumber],1
jmp	irq

irq2:
mov	qword [rel irqnumber],2
jmp	irq

irq3:
mov	qword [rel irqnumber],3
jmp	irq

irq4:
mov	qword [rel irqnumber],4
jmp	irq

irq5:
mov	qword [rel irqnumber],5
jmp	irq

irq6:
mov	qword [rel irqnumber],6
jmp	irq

irq7:
mov	qword [rel irqnumber],7
jmp	irq

irq8:
mov	qword [rel irqnumber],8
jmp	irq

irq9:
mov	qword [rel irqnumber],9
jmp	irq

irq10:
mov	qword [rel irqnumber],10
jmp	irq

irq11:
mov	qword [rel irqnumber],11
jmp	irq

irq12:
;xchg	bx,bx
mov	qword [rel irqnumber],12
jmp	irq

irq13:
mov	qword [rel irqnumber],13
jmp	irq

irq14:
mov	qword [rel irqnumber],14
jmp	irq

irq15:
mov	qword [rel irqnumber],15
jmp	irq

irq:
push	rax						; save registers
push	rbx
push	rcx
push	rdx
push	rsi
push	rdi
push	r10
push	r11
push	r12
push	r13
push	r14
push	r15

mov	rdx,[rel irqnumber]
shl	rdx,2					; multiply by four

mov	rax,qword irq_handlers			; add start of irq handlers
mov	rax,[rax]				; get irq handler address
add	rdx,rax

test	rdx,rdx					; if no handler, return
jz	irq_exit

call	rdx					; call irq handler

irq_exit:
nop
mov	al,20h				        ; reset master
out	020h,al

mov	ebx,[rel irqnumber]			        ; get interrupt number
cmp	ebx,7			     	        ; if slave irq
jle	nslave				        ; continue if not

mov	al,20h				        ; reset slave			     
out	0a0h,al

nslave:
pop	r15						; restore registers
pop	r14
pop	r13
pop	r12
pop	r11
pop	r10
pop	rdi
pop	rsi
pop	rdx
pop	rcx
pop	rbx
pop	rax
iret						; return

irqnumber dq 0

