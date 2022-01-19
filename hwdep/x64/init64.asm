;
; Hardware dependent code for i386 processors
;

%define TRUE 1
%define FALSE 0
%define offset

extern kernel							; high-level kernel
extern dispatchhandler						; high-level dispatcher
extern exception						; exception handler
extern kernel_begin						; start of kernel in memory
extern openfiles_init						; intialize structs
extern regbuf							; buffer to save registers to
extern end							; end of kernel in memory
extern page_init_first_time					; initialize paging for first time
extern row							; current row
extern col							; current column
extern exit
extern switch_task
extern kprintf
extern init_multitasking
extern driver_init
extern dma_alloc_init
extern readconsole
extern outputconsole
extern currentprocess

;
; globals
;
global tss_esp0
global getstackpointer					; get stack pointer
global switchtonextprocess				;switch to next process
global _asm_init
global _loadregisters					; load registers
global halt						; halt
global loadregisters					; load registers from buffer
global saveregisters					; save registers to buffer
global restart						; restart computer
global disable_interrupts				; disable interrupts
global enable_interrupts				; enable interrupts
global switchtousermode_and_call_process		; switch to usermode and call process
global MEM_SIZE						; size of memory
global initializestack					; intialize stack
global tickcount					; tick counter
global set_interrupt					; set interrupt
global get_interrupt					; get interrupt
global yield
global idttable

PAGE_SIZE equ 4096					; size of page  DON'T CHANGE	
SYSTEM_USE equ 0FFFFFFFFh				; page marked for system used
KERNEL_STACK_SIZE equ  65536				; size of initial kernel stack
BASE_ADDR_LOW		equ 0
BASE_ADDR_HIGH		equ 4
LENGTH_LOW		equ 8
LENGTH_HIGH		equ 12
BLOCK_TYPE		equ 16

INITIAL_KERNEL_STACK_ADDRESS equ 0x80000
BASE_OF_SECTION equ 0x100000

DMA_BUFFER_SIZE equ 32768				; size of floppy buffer

KERNEL_CODE_SELECTOR	equ	8h			; kernel code selector
KERNEL_DATA_SELECTOR	equ	10h			; kernel data selector
USER_CODE_SELECTOR	equ	1Bh			; user code selector
USER_DATA_SELECTOR	equ	23h			; user data selector
TSS_SELECTOR 		equ	28h			; tss selector

_asm_init:
;
; 16-bit initialisation code
;
; here the computer is still in real mode,
; the kernel will enable the a20 line, detect memory
; load gdt and idt and jump to protected mode
;
			

[BITS 16]
use16
cli
mov	sp,0e000h				; temporary stack

xor	ax,ax
mov	ds,ax
mov	es,ax
mov	ss,ax

;
; enable a20 line
;
call    a20wait					; wait for a20 line
mov     al,0xAD					; disable keyboard
out     0x64,al
 
call    a20wait					; wait for a20 line

mov     al,0xD0
out     0x64,al
 
call    a20wait2
in      al,0x60
push    eax
 
call    a20wait
mov     al,0xD1
out     0x64,al
 
call    a20wait
pop     eax
or      al,2
out     0x60,al
 
call    a20wait
mov     al,0xAE
out     0x64,al
 
call    a20wait
sti
jmp 	short a20done
 
a20wait:
in      al,0x64
test    al,2
jnz     a20wait
ret
 
a20wait2:
in      al,0x64
test    al,1
jz      a20wait2
ret					

;
; detect memory
; this must be done in real mode
;
; using these functions:
;

; INT 0x15, AX = 0xE820 *
; INT 0x15, AX = 0xE801 *
; INT 0x15, AX = 0xDA88 *
; INT 0x15, AH = 0x88   *
; INT 0x15, AH = 0x8A   *
;
a20done:
xor	ebx,ebx

xor	ax,ax				; segment:offset address for buffer
mov	es,ax
mov	di,8000h				; segment:offset address for buffer

xor	ecx,ecx					; clear memory count
xor	edx,edx					; clear memory count

push	ecx
push	edx

not_end_e820:
mov	eax,0e820h				; get memory block
mov	edx,534d4150h
mov	ecx,20h					; size of memory block
int	15h
jc	no_e820					; 0e820 not supported

pop	edx					; get count
pop	ecx					; get count


add	ecx,[es:di+LENGTH_LOW]			; add size
adc	edx,[es:di+LENGTH_HIGH]			; add size
push	ecx
push	edx

add	edi,20h

test	ebx,ebx					; if at end
jnz	not_end_e820				; loop if not
jmp	end_detect_memory

