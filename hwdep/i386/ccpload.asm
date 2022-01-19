%define offset
;
;
; CCP intermediate loader
; 
;
NULL equ 0
LOAD_ADDRESS		equ 0x100000				; load address for CCP.SYS
ROOTDIRADDR		equ end_prog+1024			; buffer for root directory
TEMP_ADDRESS		equ end_prog+2048			; temporary address for int 13h
FAT_BUF			equ end_prog+3072

CCPLOAD_STACK		equ 0xfffe				; stack address

KERNEL_HIGH		equ 0x80000000
_ENTRY_SIZE		equ 32					; size of fat entry
BASE_OF_SECTION 	equ 0x1000

_FILENAME 		equ 0
_FILEATTR 		equ 11
_BYTE_RESERVED		equ 12
 _CREATE_TIME_FINE_RES	equ 13 
_CREATE_TIME 		equ 14
 _CREATE_DATE 		equ 16
 _LAST_ACCESS_DATE 	equ 18
 _BLOCK_HIGH_WORD 	equ 20
 _LAST_MODIFIED_TIME 	equ 22
 _LAST_MODIFIED_DATE 	equ 24
 _BLOCK_LOW_WORD	equ 26
 _FILE_SIZE		equ 28

RETRY_COUNT equ 3

 _JUMP 			equ 0												
 _FSMARKER 		equ 3
 _SECTORSIZE		equ 11
 _SECTORSPERBLOCK	equ 13
 _RESERVEDSECTORS	equ 14
 _NUMBEROFFATS		equ 16
 _ROOTDIRENTRIES	equ 17
 _NUMBEROFSECTORS	equ 19
 _MEDIADESCRIPTOR	equ 21
 _SECTORSPERFAT		equ 22
 _SECTORSPERTRACK	equ 24
 _NUMBEROFHEADS		equ 26
 _HIDDENSECTORS		equ 28
 _SECTORSDWORD		equ 32
 _PHYSICALDRIVE		equ 36
 _RESERVED		equ 37
 _EXBOOTSIG		equ 38
 _SERIALNO		equ 39
 _VOLUMELABEL		equ 43
 _FATNAME		equ 54

ELF_MAGIC 	equ 0
ELF_BITS  	equ 4
ELF_ENDIAN 	equ 5
ELF_HEADER_VER	equ 6
ELF_OS_ABI	equ 7
ELF_PADDING5	equ 8
ELF_TYPE	equ 16
ELF_MACHINE	equ 18
ELF_VERSION	equ 20
ELF_ENTRY	equ 24
ELF_PHDR	equ 28
ELF_SHDR	equ 32
ELF_FLAGS	equ 36
ELF_HDRSIZE	equ 40
ELF_PHDRSIZE	equ 42
ELF_PHCOUNT	equ 44
ELF_SHDRSIZE	equ 46
ELF_SHCOUNT	equ 48
ELF_INDEX	equ 50

PHDR_TYPE 	equ 0
PHDR_OFFSET 	equ 4
PHDR_VADDR	equ 8
PHDR_UNDEF 	equ 12
PHDR_SEGSIZE	equ 16
PHDR_VSIZE	equ 20
PHDR_FLAGS	equ 24
PHDR_ALIGN	equ 28

SH_NAME		equ 0
SH_TYPE		equ 4
SH_FLAGS	equ 8
SH_ADDR		equ 12
SH_OFFSET	equ 16
SH_SIZE		equ 20
SH_LINK		equ 24
SH_INFO		equ 28
SH_ADDRALIGN	equ 32
SH_ENTSIZE	equ 36

PROG_LOADABLE	equ 1
PROG_EXECUTABLE equ 2

MACHINE_X86	equ 3
LITTLEENDIAN	equ 1
ELF_32BIT	equ 1

BOOT_INFO_DRIVE		equ	0xAC
BOOT_INFO_CURSOR	equ	0xAD
BOOT_INFO_INITRD_START	equ	0xAE
BOOT_INFO_SYMBOL_START	equ	0xB2
BOOT_INFO_INITRD_SIZE	equ	0xB6

org	BASE_OF_SECTION
cli							; disable interrupts

bits 16
use16
mov	sp,0e000h				; temp stack

mov	ax,cs	
mov	ds,ax
mov	es,ax
inc	ax
mov	ss,ax

;
; enable a20 line
;

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

