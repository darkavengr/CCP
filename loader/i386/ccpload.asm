;
;
; CCP intermediate loader
; 
;
%include "../kernel/hwdep/i386/bootinfo.inc"
%include "i386/fatinfo.inc"

NULL equ 0
LOAD_ADDRESS		equ 0x100000				; load address for CCP.SYS
ROOTDIRADDR		equ 0x7e00+1024			; buffer for root directory
FAT_BUF			equ 0x7e00+2048
TEMP_ADDRESS		equ 0x7e00+4096			; temporary address for int 13h

CCPLOAD_STACK		equ 0xFFFE				; stack address

KERNEL_HIGH		equ 0x80000000
BASE_OF_SECTION 	equ 0x1000

RETRY_COUNT	equ	3
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
PHDR32_OFFSET	equ 4
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
PHDR64_OFFSET	equ 8
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

DAP_SIZE		equ	0
DAP_UNUSED		equ	1
DAP_SECTOR_COUNT	equ	2
DAP_OFFSET		equ	4
DAP_SEGMENT		equ	6
DAP_SECTOR_NUMBER	equ	8

org	BASE_OF_SECTION
cli							; disable interrupts

bits 16
use16
mov	sp,0xE000				; temp stack

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
; Load temporary GDT, switch to protected mode, load IDT and then switch back to (flat) real mode
;

cli	
xor	ax,ax
mov	es,ax
mov	ss,ax
mov	ds,ax

push	ds
push	es
push	ss

mov	edi,gdtinfo

db 	0x66
lgdt 	[ds:di]					; load gdt

mov 	eax,cr0   				; switch to protected mode
or	al,1
mov  	cr0,eax

mov   	bx,0x10			; load selector
mov   	ds,bx
mov   	es,bx
mov   	ss,bx

mov 	eax,cr0  		; switch to unreal mode
and	al,0xFE
mov  	cr0,eax

pop	ss
pop	es
pop	ds

mov	al,[BOOT_INFO_PHYSICAL_DRIVE]			; get physical drive

test	al,0x80						; is hard drive
jz	not_boot_hd

sub	al,0x80						; get logical drive from physical drive
add	al,2						; logical drives start from drive 2

not_boot_hd:
mov	[BOOT_INFO_DRIVE],al

; get FAT volume type

movzx	eax,word [0x7c00+BPB_NUMBEROFSECTORS]			; get number of sectors
test	eax,eax				; if zero, use 0x7c00+BPB_SECTORSDWORD
jnz	short get_sector_count_done

mov	eax,[0x7c00+BPB_SECTORSDWORD]

; fall through

get_sector_count_done:
xor	edx,edx
movzx	ecx,byte [0x7c00+BPB_SECTORSPERBLOCK]
div	ecx					; get number of sectors

cmp	eax,4095
jb	is_fat12				; FAT12

cmp	eax,65525
jb	is_fat16				; FAT16

mov	cl,0x32					; FAT32
jmp	short got_fat_type

is_fat12:
mov	cl,0x12
jmp	short got_fat_type

is_fat16:
mov	cl,0x16

; fall through

got_fat_type:
mov	[fattype],cl

movzx	ax,byte [0x7C00+BPB_SECTORSPERBLOCK]			; sectors per block
mov	dx,[0x7C00+BPB_SECTORSIZE]
mul	dx 
mov	[blocksize],ax

;**************************************
; Load kernel
;**************************************

mov	esi,loading_ccp
call	output16

mov	ebx,findbuf
mov	edx,ccp_name
call	find_file

test	eax,eax					; found file
jnz	ccpsys_not_found

; read files
mov	ebx,findbuf
; save kernel load address

mov	edx,LOAD_ADDRESS
mov	[BOOT_INFO_KERNEL_START],edx

; save kernel size

mov	ecx,[ebx+FAT_DIRECTORY_FILE_SIZE]
mov	[BOOT_INFO_KERNEL_SIZE],ecx

; save current end address

mov	eax,edx					; address
add	eax,[ebx+FAT_DIRECTORY_FILE_SIZE]			; +size
mov	[currentptr],eax

