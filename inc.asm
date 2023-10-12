section increment progbits alloc exec write align=1
USE32
[BITS 32]

global main

main:
inc	dword [0x800b8000]
jmp	$