a20done:
;
; load temporary gdt switch to protected mode, load idt and then switch back to (flat) real mode
;

cli	
xor ax,ax
mov es,ax
mov ss,ax
mov ds,ax

push	ds
push	es
push	ss

mov edi,offset gdtinfo

db 	66h
lgdt 	[ds:di]					; load gdt

mov 	eax,cr0   				; switch to pmode
or	al,1
mov  	cr0,eax

mov   	bx,10h			; load selector
mov   	ds,bx
mov   	es,bx
mov   	ss,bx

mov 	eax,cr0  
and	al,0feh
mov  	cr0,eax

pop	ss
pop	es
pop	ds

mov	ax,[7c00h+_FATNAME+3]				; get fat type
xchg	ah,al						; swap bytes
; sub ax,3120h is the same as:
;  sub ax,3030h
; sub ax,0f0h

sub	ax,3120h					; from ascii to number
mov	[fattype],al					; save fat type

xor	eax,eax
mov	al,[7c00h+_SECTORSPERBLOCK]			; sectors per block
mov	dx,[7c00h+_SECTORSIZE]
mul	dx 
mov	[blocksize],eax

;
; search root directory for ccp.sys
;

; find and load ccp.sys

mov	esi,offset loading_ccp
call	output16

mov	ah,3h					; get cursor
int	10h
mov	[BOOT_INFO_CURSOR],dx

mov	ebx,offset findbuf
mov	edx,offset ccp_name
call	find_file

test	eax,eax					; found file
jnz	ccpsys_not_found

; read files
mov	ebx,offset findbuf
mov	edx,LOAD_ADDRESS

add	edx,[ebx+_FILE_SIZE]			; load after file load address so it can
mov	[loadbuf],edx
mov	[BOOT_INFO_INITRD_START],edx
call	loadfile				; be relocated

mov	esi,edx
mov	eax,[esi+ELF_MAGIC]				; get elf marker
cmp	eax,464c457Fh					; check if file is elf file
je	is_elf
jmp	short bad_elf

is_elf:
mov	ax,[esi+ELF_TYPE]				; check elf type
cmp	ax,PROG_EXECUTABLE
je	is_exec
jmp	bad_elf

is_exec:
mov	ah,[esi+ELF_ENDIAN]				; check endianness
cmp	ah,LITTLEENDIAN
je	is_endian
jmp	bad_elf

is_endian:
mov	ah,[esi+ELF_BITS]				; check bits
cmp	ah,ELF_32BIT
je	is32bit
jmp	bad_elf

is32bit:
mov	ax,[esi+ELF_MACHINE]			; check machine
cmp	ax,MACHINE_X86
je	is_x86
jmp	bad_elf

is_x86:
mov	ax,[esi+ELF_PHCOUNT]			; check program header count
test	eax,eax
jnz	is_phok
jmp	bad_elf

bad_elf:
mov	esi,offset elf_invalid
call	output16

mov	esi,offset pressanykey
call	output16				; display message

call	readkey

int	19h

is_phok:
;
; load program segments
;
mov	esi,[loadbuf]
movsx	ecx,word [esi+ELF_PHCOUNT]			; number of program headers
add	esi,[esi+ELF_PHDR]				; point to program headers

next_programheader:
mov	eax,[esi+PHDR_TYPE]				; get segment type
cmp	eax,PROG_LOADABLE
jne	next_segment

push	esi
push	ecx
mov	edi,[esi+PHDR_VADDR]				; destination
sub	edi,KERNEL_HIGH
mov	ecx,[esi+PHDR_SEGSIZE]
mov	eax,[esi+PHDR_OFFSET]
mov	esi,[loadbuf]
add	esi,eax

a32 rep	movsd						; copy data

pop	ecx
pop	esi
next_segment:
loop	next_programheader
	
;mov	esi,offset loading_initrd
;call	output16

;mov	ebx,offset findbuf
;mov	edx,offset initrd_name
;call	find_file

;test	eax,eax					; found file
;jnz	initrd_file_not_found

;mov	edx,[ebx+_FILE_SIZE]			; memory buf start
;mov	[BOOT_INFO_INITRD_START],edx

; read files
;mov	ebx,offset findbuf
;mov	edx,LOAD_ADDRESS

;add	edx,[ebx+_FILE_SIZE]			; load after file load address so it can
;mov	[loadbuf],edx
;mov	[BOOT_INFO_INITRD_START],edx

;mov	edx,[ebx+_FILE_SIZE]
;mov	[BOOT_INFO_INITRD_SIZE],edx
;call	loadfile

mov	ax,10h
mov	ds,ax
mov	es,ax
mov	ss,ax

db	67h
jmp	0ffffh:10h					; jump to kernel

initrd_file_not_found:
mov	esi,offset initrd_notfound
call	output16				; display message

jmp	short presskey
;
; not found
;
ccpsys_not_found:
mov	esi,offset ccpsys_notfound
call	output16				; display message

presskey:
mov	esi,offset pressanykey
call	output16				; display message

call	readkey
int	19h					; reset

; get next block
;
;
getnextblock:
push	ebx
push	ecx
push	edx

mov	cl,[fattype]			; get fat type
cmp	cl,12h				; fat12
je	getnext_fat12

cmp	cl,16h				; fat16
je	getnext_fat16

cmp	cl,32h				; fat32
je	getnext_fat32

getnext_fat12:
mov	ebx,eax				; entryno=block * (block/2)
shr	eax,1				; divide by two
add	eax,ebx
mov	[entryno],eax
jmp	ok

getnext_fat16:
shl	eax,1				; multiply by two
add	eax,ebx
mov	[entryno],eax
jmp ok

getnext_fat32:
shl	eax,2				; multiply by four
add	eax,ebx
mov	[entryno],eax

ok:
xor	eax,eax				;blockno=(next->reservedsectors*next->sectorsperblock)+(entryno / (next->sectorsperblock*next->sectorsize));		
mov	al,[7c00h+_SECTORSPERBLOCK]	
mul	word [7c00h+_RESERVEDSECTORS]
mov	ecx,eax

;  entry_offset=(entryno % (next->sectorsperblock*next->sectorsize));	/* offset into fat */

xor	edx,edx
mov	eax,[entryno]
div	dword [blocksize]		; entryno/blocksize
add	eax,ecx
mov	[fat_blockno],eax
mov	[entry_offset],edx

mov	ecx,2
mov	ebx,FAT_BUF			; buffer

push	ebx				; save address

next_fatblock:
mov	eax,[fat_blockno]			; read fat sector and fat sector + 1 to buffer
mov	edx,[BOOT_INFO_DRIVE]
call	readblock

inc	eax				; block+1
add	ebx,[blocksize]
loop	next_fatblock


pop	ebx				; restore address

mov	ch,[fattype]			; get fat type
cmp	ch,12h				; fat 16
je	get_block_fat12

cmp	ch,16h				; fat 16
je	get_block_fat16

cmp	ch,32h				; fat 32
je	get_block_fat32

get_block_fat12:
add	ebx,[entry_offset]		; get entry
mov	ax,[bx]

push	ax
mov	ax,[block]
rcr	ax,1				; copy lsb to carry flag
pop	ax
jc	is_odd

and	ax,0fffh					; is even
jmp	 check_end

is_odd:
shr	ax,4
jmp check_end


get_block_fat16:
xor	eax,eax
add	ebx,[entry_offset]		; get entry
mov	ax,[ebx]
jmp check_end

get_block_fat32:
xor	eax,eax
add	ebx,[entry_offset]		; get entry
mov	eax,[ebx]

check_end:
pop	edx
pop	ecx
pop	ebx
ret

;
; ebx -> filename
; edx -> buffer
;
find_file:
push	ebx
push	ecx
push	edx
push	esi
push	edi

mov	[find_buf],ebx
mov	[find_name],edx

mov	bx,[7C00H+_SECTORSPERFAT]			; number of sectors per FAT
xor	eax,eax
mov	al,[7c00h+_NUMBEROFFATS]			; number of fats
mul	bx

add	al,[7c00h+_RESERVEDSECTORS]			; reserved sectors

mov	ebx,eax
xor	ah,ah
mov	al,[7c00h+_SECTORSPERBLOCK]			; sectors per block
mul	ebx
add	eax,[7c00h+_HIDDENSECTORS]			; start of partition

mov	[find_blockno],eax

;
; read sectors from root directory and search
;

read_next_block:
xor	eax,eax
mov	[blockcount],eax

mov	edx,[BOOT_INFO_DRIVE]
mov	eax,[find_blockno]
mov	ebx,ROOTDIRADDR					; read root directory
call	readblock					; read block

mov	esi,ebx						; point to buffer

next_entry:
;
; compare name
;
cld
mov	esi,ebx
mov	edi,[find_name]					; point to name
mov	ecx,11						; size
rep	cmpsb						; compare
je	found_file					; load file

add	ebx,_ENTRY_SIZE
mov	eax,[blockcount]
add	eax,_ENTRY_SIZE					; to next entry
mov	[blockcount],eax

cmp	eax,[blocksize]					; if at end of block
jle	next_entry

mov	eax,[find_blockno]
call	getnextblock					; get next block
mov	[find_blockno],eax

mov	ah,[fattype]
cmp	ah,12h
je	check_fat_12

cmp	ah,16h
je	check_fat_16

cmp	ah,32h
je	check_fat_32

check_fat_12:
mov	eax,0fffh
jl	read_next_block

check_fat_16:
mov	eax,0ffffh
jl	read_next_block	

check_fat_32:
mov	eax,0ffffffffh
jl	read_next_block

jmp	short end_find

found_file:
mov	esi,ebx
mov	edi,[find_buf]
mov	ecx,_ENTRY_SIZE
rep	movsb				; copy entry

xor	eax,eax

end_find:
pop	edi
pop	esi
pop	edx
pop	ecx
pop	ebx
ret

;
; load file
;
; edx -> buffer
; ebx -> file information returned by find
;

loadfile:
push	ebx
push	ecx
push	edx
push	esi
push	edi

mov	[loadptr],edx				; save address

mov	al,[fattype]	
cmp	al,12h	
je	fat_12_16_getblock

cmp	al,16h	
je	fat_12_16_getblock

cmp	al,32h	
je	fat_32_getblock

fat_12_16_getblock:
xor	edx,edx

mov	dx,[ebx+26]
mov	[block],dx
jmp	short foundblock

fat_32_getblock:
xor	edx,edx
mov	dx,[ebx+20]
bswap	edx
mov	dx,[ebx+16]

mov	[block],edx

foundblock:

next_block:
mov	ebx,[block]
movsx	ecx,word [7c00h+_RESERVEDSECTORS]			; reserved sectors
add	ebx,ecx

xor	eax,eax

xor	ah,ah
mov	cx,[7C00H+_SECTORSPERFAT]			; number of sectors per FAT
mov	al,[7c00h+_NUMBEROFFATS]			; number of fats
mul	ecx
add	ebx,eax

mov	eax,_ENTRY_SIZE
mul	word [7C00H+_ROOTDIRENTRIES]			; number of root directory entries

xor	edx,edx
xor	ecx,ecx
mov	cx,[7c00h+_SECTORSIZE]
div	ecx

add	ebx,eax
sub	ebx,2	
mov	eax,ebx						; block number

mov	edx,[BOOT_INFO_DRIVE]
mov	ebx,[loadptr]					; buffer
call	readblock					; read block

mov	ebx,[loadptr]					; buffer
add	ebx,[blocksize]					; point to next
mov	[loadptr],ebx

mov	ax,0e2eh
int	10h

;
; get next block
;

mov	eax,[block]					; start block
call	getnextblock					; get next block
mov	[block],eax

;
; check if at end
;
mov	cl,[fattype]					; get fat type
cmp	cl,16h
je	check_fat16_end

cmp	cl,32h
je	check_fat32_end

; fat 12 otherwise
cmp	ax,0ff8h					; if at end
jle	next_block					; loop if not

check_fat16_end:
cmp	ax,0fff8h					; if at end
jle	next_block					; loop if not

check_fat32_end:
cmp	eax,0fffffff8h					; if at end
jle	next_block					; loop if not

xor	eax,eax				; loaded ok
jmp	short end_load

end_load:
pop	edi
pop	esi
pop	edx
pop	ecx
pop	ebx
ret

;
; read block
;
; eax=block
; ebx=buffer

readblock:
push	ebx					; save registers
push	ecx
push	edx

mov	[save_esp],esp				; save esp
mov	[dest_addr],ebx
mov	[read_block],eax

cmp	edx,80h
jl	no_extensions

mov	ah,41h					; check for int 13h extensions
mov	bx,55aah
int	13h
jc	no_extensions

mov	esi,offset dap				; set up disk address packet
mov	al,10h
mov	[esi],al				; size of dap
xor	al,al
mov	[esi+1],al				; reserved
mov	al,[7c00h+_SECTORSPERBLOCK]	
mov	[esi+3],al				; number of sectors to read
mov	ax,ds					; segment:offset address to read to
mov	[esi+4],ax
mov	ax,offset TEMP_ADDRESS
mov	[esi+6],ax
mov	eax,[read_block]
mov	[esi+4],eax				; block number

xor	eax,eax						; clear counter
mov	dword [retry_counter],eax

next_retry_ext:
mov	ah,42h					; extended read
mov	dl,[BOOT_INFO_DRIVE]
int	13h

inc	dword [retry_counter]
cmp	dword [retry_counter],RETRY_COUNT
jne	next_retry_ext					; loop

mov	eax,0fffffffh
jmp	end_read

no_extensions:
;
; convert block number to cylinder,head,sector
;

;;xchg	bx,bx
xor	ecx,ecx

push	ebx
mov	eax,[read_block]
xor 	edx,edx		
xor	ebx,ebx
mov 	bx,[7c00h+_SECTORSPERTRACK]			; sectors per track
div	ebx

inc	edx				; sectors start fron 1
mov	[sector],dl

mov	cl,dl						; save sector
inc	cl

xor 	dx,dx
mov 	bx,[7c00h+_NUMBEROFHEADS]			; number of heads
div 	bx

mov	[cyl],ax
mov	[head],dl

xor	eax,eax						; clear counter
mov	dword [retry_counter],eax

;
; read file until end of fat chain
;
pop	ebx
mov	edi,ebx						; save buffer address

next_retry:

; read block to buffer somewhere below 640k because int 13h can't read > 1Mb
;

xor	ebx,ebx

mov	ah,2h						; read sectors
mov	al,[7c00h+_SECTORSPERBLOCK]			; block
mov	bx,TEMP_ADDRESS
mov	ch,[cyl]
mov	cl,[sector]
mov	dh,[head]
mov	dl,[BOOT_INFO_DRIVE]
int	13h

jnc	copy_data

bad_read:						; error reading file

inc	dword [retry_counter]
cmp	dword [retry_counter],RETRY_COUNT
jne	next_retry					; loop

mov	eax,0fffffffh
jmp	short end_read

copy_data:
mov	esi,TEMP_ADDRESS
mov	edi,[dest_addr]
mov	ecx,[blocksize]

db	67h
rep	movsd

end_read:
mov	esp,[save_esp]			; restore esp

pop	edx						; restore registers
pop	ecx
pop	ebx
ret

;
; display message
;
output16:
push	eax
push	esi

mov	[save_esp],esp				; save esp

next_output:
xor	eax,eax

mov	ah,0eh						; output character
mov	al,[esi]
int	10h

inc	esi

test	al,al
jnz	next_output

mov	esp,[save_esp]			; restore esp

pop	esi
pop	eax
ret

readkey:
pusha
xor	ax,ax
int	16h					; get key
popa
ret

ccpsys_notfound db "CCP.SYS not found",0
initrd_notfound db 10,13,"INITRD.SYS not found",0
noa20_error db "Can't enable A20 line",0
loading_ccp db "Loading CCP.SYS...",0
loading_initrd db 10,13,"Loading INITRD.SYS...",0
elf_invalid db 10,13,"CCP.SYS is not a valid ELF executable",10,13,0
pressanykey db " Press any key to reboot",10,13,0
ccp_name db    "CCP     SYS",0
initrd_name db "INITRD  SYS",0
crlf db 10,13,0
symbol_section_name db ".strtab",0
entrycount dd 0
fattype dw 0
blockno dd 0
blocksize dd 0
block dd 0
entryno dd 0
entry_offset dd 0
retry_counter dd 0
find_blockno dd 0
fat_blockno dd 0
loadbuf dd 0
read_block dd 0
loadptr dd 0
blockcount dd 0
save_esp dd 0
real_ds dd 0
dest_addr dd 0
find_name dd 0
find_buf dd 0
head db 0
cyl dw 0
sector db 0

findbuf TIMES 32 db 0
dap	TIMES 16 db 0


gdtinfo:
dw gdt_end - gdt
dd gdt

; null entries, don't modify
gdt dw 0
dw 0
db 0
db 0,0
db 0

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

end_prog:
