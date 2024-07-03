%define offset
;
;
; CCP intermediate loader
; 
;
%include "../kernel/hwdep/i386/bootinfo.inc"

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

; 32-bit table indexes

ELF32_MAGIC 	equ 0
ELF32_BITS  	equ 4
ELF32_ENDIAN 	equ 5
ELF32_HEADER_VER	equ 6
ELF32_OS_ABI	equ 7
ELF32_PADDING5	equ 8
ELF32_TYPE	equ 16
ELF32_MACHINE	equ 18
ELF32_VERSION	equ 20
ELF32_ENTRY	equ 24
ELF32_PHDR	equ 28
ELF32_SHDR	equ 32
ELF32_FLAGS	equ 36
ELF32_HDRSIZE	equ 40
ELF32_PHDRSIZE	equ 42
ELF32_PHCOUNT	equ 44
ELF32_SHDRSIZE	equ 46
ELF32_SHCOUNT	equ 48
ELF32_INDEX	equ 50

PHDR32_TYPE 	equ 0
PHDR32_OFFSET 	equ 4
PHDR32_VADDR	equ 8
PHDR32_UNDEF 	equ 12
PHDR32_SEGSIZE	equ 16
PHDR32_VSIZE	equ 20
PHDR32_FLAGS	equ 24
PHDR32_ALIGN	equ 28

SH32_NAME	equ 0
SH32_TYPE	equ 4
SH32_FLAGS	equ 8
SH32_ADDR	equ 12
SH32_OFFSET	equ 16
SH32_SIZE	equ 20
SH32_LINK	equ 24
SH32_INFO	equ 28
SH32_ADDRALIGN	equ 32
SH32_ENTSIZE	equ 36

SYM32_NAME	equ 0
SYM32_VALUE	equ 0x4
SYM32_SIZE	equ 0x8
SYM32_INFO	equ 0xC
SYM32_VISIBILITY equ 0xD
SYM32_SECTION	equ 0xE

SYM32_ENTRY_SIZE	equ 0x10

; 64-bit indexes

ELF64_MAGIC 	equ 0
ELF64_BITS  	equ 4
ELF64_ENDIAN 	equ 5
ELF64_HEADER_VER	equ 6
ELF64_OS_ABI	equ 7
ELF64_PADDING5	equ 8
ELF64_TYPE	equ 16
ELF64_MACHINE	equ 18
ELF64_VERSION	equ 20
ELF64_ENTRY	equ 24
ELF64_PHDR	equ 32
ELF64_SHDR	equ 40
ELF64_FLAGS	equ 48
ELF64_HDRSIZE	equ 52
ELF64_PHDRSIZE	equ 54
ELF64_PHCOUNT	equ 56
ELF64_SHDRSIZE	equ 58
ELF64_SHCOUNT	equ 60
ELF64_SINDEX	equ 62

PHDR64_TYPE 	equ 0
PHDR64_FLAGS	equ 4
PHDR64_OFFSET 	equ 8
PHDR64_VADDR	equ 16
PHDR64_PADDR 	equ 24
PHDR64_FILESZ	equ 32
PHDR64_MEMSZ	equ 40
PHDR64_ALIGN	equ 48

SH64_NAME	equ 0
SH64_TYPE	equ 4
SH64_FLAGS	equ 8
SH64_ADDR	equ 16
SH64_OFFSET	equ 24
SH64_SIZE	equ 32
SH64_LINK	equ 40
SH64_INFO	equ 44
SH64_ADDRALIGN	equ 48
SH64_ENTSIZE	equ 56

SYM64_NAME	equ 0
SYM64_INFO	equ 4
SYM64_OTHER	equ 5
SYM64_SECTION	equ 6	
SYM64_VALUE	equ 14
SYM64_SIZE	equ 24

PROG_LOADABLE	equ 1
PROG_EXECUTABLE equ 2

MACHINE_X86	equ 3
LITTLEENDIAN	equ 1
ELF32_BIT	equ 1

SHT_SYMTAB	equ 2
SHT_STRTAB	equ 3

STT_NOTYPE	equ 0
STT_SECTION	equ 3

INT15_E820_BASE_ADDR_LOW	equ 0
INT15_E820_BASE_ADDR_HIGH	equ 4
INT15_E820_LENGTH_LOW		equ 8
INT15_E820_LENGTH_HIGH		equ 12
INT15_E820_BLOCK_TYPE		equ 16