add	edx,(1024*1024)			; load after file load address so it can be relocated
mov	[loadbuf],edx
call	loadfile				

mov	esi,edx
mov	eax,[esi+ELF32_MAGIC]				; get elf marker
cmp	eax,0x464C457F					; check if file is elf file
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
mov	ah,[esi+ELF32_BITS]			; get 32 or 64-bit
cmp	ah,ELF32_BIT
je	load_elf32

jmp	load_elf64

load_elf32:
mov	ax,[esi+ELF32_PHCOUNT]			; check program header count
test	eax,eax
jnz	is_phok
jmp	bad_elf

bad_elf:
mov	esi,ELF32_invalid
call	output16

mov	esi,pressanykey
call	output16				; display message

call	readkey

int	0x19

is_phok:
;
; Copy kernel symbols before copying kernel, copying segments overwrites temporary buffer
;

; find size of program segments to calculate destination address

mov	esi,[loadbuf]
movsx	ecx,word [esi+ELF32_PHCOUNT]			; number of program headers

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
mov	eax,[esi+SH32_OFFSET]				; get
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
mov	eax,[esi+SH32_OFFSET]				; get
add	eax,[loadbuf]
inc	eax

mov	[strtab],eax
jmp	not_section

not_section:
; point to next section header
mov	edx,[loadbuf]					; point to elf header
movsx	eax,word [edx+ELF32_SHDRSIZE]			; size of section header
add	esi,eax						; point to next section header

loop	next_find_sections

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

mov	eax,[ebx+SYM32_NAME]

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
add	esi,[esi+ELF64_PHDR]				; point to program headers in file low dword

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
mov	[BOOT_INFO_SYMBOL_START],eax

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
mov	edx,[esi+SH64_OFFSET]				; get
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
mov	eax,[esi+SH64_OFFSET]				; get
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
mov	ebx,findbuf
mov	edx,initrd_name
call	find_file

test	eax,eax					; found file
jnz	initrd_file_not_found

mov	esi,loading_initrd
call	output16

; save initrd address

mov	edx,[BOOT_INFO_KERNEL_START]
add	edx,[BOOT_INFO_KERNEL_SIZE]		; Point to end of kernel
add	edx,[BOOT_INFO_SYMBOL_SIZE]

mov	[BOOT_INFO_INITRD_START],edx

mov	ecx,[ebx+FAT_DIRECTORY_FILE_SIZE]			; save initrd size
mov	[BOOT_INFO_INITRD_SIZE],ecx

call	loadfile
jmp	short continue

noinitrd:
xor	edx,edx

mov	[BOOT_INFO_INITRD_START],edx
mov	[BOOT_INFO_INITRD_SIZE],edx

continue:
mov	ah,0x3					; get cursor location
xor	bh,bh
int	10h

add	dh,3
mov	[BOOT_INFO_CURSOR_ROW],dh
					; go to next line
xor	dl,dl					; go to start of line
mov	[BOOT_INFO_CURSOR_COL],dl

call	detect_memory				; get memory size

mov	[BOOT_INFO_MEMORY_SIZE],eax
mov	[BOOT_INFO_MEMORY_SIZE+4],edx

mov	ax,0x10
mov	ds,ax
mov	es,ax
mov	ss,ax

db	0x67
jmp	0xffff:0x10					; jump to kernel

initrd_file_not_found:
mov	esi,initrd_notfound
call	output16				; display message

jmp	short noinitrd
;
; not found
;
ccpsys_not_found:
mov	esi,ccpsys_notfound
call	output16				; display message

presskey:
mov	esi,pressanykey
call	output16				; display message

call	readkey
int	0x19					; reset

;
; get next block
;
getnextblock:
push	ebx
push	ecx
push	edx

mov	[block],eax

cmp	byte [fattype],0x12
je	getnext_mult_blockno_fat12

cmp	byte [fattype],0x16
je	getnext_mult_blockno_fat16

cmp	byte [fattype],0x32
je	getnext_mult_blockno_fat32

getnext_mult_blockno_fat12:
mov	ebx,eax				; entryno=block * (block/2)
shr	eax,1				; divide by two
add	ebx,eax
mov	[entryno],ebx
jmp	short ok

getnext_mult_blockno_fat16:
shl	eax,1				; multiple by two
mov	[entryno],eax
jmp	short ok

getnext_mult_blockno_fat32:
shl	ax,2				; multiple by four
mov	[entryno],eax

; fall through

ok:
; blockno=next->reservedsectors+(entryno / next->sectorsize);		
; entry_offset=(entryno % next->sectorsize);	/* into fat */

mov	eax,[entryno]
xor	edx,edx
movzx	ecx,word [0x7c00+BPB_SECTORSIZE]
div	ecx

movzx	ecx,word [0x7c00+BPB_RESERVEDSECTORS]
add	eax,ecx

mov	[blockno],eax
mov	[entry_offset],edx

mov	ecx,2
mov	ebx,FAT_BUF			; buffer

push	ebx

mov	eax,[blockno]			; read fat sector and fat sector+1 to buffer

next_fatblock:
call	readblock

inc	eax				; block+1
add	ebx,[blocksize]
loop	next_fatblock

pop	ebx
add	ebx,[entry_offset]		; get entry offset

cmp	byte [fattype],0x12
je	getentry_blockno_fat12

cmp	byte [fattype],0x16
je	getentry_blockno_fat16

cmp	byte [fattype],0x32
je	getentry_blockno_fat32

getentry_blockno_fat12:
xor	eax,eax

mov	ax,[ebx]

mov	cx,[block]
and	cx,1
test	cx,cx
jnz	is_odd

and	eax,0xfff					; is even
jmp	short got_entry

is_odd:
shr	eax,4
jmp	short got_entry

getentry_blockno_fat16:
mov	ax,[ebx]
jmp	short got_entry

getentry_blockno_fat32:
mov	eax,[ebx]

; fall through
got_entry:
pop	edx
pop	ecx
pop	ebx
ret

;
; Search root directory for file
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

;
; search root directory for CCP.SYS
;

; find root directory

cmp	byte [fattype],0x32
je	get_root_fat32					; get root directory for FAT32 volumes

; FAT12/FAT16 otherwise

movzx	ebx,word [0x7c00+BPB_SECTORSPERFAT]			; number of sectors per FAT
movzx	eax,byte [0x7c00+BPB_NUMBEROFFATS]			; number of FATs
mul	ebx

movzx	ecx,word [0x7c00+BPB_RESERVEDSECTORS]
add	eax,ecx				; reserved sectors
jmp	saveblock

get_root_fat32:
mov	eax,[0x7c00+BPB_FAT32_ROOTDIR]

; fall through

saveblock:
mov	[find_blockno],eax

;
; read sectors from root directory and search for CCP.SYS
;

xor	edx,edx
mov	[blockcount],edx			; clear counter

read_next_block:
mov	eax,[find_blockno]

; For FAT32 volumes, add start of data area to each block number

mov	cl,[fattype]
cmp	cl,0x32
jne	not_fat32

call	add_data_area_start

; fall through

not_fat32:
mov	ebx,ROOTDIRADDR					; read root directory
call	readblock					; read block

mov	esi,ebx						; point to buffer

next_entry:
;
; compare name
;

mov	al,[ebx]					; skip deleted files
cmp	al,FAT_DELETED
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
add	ebx,FAT_DIRECTORY_ENTRY_SIZE
mov	eax,[blockcount]
add	eax,FAT_DIRECTORY_ENTRY_SIZE					; to next entry
mov	[blockcount],eax

cmp	eax,[blocksize]					; if at end of block
jle	next_entry

mov	eax,[find_blockno]
call	getnextblock					; get next block
mov	[find_blockno],eax

mov	ah,[fattype]
cmp	ah,0x12
je	check_fat_12

cmp	ah,0x16
je	check_fat_16

cmp	ah,0x32
je	check_fat_32

