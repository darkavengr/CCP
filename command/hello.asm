%define offset

section hello_world exec write align=1

global _start

USE32
[BITS 32]
_start:
mov	ah,9h
mov	edx,offset hello_message
int	21h

mov	ax,4c00h
int	21h

hello_message db "Hello, World!",10,13,0