INT15_BUFFER			equ	0x9000

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
xor	ax,ax
mov	es,ax
mov	ss,ax
mov	ds,ax

push	ds
push	es
push	ss

mov	edi,offset gdtinfo

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

mov	al,[BOOT_INFO_PHYSICAL_DRIVE]			; get physical drive

test	al,80h						; is hard drive
jz	not_boot_hd

sub	al,80h						; get logical drive from physical drive
add	al,2						; logical drives start from drive 2

not_boot_hd:
mov	[BOOT_INFO_DRIVE],al

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

;**************************************
; Load kernel
;**************************************

mov	esi,offset loading_ccp
call	output16

mov	ebx,offset findbuf
mov	edx,offset ccp_name
call	find_file

test	eax,eax					; found file
jnz	ccpsys_not_found

; read files
mov	ebx,offset findbuf
; save kernel load address

mov	edx,LOAD_ADDRESS
mov	[BOOT_INFO_KERNEL_START],edx

; save kernel size

mov	ecx,[ebx+_FILE_SIZE]
mov	[BOOT_INFO_KERNEL_SIZE],ecx

; save current end address

mov	eax,edx					; address
add	eax,[ebx+_FILE_SIZE]			; +size
mov	[currentptr],eax

add	edx,(1024*1024)			; load after file load address so it can be relocated
mov	[loadbuf],edx
call	loadfile				

mov	esi,edx
mov	eax,[esi+ELF32_MAGIC]				; get elf marker
cmp	eax,464c457Fh					; check if file is elf file
je	is_elf
jmp	short bad_elf

is_elf:
mov	ax,[esi+ELF32_TYPE]				; check elf type
cmp	ax,PROG_EXECUTABLE
je	is_exec
jmp	bad_elf

is_exec:
mov	ah,[esi+ELF32_ENDIAN]				; check endianness
cmp	ah,LITTLEENDIAN
je	is_endian
jmp	bad_elf

is_endian:
mov	ax,[esi+ELF32_PHCOUNT]			; check program header count
test	eax,eax
jnz	is_phok
jmp	bad_elf

bad_elf:
mov	esi,offset ELF32_invalid
call	output16

mov	esi,offset pressanykey
call	output16				; display message

call	readkey

int	19h

is_phok:
mov	ah,[esi+ELF32_BITS]			; get 32 or 64-bit
cmp	ah,ELF32_BIT
je	load_elf32

jmp	load_elf64

;
; Copy kernel symbols before copying kernel, copying segments overwrites temporary buffer
;

; find size of program segments to calculate destination address

load_elf32:
mov	esi,[loadbuf]
movsx	edx,word [esi+ELF64_SHCOUNT]			; number of program headers
mov	[sectioncount],edx
xor	ecx,ecx

mov	esi,[loadbuf]
add	esi,[esi+ELF32_PHDR]				; point to program headers
xor	edx,edx						; clear counter

next_size:
mov	eax,[esi+PHDR32_TYPE]				; get segment type
cmp	eax,PROG_LOADABLE
jne	next_segment_size

add	edx,[esi+PHDR32_VSIZE]

next_segment_size:
loop	next_size

mov	eax,LOAD_ADDRESS				; calculate destination address
add	eax,edx

mov	[symbols_start],eax

; Copy symbol names and addresses

mov	esi,[loadbuf]
movsx	ecx,word [esi+ELF32_SHCOUNT]			; number of section headers

add	esi,[esi+ELF32_SHDR]				; point to section headers

next_find_sections:
mov	eax,[esi+SH32_TYPE]				; get section type
cmp	eax,SHT_SYMTAB					; symbol table
je	save_symtab

cmp	eax,SHT_STRTAB					; string table
je	save_strtab

jmp	not_section

save_symtab:
mov	eax,[esi+SH32_OFFSET]				; get offset
add	eax,[loadbuf]
add	eax,16						; skip first

mov	[symtab],eax

mov	edx,[esi+SH32_SIZE]				; get size
shr	edx,4						; divide by sixteen
dec	edx

mov	[number_of_symbols],edx
mov	[BOOT_INFO_NUM_SYMBOLS],edx
jmp	not_section

save_strtab:
mov	ebx,[loadbuf]
movsx	ebx,word [ebx+ELF64_SINDEX]			; get string header index

cmp	ecx,ebx						; if is .shstrtab
je	not_section

mov	eax,[esi+SH32_OFFSET]				; get offset
add	eax,[loadbuf]
inc	eax

mov	[strtab],eax
jmp	not_section

not_section:
; point to next section header
mov	edx,[loadbuf]					; point to elf header
movsx	eax,word [edx+ELF32_SHDRSIZE]			; size of section header
add	esi,eax						; point to next section header

inc	ecx						; increment counter
cmp	ecx,[sectioncount]
jle	next_find_sections


;
; copy the symbol names and addresses to symbol table
;

mov	edi,[symbols_start]				; output address
mov	ebx,[symtab]					; point to symbol table

next_symbol:
mov	esi,[strtab]					; point to string table
add	esi,[ebx+SYM32_NAME]				; point to string
dec	esi

next_string:
a32	movsb

cmp	byte [edi-1],0
jne	next_string

mov	eax,[ebx+SYM32_VALUE]				; get value
mov	[edi],eax

add	edi,4						; point to next
add	ebx,SYM32_ENTRY_SIZE

mov	eax,[number_of_symbols]
dec	eax
mov	[number_of_symbols],eax

test	eax,eax					; if at end
jnz	next_symbol

;
; load program segments
;
;
load_segments:
mov	eax,[symbols_start]				; symbols address
mov	[BOOT_INFO_SYMBOL_START],eax

mov	eax,[symbols_start]				; output address
sub	edi,eax						; get size of symbol table
mov	[BOOT_INFO_SYMBOL_SIZE],edi

mov	esi,[loadbuf]
movsx	ecx,word [esi+ELF32_PHCOUNT]			; number of program headers
add	esi,[esi+ELF32_PHDR]				; point to program headers

next_programheader:
mov	eax,[esi+PHDR32_TYPE]				; get segment type
cmp	eax,PROG_LOADABLE
jne	next_segment

push	esi
push	ecx

mov	edi,[esi+PHDR32_VADDR]				; get destination address
sub	edi,KERNEL_HIGH					; minus higher-half address

mov	ecx,[esi+PHDR32_SEGSIZE]				; number of bytes to copy
mov	eax,[esi+PHDR32_OFFSET]				; get address
mov	esi,[loadbuf]					; add load address
add	esi,eax

a32 rep	movsb						; copy data

pop	ecx
pop	esi
next_segment:
loop	next_programheader

jmp	load_initrd

;
; load 64-bit ELF file
;
load_elf64:
mov	esi,[loadbuf]
movsx	ecx,word [esi+ELF64_PHCOUNT]			; number of program headers
add	esi,[esi+ELF64_PHDR]				; point to program headers offset in file low dword

xor	edx,edx						; clear counter

next_size64:
mov	eax,[esi+PHDR64_TYPE]				; get segment type
cmp	eax,PROG_LOADABLE
jne	next_segment_size64

add	edx,[esi+PHDR64_MEMSZ]

next_segment_size64:
loop	next_size64

mov	eax,LOAD_ADDRESS				; calculate destination address for symbols
add	eax,edx

mov	[symbols_start],eax

mov	esi,[loadbuf]
movsx	edx,word [esi+ELF64_SHCOUNT]			; number of program headers
mov	[sectioncount],edx
xor	ecx,ecx

; Copy symbol names and addresses

mov	esi,[loadbuf]
add	esi,[esi+ELF64_SHDR]				; point to section headers

next_find_sections64:
mov	eax,[esi+SH64_TYPE]				; get section type
cmp	eax,SHT_SYMTAB					; symbol table
je	save_symtab64

cmp	eax,SHT_STRTAB					; string table
je	save_strtab64

jmp	not_section64

save_symtab64:
mov	edx,[esi+SH64_OFFSET]				; get offset
add	edx,[loadbuf]
add	edx,SYM64_SIZE					; skip first
mov	[symtab],edx

mov	eax,[esi+SH64_SIZE]				; size of section data
xor	edx,edx

mov	ebx,SYM64_SIZE					; divide by SYM64_SIZE
div	ebx
dec	eax

mov	[number_of_symbols],eax
mov	[BOOT_INFO_NUM_SYMBOLS],eax
jmp	not_section64

save_strtab64:
mov	ebx,[loadbuf]
movsx	ebx,word [ebx+ELF64_SINDEX]			; get string header index

cmp	ecx,ebx						; if is .shstrtab
je	not_section64

found_symtab_section64:
mov	eax,[esi+SH64_OFFSET]				; get offset
add	eax,[loadbuf]

mov	[strtab],eax
;jmp	not_section64