check_fat_12:
mov	eax,0xFF8
jl	read_next_block

check_fat_16:
mov	eax,0xFFF8
jl	read_next_block	

check_fat_32:
mov	eax,0x0FFFFFF8
jl	read_next_block

jmp	short end_find

found_file:
mov	esi,ebx
mov	edi,[find_buf]
mov	ecx,FAT_DIRECTORY_ENTRY_SIZE
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

; get start block

mov	al,[fattype]	
cmp	al,0x12
je	fat_12_16_getblock

cmp	al,0x16
je	fat_12_16_getblock

cmp	al,0x32	
je	fat_32_getblock

fat_12_16_getblock:
movzx	eax,word [ebx+FAT_DIRECTORY_BLOCK_LOW_WORD]
mov	[block],eax
jmp	short foundblock

fat_32_getblock:
xor	eax,eax
mov	ax,[ebx+FAT_DIRECTORY_BLOCK_HIGH_WORD]
shl	eax,16
mov	ax,[ebx+FAT_DIRECTORY_BLOCK_LOW_WORD]

mov	[block],eax

; fall through

foundblock:

next_block:
call	add_data_area_start				; add start of data area

;call	debug_print_hex

mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]
mov	ebx,[loadptr]					; buffer
call	readblock					; read block

mov	ebx,[loadptr]					; buffer
add	ebx,[blocksize]					; point to next
mov	[loadptr],ebx

mov	ax,0x0E2E		; print .
int	0x10

;
; get next block
;

mov	eax,[block]					; start block
call	getnextblock					; get next block
mov	[block],eax

;
; check if at end
;
mov	cl,[fattype]					; get FAT type
cmp	cl,0x16
je	check_fat16_end

cmp	cl,0x32
je	check_fat32_end

; fat 12 otherwise
cmp	ax,0xFF8					; if at end
jae	end_load

check_fat16_end:
cmp	ax,0xFFF8					; if at end
jae	end_load

check_fat32_end:
cmp	eax,0x0FFFFFF8					; if at end
jae	end_load

jmp	next_block

end_load:
xor	eax,eax				; loaded ok

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
push	eax					; save registers
push	ebx
push	ecx
push	edx
push	esi
push	edi

mov	[save_esp],esp				; save esp
mov	[dest_addr],ebx

cmp	byte [BOOT_INFO_PHYSICAL_DRIVE],0x80	; if hard drive
jb	not_hard_drive

add	eax,[BOOT_INFO_BOOT_DRIVE_START_LBA]	; add starting LBA for the partition

not_hard_drive:
mov	[read_block],eax

mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]

cmp	dl,0x80					; if hard drive
jb	no_extensions				; skip extensions if not

mov	ah,0x41					; check for int 13h extensions
mov	bx,0x55AA
int	0x13
jc	no_extensions

; set up disk address packet

mov	esi,dap
mov	al,0x10
mov	[esi+DAP_SIZE],al				; size of disk address packet

xor	al,al
mov	[esi+DAP_UNUSED],al				; reserved

xor	ecx,ecx

mov	al,[0x7C00+BPB_SECTORSPERBLOCK]	
mov	[esi+DAP_SECTOR_COUNT],al				; number of sectors to read

mov	eax,[read_block]
mov	[esi+DAP_SECTOR_NUMBER],eax				; block number

mov	ax,ds					; segment:address to read to
mov	[esi+DAP_SEGMENT],ax
mov	ax,TEMP_ADDRESS
mov	[esi+DAP_OFFSET],ax

xor	eax,eax						; clear counter
mov	dword [retry_counter],eax

next_retry_ext:
mov	ah,0x42				; extended read
mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]
mov	esi,dap
int	0x13
jnc	ext_read_ok

inc	dword [retry_counter]
cmp	dword [retry_counter],RETRY_COUNT
jne	next_retry_ext					; loop

mov	eax,0xFFFFFFFF
jmp	end_read

ext_read_ok:
mov	esi,TEMP_ADDRESS
mov	edi,[dest_addr]
mov	ecx,[blocksize]

db	0x67
rep	movsb
jmp	end_read

no_extensions:

push	es						; restore es:bx, int 0x13, ah=0x8 overwrites it
push	ebx

xor	edx,edx
xor	ecx,ecx

mov	ah,0x8						; get drive parameters
int	0x13

pop	ebx						; restore es:bx
pop	es

mov	eax,[read_block]						; save block number
;xchg	bx,bx

and	cx,0000000000111111b				; get sectors per track

shr	dx,8						; get number of heads
inc	dl						; heads is zero-based
;
; convert block number to cylinder,head,sector
;

push	ebx						; save buffer address
push	edx						; save number of heads
push	ecx						; save sectors per track

xor 	edx,edx		
pop	ebx						; sectors per track
div	ebx

mov	cl,dl						; save sector
inc	cl

xor 	edx,edx
pop	ebx						; number of heads
div 	ebx

mov	dh,dl						; head
mov	ch,al						; cylinder	

shr	ah,2						; add bits 8-9 of cylinder to sector number
and	ah,0xc0
or	ch,ah

xor	edi,edi						; reset count

next_retry:
;
; read block to buffer somewhere below 640k because int 13h can't read > 1Mb
;
mov	bx,TEMP_ADDRESS

mov	ah,0x2						; read sectors
mov	al,[0x7c00+BPB_SECTORSPERBLOCK]			; number of sectors
mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]
int	0x13
jnc	copy_data

inc 	edi					; increment counter

cmp	edi,RETRY_COUNT
jne	next_retry

bad_read:						; error reading file
inc	dword [retry_counter]
cmp	dword [retry_counter],RETRY_COUNT
jne	next_retry					; loop

mov	eax,0xFFFFFFFF
jmp	short end_read

copy_data:
pop	edi
mov	esi,TEMP_ADDRESS
mov	ecx,[blocksize]

db	0x67
rep	movsb

end_read:
mov	esp,[save_esp]			; restore esp

pop	edi
pop	esi
pop	edx						; restore registers
pop	ecx
pop	ebx
pop	eax
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

mov	ah,0xE						; output character
mov	al,[esi]
int	0x10

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
int	0x16					; get key
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

xor	ax,ax				; segment:address for buffer
mov	es,ax
mov	di,INT15_BUFFER			; segment:address for buffer

xor	ecx,ecx					; clear memory count
xor	edx,edx					; clear memory count

xor	ebx,ebx

not_end_e820:
push	ecx
push	edx

mov	eax,0xE820				; get memory block
mov	edx,0x534D4150
mov	ecx,24					; size of memory block
int	0x15

pop	edx
pop	ecx

add	ecx,[es:di+INT15_E820_LENGTH_LOW]			; add size
adc	edx,[es:di+INT15_E820_LENGTH_HIGH]			; add size

test	ebx,ebx					; if at end
jnz	not_end_e820				; loop if not

add	ecx,0x8000
adc	edx,0

jmp	end_detect_memory

no_e820:
mov	ax,0xe801				; get memory info
int	0x15
jc	no_e801					; e801 not supported

cmp	ah,0x86					; not supported
je	no_e801

cmp	ah,0x80					; not supported
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
mov	ah,0x88					; get extended memory count
int	0x15
jc	no_88h					; not supported

test	ax,ax					; error
jz	no_88h					; not supported

cmp	ah,0x86					; not supported
je	no_88h

cmp	ah,0x80					; not supported
je	no_88h

add	eax,1024				; plus conventional memory
shl	eax,10					; multiply by 1024, size is in kilobytes
mov	ecx,eax
jmp	short end_detect_memory

no_88h:
xor	ecx,ecx
xor	ebx,ebx

mov	ax,0xDA88				; get extended memory size
int	15h
jc	no_da88h				; not supported

; returned in cl:bx
shr	ecx,16					; shift and add
add	ecx,ebx
pop	edx
add	ecx,1024					; add conventional memory size
jmp	short end_detect_memory

no_da88h:
mov	ah,0x8A					; get extended memory size
int	0x15
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

;
; Add data area start to block number
;
;
; eax=block number
; returns: block number+data area
;
add_data_area_start:
push	ebx
push	ecx
push	edx

mov	ebx,eax
sub	ebx,2						; data area starts at block 2

movzx	eax,byte [0x7c00+BPB_SECTORSPERBLOCK]			; sectors per block
mul	ebx
mov	ebx,eax

mov	cl,[fattype]					; get FAT type

cmp	cl,0x32
je	data_start_fat32

; FAT12 or FAT16 otherwise

; add start of data area to block number

movzx	ecx,word [0x7c00+BPB_RESERVEDSECTORS]
add	ebx,ecx				; reserved sectors

movzx	ecx,word [0x7c00+BPB_SECTORSPERFAT]			; number of sectors per FAT
movzx	eax,byte [0x7c00+BPB_NUMBEROFFATS]			; number of fats
mul	ecx
add	ebx,eax

mov	eax,FAT_DIRECTORY_ENTRY_SIZE
movzx	ecx,word [0x7c00+BPB_ROOTDIRENTRIES]			; number of root directory entries
mul	ecx

xor	edx,edx
movzx	ecx,word [0x7c00+BPB_SECTORSIZE]
div	ecx
add	ebx,eax

mov	eax,ebx
jmp	add_data_start_done	

data_start_fat32:
push	eax

; DataArea=(SectorsPerFAT*NumberOfFATs)+NumberOfReservedSectors

; (SectorsPerFAT*NumberOfFATs)
xor	edx,edx
mov	ecx,[0x7c00+BPB_FAT32_SECTORS_PER_FAT]		; number of sectors per FAT
movzx	eax,byte [0x7c00+BPB_NUMBEROFFATS]		; number of fats
mul	ecx

;+NumberOfReservedSectors
movzx	ecx,word [0x7c00+BPB_RESERVEDSECTORS]		; add reserved sectors
add	eax,ecx

pop	ecx
add	eax,ecx

add_data_start_done:
pop	edx
pop	ecx
pop	ebx
ret

debug_print_hex:
push	eax
push	ebx
push	ecx
push	edx
push	esi
push	edi

mov	edx,0xf0000000		; bit mask
mov	cl,28			; shift value

next_digit:
push	eax			; save number

and	eax,edx			; mask off unwanted bits
shr	edx,4			; shift bit mask
shr	eax,cl			; shift bits to lowest nibble

sub	cl,4			; update shift value

mov	ebx,digits		; point to digits
add	ebx,eax			; point to hex digit

mov	ah,0xe			; output character
mov	al,[ebx]		; character
int	0x10

pop	eax			; restore number

cmp	cl,0xfc
jnz	next_digit

mov	ax,0x0e0d		; output newline
int	0x10

mov	ax,0x0e0a		; output newline
int	0x10

pop	edi
pop	esi
pop	edx
pop	ecx
pop	ebx
pop	eax
ret

digits db "0123456789ABCDEF",0
ccpsys_notfound db "loader: CCP.SYS not found",0
initrd_notfound db 10,13,"loader: No initrd found (INITRD.SYS), skipping it.",0
noa20_error db "loader: Can't enable A20 line",0
loading_ccp db "Loading CCP.SYS...",0
loading_initrd db 10,13,"Loading INITRD.SYS...",0
ELF32_invalid db 10,13,"loader: CCP.SYS is not a valid ELF executable",10,13,0
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

dap	TIMES 16 db 0
findbuf TIMES 32 db 0


gdtinfo:
dw gdt_end - gdt
dd gdt

; null entries, don't modify
gdt dw 0
dw 0
db 0
db 0,0
db 0

dw 0xFFFF					; low word of limit
dw 0						; low word of base
db 0						; middle byte of base
db 0x9A,0xCF					; Code segment
db 0						; last byte of base

dw 0xFFFF					; low word of limit
dw 0		 				; low word of base
db 0						; middle byte of base
db 0x92,0xCF					; Data segment
db 0						; last byte of base
gdt_end:
db 0

end_prog:

