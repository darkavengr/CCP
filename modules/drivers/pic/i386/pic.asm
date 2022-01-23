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

extern getpid

irq0:
inc	dword [tickcount]

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
xchg	bx,bx
mov	dword [irqnumber],8
jmp	irq

irq9:
mov	dword [irqnumber],9
jmp	irq

irq10:
xchg	bx,bx
mov	dword [irqnumber],10
jmp	irq

irq11:
mov	dword [irqnumber],11
jmp	irq

irq12:
;xchg	bx,bx
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

mov	edx,[irqnumber]
mov	eax,4
mul	edx

add	eax,irq_handlers			; add start of irq handlers
mov	eax,[eax]

test	eax,eax
jz	irq_exit

mov	edx,esp
push	edx
call	eax					; call irq handler
add	esp,4

irq_exit:
nop
mov	al,20h				         ; reset master
out	020h,al

mov	ebx,[irqnumber]			           ; get interrupt number
cmp	ebx,7			     	          ; if slave irq
jle	nslave				          ; continue

mov	al,20h				          ; reset slave			     
out	0a0h,al

nslave:
popa					; restore registers

iret			; return

irqnumber dd 0
tickcount dd 0