not_section64:
; point to next section header
mov	edx,[loadbuf]					; point to elf header
movsx	eax,word [edx+ELF64_SHDRSIZE]			; size of section header
add	esi,eax						; point to next section header

inc	ecx						; increment counter
cmp	ecx,[sectioncount]
jle	next_find_sections64

;
; copy the symbol names and addresses to symbol table
;
mov	edi,[symbols_start]				; output address
mov	ebx,[symtab]					; point to symbol table

next_symbol64:
mov	esi,[strtab]					; point to string table
add	esi,[ebx+SYM64_NAME]				; point to string

next_string64:
a32	movsb

cmp	byte [edi-1],0
jne	next_string64

mov	eax,[ebx+SYM64_VALUE]				; get value low word
mov	[edi],eax
mov	eax,[ebx+SYM64_VALUE+4]				; get value high word
mov	[edi+4],eax

add	edi,8						; point to next
add	ebx,SYM64_SIZE

mov	eax,[number_of_symbols]
dec	eax
mov	[number_of_symbols],eax

test	eax,eax					; if at end
jnz	next_symbol64

;
; load program segments
;
;
load_segments64:
mov	eax,[symbols_start]				; symbols address
mov	[BOOT_INFO_SYMBOL_START],eax

mov	eax,[symbols_start]				; output address
sub	edi,eax						; get size of symbol table
mov	[BOOT_INFO_SYMBOL_SIZE],edi

xchg	bx,bx

mov	esi,[loadbuf]
movsx	ecx,word [esi+ELF64_PHCOUNT]			; number of program headers
add	esi,[esi+ELF64_PHDR]				; point to program headers

next_programheader64:
mov	eax,[esi+PHDR64_TYPE]				; get segment type
cmp	eax,PROG_LOADABLE
jne	next_segment64

push	esi
push	ecx

mov	edi,LOAD_ADDRESS				; get destination address low dword
mov	ecx,[esi+PHDR64_MEMSZ]				; number of bytes to copy
mov	eax,[esi+PHDR64_OFFSET]				; get address
mov	esi,[loadbuf]					; add load address
add	esi,eax

a32 rep	movsb						; copy data

pop	ecx
pop	esi
next_segment64:
loop	next_programheader64


;***********************
; Load initrd
;**********************
load_initrd:
mov	ebx,offset findbuf
mov	edx,offset initrd_name
call	find_file

test	eax,eax					; found file
jnz	initrd_file_not_found

mov	esi,offset loading_initrd
call	output16

; save initrd address

mov	edx,[BOOT_INFO_KERNEL_START]
add	edx,[BOOT_INFO_KERNEL_SIZE]		; Point to end of kernel
add	edx,[BOOT_INFO_SYMBOL_SIZE]

mov	[BOOT_INFO_INITRD_START],edx

mov	ecx,[ebx+_FILE_SIZE]			; save initrd size
mov	[BOOT_INFO_INITRD_SIZE],ecx

call	loadfile
jmp	short continue

noinitrd:
xor	edx,edx

mov	[BOOT_INFO_INITRD_START],edx
mov	[BOOT_INFO_INITRD_SIZE],edx

continue:
mov	ah,3h					; get cursor
xor	bh,bh
int	10h

add	dh,3
mov	[BOOT_INFO_CURSOR_ROW],dh
					; go to next line
xor	dl,dl					; go to start of line
mov	[BOOT_INFO_CURSOR_COL],dl

mov	ah,2h					; set cursor
xor	bh,bh
int	10h

call	detect_memory				; get memory size

mov	[BOOT_INFO_MEMORY_SIZE],eax
mov	[BOOT_INFO_MEMORY_SIZE+4],edx

mov	ax,10h
mov	ds,ax
mov	es,ax
mov	ss,ax

db	67h
jmp	0ffffh:10h					; jump to kernel

initrd_file_not_found:
mov	esi,offset initrd_notfound
call	output16				; display message

jmp	short noinitrd
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
;  entryno=(block+(block/2));

mov	ebx,eax				; entryno=block + (block/2)
shr	eax,1				; divide by two
add	eax,ebx
mov	[entryno],eax
jmp	ok

getnext_fat16:
shl	eax,1				; multiply by two
add	eax,ebx
mov	[entryno],eax
jmp 	ok

getnext_fat32:
shl	eax,2				; multiply by four
add	eax,ebx
mov	[entryno],eax

ok:
xor	eax,eax				
mov	al,[7c00h+_SECTORSPERBLOCK]	
mul	word [7c00h+_RESERVEDSECTORS]
mov	ecx,eax