no_e820:
mov	ax,0e801h				; get memory info
int	15h
jc	no_e801					; e801 not supported

cmp	ah,86h					; not supported
je	no_e801

cmp	ah,80h					; not supported
je	no_e801

test	eax,eax					; use eax,ebx
jnz	use_ebx_eax

add	ecx,1024				; plus conventional memory
shl	ecx,16					; multiply by 65536 because edx is in 64k blocks
shl	edx,10					; multiply by 1024 because eax is in kilobytes
mov	ecx,edx
jmp	end_detect_memory

use_ebx_eax:
add	eax,1024				; plus conventional memory
shl	ebx,16					; multiply by 65536 because edx is in 64k blocks
shl	eax,10					; multiply by 1024 because eax is in kilobytes
add	eax,ebx					; add memory count below 16M
mov	ecx,eax
jmp	end_detect_memory

no_e801:
mov	ah,88h					; get extended memory count
int	15h
jc	no_88h					; not supported

test	ax,ax					; error
jz	no_88h					; not supported

cmp	ah,86h					; not supported
je	no_88h

cmp	ah,80h					; not supported
je	no_88h

add	eax,1024				; plus conventional memory
shl	eax,10					; multiply by 1024, size is in kilobytes
mov	ecx,eax
jmp	end_detect_memory

no_88h:
xor	ecx,ecx
xor	ebx,ebx

mov	ax,0da88h				; get extended memory size
int	15h
jc	no_da88h				; not supported

; returned in cl:bx
shr	ecx,16					; shift and add
add	ecx,ebx
pop	edx
add	ecx,1024					; add conventional memory size
jmp	end_detect_memory

no_da88h:
mov	ah,8ah					; get extended memory size
int	15h
jc	no_8ah
push	ax					; returned in dx:ax
push	dx
pop	ecx

add	ecx,1024*1024				; add conventional memory size
add	ecx,eax
jmp	end_detect_memory

no_8ah:
hlt						; halt

end_detect_memory:
;
; from here to pmode: don't change ecx or edx
;
xor	ax,ax
mov	ds,ax
mov	es,ax
mov	ss,ax

cli	
mov 	edi,offset 32bgdtinfo
db	66h
lgdt 	[ds:edi]					; load gdt

mov 	eax,cr0   				; switch to pmode
or	al,1
mov  	cr0,eax

jmp	 dword 0x8:pmode			; flush instruction cache

[BITS 32]
use32

pmode:
cli
;****************************************************
; 32 bit protected mode
;****************************************************

mov	ax,KERNEL_DATA_SELECTOR			; selector
mov	ds,ax					; put selectors
mov	es,ax					
mov	fs,ax					
mov	gs,ax					
mov	ss,ax					

mov	[MEM_SIZE_HIGH],edx				; save memory size
mov	[MEM_SIZE],ecx				; save memory size

mov	esp,INITIAL_KERNEL_STACK_ADDRESS+KERNEL_STACK_SIZE	; temporary stack
mov	ebp,INITIAL_KERNEL_STACK_ADDRESS

lidt	[idt]					; load interrupts
mov	dx,[0xae]
mov	[row],dh
mov	[col],dl

; switch to long mode enable PAE paging, set long mode bit in efer

; check long mode support

mov	eax,80000000h
cpuid
cmp	eax,80000001h		; not extended support
jne	not_long_mode

mov	eax,80000001h		; check for long mode
cpuid

and	edx,10000000h
test	edx,edx
jnz	long_mode_supported

;
; long mode is not supported, display a message and halt
;
not_long_mode:
push	0 
push	offset no_long_mode
call 	kprintf

cli
hlt
jmp 	$
	
long_mode_supported:
; paging needs to be enabled with long mode

call	page_init_first_time			; paging

push	0					; load cr3
call	loadpagetable
add	esp,4

mov	eax,cr4					; set pae paging bit
and	eax,10h
mov	cr4,eax

rmsr						; enable long mode
or	eax,512
wmsr

mov	eax,cr0					; set pmode and paging
and	eax,800000001h
mov	cr0,eax

; here the cpu is in compatibility mode, a 64bit gdt and idt is loaded to
; switch to long mode

[USE64]
bits 64

lgdt	[gdt]					; load 64 bit gdt

jmp	8:longmode

longmode:
xor	rax,rax
mov	[currentprocess],rax
mov	[processes],rax
mov	[blockdevices],rax
mov	[characterdevs],rax

push	0 
push	offset startingccp				; display banner
call 	kprintf

mov	rsp,INITIAL_KERNEL_STACK_ADDRESS+KERNEL_STACK_SIZE	; temporary stack
mov	rbp,INITIAL_KERNEL_STACK_ADDRESS


32bgdtinfo:
dw offset gdt_end - offset gdt-1
dd offset gdt

; null entries, don't modify
gdt dw 0
dw 0
db 0
db 0,0
db 0

; intial 32 bit gdt

dw 0ffffh					; low word of limit
dw 0						; low word of base
db 0						; middle byte of base
db 09ah,0cfh					; Code segment
db 0						; last byte of base

dw 0ffffh					; low word of limit
dw 0		 				; low word of base
db 0	 					; middle byte of base
db 92h,0cfh					; Data segment
db 0						; last byte of base

gdt_end:
db 0

32bidt:
dw 0x3FFF					; limit
dd offset 32bidttable				; base

32bidttable:
times 256 dw (BASE_OF_SECTION + null_32bhandler - $$) & 0xFFFF,8h,dw 8eh,(BASE_OF_SECTION +  null_32b_handler - $$)>> 16

gdtinfo:
dd offset gdt
dw offset gdt_end-offset gdt

gdt:
dw 0xFFFF             	      ; low word of limit
dw 0                         ; low word of base
db 0                         ; middle byte of base
db 0                  
db 1
db 0                         ; high byte of base

; ring 0 code segment

dw 0                         ; low word of limit
dw 0                         ; low word of base
db 0                         ; middle byte of base
db 9ah               
db 0afh
db 0        		; high byte of base

; ring 0 data segment
dw 0                        
dw 0                 
db 0
db 92h              
db 0afh
db 0

; ring 3 code segment

dw 0                         ; low word of limit
dw 0                         ; low word of base
db 0                         ; middle byte of base
db 9ah               
db 0afh
db 0        		; high byte of base

; ring 3 data segment
dw 0                        
dw 0                 
db 0
db 92h              
db 0afh
db 0


idt:
dw offset end_idt-offset idttable		; limit
dq offset idttable				; base

idttable:
dw (BASE_OF_SECTION + int0_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int0_handler - $$)>> 16

dw (BASE_OF_SECTION + int1_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int1_handler - $$)>> 16

dw (BASE_OF_SECTION + int2_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int2_handler - $$)>> 16

dw (BASE_OF_SECTION + int3_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int3_handler - $$)>> 16

dw (BASE_OF_SECTION + int4_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int4_handler - $$)>> 16

dw (BASE_OF_SECTION + int5_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int5_handler - $$)>> 16

dw (BASE_OF_SECTION + int6_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int6_handler - $$)>> 16

dw (BASE_OF_SECTION + int7_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int7_handler - $$)>> 16

dw (BASE_OF_SECTION + int8_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int8_handler - $$)>> 16

dw (BASE_OF_SECTION + int9_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int9_handler - $$)>> 16

dw (BASE_OF_SECTION + int10_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int10_handler - $$)>> 16

dw (BASE_OF_SECTION + int11_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int11_handler - $$)>> 16

dw (BASE_OF_SECTION + int12_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int12_handler - $$)>> 16

dw (BASE_OF_SECTION + int13_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int13_handler - $$)>> 16

dw (BASE_OF_SECTION + int14_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int14_handler - $$)>> 16

dw (BASE_OF_SECTION + int15_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int15_handler - $$)>> 16

dw (BASE_OF_SECTION + int16_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int16_handler - $$)>> 16

dw (BASE_OF_SECTION + int17_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int17_handler - $$)>> 16

dw (BASE_OF_SECTION + int18_handler - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  int18_handler - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		;19
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16	

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 20
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 21
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt- $$) & 0xFFFF		; 22
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 23
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 24
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 25
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 26
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt- $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 27
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 28
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt- $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 29
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 30
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF		; 31
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + end_process - $$) & 0xFFFF
dw 8h
db 0h
db 0eeh
dw (BASE_OF_SECTION + end_process  - $$)>> 16

dw (BASE_OF_SECTION + d_lowlevel - $$) & 0xFFFF
dw 8h
db 0h
db 0eeh
dw (BASE_OF_SECTION + d_lowlevel  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16




dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION +  null_interrupt - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16


dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16


dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16


dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16


dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16


dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16


dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16


dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16


dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

dw (BASE_OF_SECTION + null_interrupt - $$) & 0xFFFF
dw 8h
db 0h
db 8eh
dw (BASE_OF_SECTION + null_interrupt  - $$)>> 16

tempone dq 0
temptwo dq 0
MEMBUF_START dq offset end
MEM_SIZE dd 0
MEM_SIZE_HIGH dd 0

banner db "Starting CCP...",13,0
long_mode_not_supported db "CCP requires a x64 CPU to run, system halted.",13,0