;  blockno=(bpb->reservedsectors*bpb->sectorsperblock)+(entryno / (bpb->sectorsperblock*bpb->sectorsize));
;  entry_offset=(entryno % (bpb->sectorsperblock*bpb->sectorsize));	/* offset into fat */

xor	edx,edx
mov	eax,[entryno]
mov	ebx,[blocksize]
div	ebx

add	eax,ecx
mov	[fat_blockno],eax
mov	[entry_offset],edx

mov	ecx,2
mov	ebx,FAT_BUF			; buffer

push	ebx				; save address

mov	eax,[fat_blockno]			; read fat sector and fat sector + 1 to buffer

next_fatblock:
mov	edx,[BOOT_INFO_PHYSICAL_DRIVE]
call	readblock

mov	eax,[fat_blockno]			; read fat sector and fat sector + 1 to buffer
inc	eax				; block+1
mov	[fat_blockno],eax

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
mov	ax,[ebx]

mov	dx,[block]
rcr	dx,1				; copy lsb to carry flag
jc	is_odd

and	ax,0fffh					; is even
jmp	check_end

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

mov	edx,[BOOT_INFO_PHYSICAL_DRIVE]
mov	eax,[find_blockno]
mov	ebx,ROOTDIRADDR					; read root directory
call	readblock					; read block

mov	esi,ebx						; point to buffer

next_entry:
;
; compare name
;

mov	al,[ebx]					; skip deleted files
cmp	al,0E5h
jne	not_deleted

jmp	next

not_deleted:
cld
mov	esi,ebx
mov	edi,[find_name]					; point to name
mov	ecx,11						; size
rep	cmpsb						; compare
je	found_file					; load file

next:
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

mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]
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

mov	al,dl
and	al,80h	
test	al,al
jz	no_extensions

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
mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]
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
mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]
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
; returns memory size in eax:edx
;
detect_memory:
push	ebx
push	ecx
push	esi
push	edi
push	es

xor	ebx,ebx

xor	ax,ax				; segment:offset address for buffer
mov	es,ax
mov	di,INT15_BUFFER			; segment:offset address for buffer

xor	ecx,ecx					; clear memory count
xor	edx,edx					; clear memory count

xor	ebx,ebx

not_end_e820:
push	ecx
push	edx

mov	eax,0e820h				; get memory block
mov	edx,534d4150h
mov	ecx,24					; size of memory block
int	15h

pop	edx
pop	ecx

add	ecx,[es:di+INT15_E820_LENGTH_LOW]			; add size
adc	edx,[es:di+INT15_E820_LENGTH_HIGH]			; add size

test	ebx,ebx					; if at end
jnz	not_end_e820				; loop if not

add	ecx,8000h
adc	edx,0

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
jmp	short end_detect_memory

use_ebx_eax:
add	eax,1024				; plus conventional memory
shl	ebx,16					; multiply by 65536 because edx is in 64k blocks
shl	eax,10					; multiply by 1024 because eax is in kilobytes
add	eax,ebx					; add memory count below 16M
mov	ecx,eax
jmp	short end_detect_memory

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
jmp	short end_detect_memory

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
jmp	short end_detect_memory

no_da88h:
mov	ah,8ah					; get extended memory size
int	15h
jc	no_8ah
push	ax					; returned in dx:ax
push	dx
pop	ecx

add	ecx,1024*1024				; add conventional memory size
add	ecx,eax
jmp	short end_detect_memory

no_8ah:
hlt						; halt

end_detect_memory:
mov	eax,ecx

pop	es
pop	edi
pop	esi
pop	ecx
pop	ebx
ret	

ccpsys_notfound db "CCP.SYS not found",0
initrd_notfound db 10,13,"No initrd found (INITRD.SYS), skipping it.",0
noa20_error db "Can't enable A20 line",0
loading_ccp db "Loading CCP.SYS...",0
loading_initrd db 10,13,"Loading INITRD.SYS...",0
ELF32_invalid db 10,13,"CCP.SYS is not a valid ELF executable",10,13,0
pressanykey db " Press any key to reboot",10,13,0
ccp_name db    "CCP     SYS",0
initrd_name db "INITRD  SYS",0
crlf db 10,13,0
string_section_name db ".strtab",0
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
currentptr dd 0
symbols_start dd 0
symtab dd 0
strtab dd 0
number_of_symbols dd 0
sectioncount dd 0

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
