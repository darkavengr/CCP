;
; CCP boot loader
;

%include "../kernel/hwdep/i386/bootinfo.inc"
%include "i386/fatinfo.inc"

NULL equ 0
LOAD_SEG		equ 0			; segment:offset address to to load to
LOAD_ADDRESS		equ 0x1000

PTABLE_LBA equ 8

RETRY_COUNT equ 3						; number of times to retry

root_start equ end_prog					; addresses of variables
file_count  equ end_prog + 2				
blockstart  equ end_prog + 4
numtracks  equ end_prog + 8
sect  equ end_prog + 16
entrycount  equ end_prog + 18
blocksize  equ end_prog + 20
epb  equ end_prog + 22
retrycount  equ end_prog + 24
block equ end_prog + 26
sector equ end_prog + 28
fat_start_block equ end_prog + 30
startblock equ end_prog + 32
BOOT_DRIVE equ end_prog + 33
entryno  equ end_prog + 34
blockno  equ end_prog + 36
entry_offset  equ end_prog + 38
PTABLE_INFO equ end_prog + 43

fattype equ end_prog + 41
FAT_BUF equ end_prog + 42
ROOTDIRADDR equ end_prog + 512 + 43

 TRUE		equ 1
 FALSE		equ 0

org	0x7c00						; origin
 jmp	short	over					; eb 3c
 db	0x90						; nop
 db	"MSDOS5.0"					; oem name
 dw	512						; bytes per sector
 db	1						; sectors per block
 dw	1						; reserved sectors
 db 	2						; number of FATs
 dw	224						; maximum number of directory entries
 dw	2880						; maximum number of sectors if less than 65536 
 db	0xF0						; media descriptor
 dw	9						; sectors per FAT
 dw	18						; sectors per track
 dw	2						; number of heads
 dd	0						; number of sectors before this parition if hard disk
 dd	0						; number of sectors of sectors if more than 65536
 db	0						; drive number (useless)
 db 	0x29						; flags
 dw	0x29A,0x29A					; volume serial number
 db	"BOOT        "					; label
 db	"FAT12  "					; fat identification
over:
mov	[BOOT_INFO_PHYSICAL_DRIVE],dl			; save drive
mov	[PTABLE_INFO],si				; save pointer to partition table entry for this volume

mov	sp,0xfffe					; stack

mov	ax,cs						; cs
mov	ds,ax						; ds=cs
mov	es,ax
mov	ss,ax

mov	ax,[0x7c00+BPB_FATNAME+3]			; get fat type
xchg	ah,al						; swap bytes
; sub ax,0x3120 is the same as:
;  sub ax,0x3030
; sub ax,0xf0

sub	ax,0x3120					; from ascii to number
mov	[fattype],al					; save fat type

xor	ah,ah
mov	al,[0x7c00+BPB_SECTORSPERBLOCK]			; sectors per block
mul	word [0x7c00+BPB_SECTORSIZE]
mov	[blocksize],ax

;
; search root directory for ccpload.sys
;

; find root directory

;xchg	bx,bx

mov	bx,[0x7c00+BPB_SECTORSPERFAT]			; number of sectors per FAT
xor	ah,ah
mov	al,[0x7c00+BPB_NUMBEROFFATS]			; number of fats
mul	bx

add	ax,[0x7c00+BPB_RESERVEDSECTORS]			; reserved sectors
mov	[root_start],ax					; save root start

;
; read sectors from root directory and search
;

read_next_block:
mov	bx,ROOTDIRADDR					; read root directory
call	readblock					; read block


next_entry:
;
; compare name
;

cld
mov	si,bx
mov	di,ccp_name				; point to name
mov	cx,11						; size
rep	cmpsb						; compare
je	found_file					; load file

add	bx,FAT_DIRECTORY_ENTRY_SIZE			; point
cmp	bx,[blocksize]					; if at end of block
jle	next_entry

inc	word [entrycount]				; next entry
mov	dx,[entrycount]
mov	ax,[0x7c00+BPB_ROOTDIRENTRIES]

cmp	dx,[0x7c00+BPB_ROOTDIRENTRIES]
je	read_next_block					; loop

;
; not found
;

badfile:		
mov	si,bad_system				; point to message

output_message:
mov	ah,0xE						; output character
lodsb							; get character
int	0x10

test	al,al						; if at end
jne	output_message					; loop if not

xor	ax,ax
int	0x16						; get key
int	0x19						; reset

found_file:
mov	ax,[bx+FAT_DIRECTORY_BLOCK_LOW_WORD]		; get block
mov	[block],ax

;
; load ccpload.sys

mov	ax,LOAD_SEG					; segment
mov	es,ax						; put segment
mov	si,LOAD_ADDRESS					; buffer

next_block:
mov	bx,[block]

add	bx,[0x7c00+BPB_RESERVEDSECTORS]			; reserved sectors

xor	ah,ah
mov	cx,[0x7c00+BPB_SECTORSPERFAT]			; number of sectors per FAT
mov	al,[0x7c00+BPB_NUMBEROFFATS]			; number of fats
mul	cx
add	bx,ax

mov	ax,FAT_DIRECTORY_ENTRY_SIZE
mul	word [0x7c00+BPB_ROOTDIRENTRIES]			; number of root directory entries
xor	dx,dx
mov	cx,[0x7c00+BPB_SECTORSIZE]
div	cx
add	bx,ax
sub	bx,2						; data area starts at block 2

mov	ax,bx						; block number
mov	bx,si						; buffer
call	readblock					; read block

add	si,[blocksize]					; point to next

mov	ax,[block]					; start block
call	getnextblock					; get next block
mov	[block],ax

mov	cl,[fattype]					; get fat type
cmp	cl,0x16
je	check_fat16_end

; fat 12 otherwise
cmp	ax,0xff8					; if at end
jl	next_block					; loop if not

check_fat16_end:
cmp	ax,0xfff8					; if at end
jl	next_block					; loop if not
jmp	LOAD_SEG:LOAD_ADDRESS				; load


;
; read block
;
; ax=block
; es:bx=buffer

readblock:
push	ax						; save registers
push	bx
push	cx
push	dx

mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]
and	dl,0x80
cmp	dl,0x80
jb	not_hd						; don't add starting LBA if not hard drive

mov	si,[PTABLE_INFO]				; get pointer to partition table entry for this volume
mov	di,[BOOT_INFO_BOOT_DRIVE_START_LBA]

lodsw
add	ax,[si+PTABLE_LBA]				; add starting LBA

stosw
movsw

not_hd:
;
; convert block number to cylinder,head,sector
;

push	bx

xor 	dx,dx		
mov 	bx,[0x7c00+BPB_SECTORSPERTRACK]			; sectors per track
div	bx

mov	cl,dl						; save sector
inc	cl

xor 	dx,dx
mov 	bx,[0x7c00+BPB_NUMBEROFHEADS]			; number of heads
div 	bx

mov	dh,dl						; head
mov	ch,al						; cylinder	

xor	di,di						; reset count

pop	bx

next_retry:
mov	ah,2h						; read sectors
mov	al,[0x7c00+BPB_SECTORSPERBLOCK]			; block
mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]
int	13h
jnc	read_exit

inc 	di					; increment counter

not_end:
cmp	di,RETRY_COUNT
jne	next_retry					; loop

jmp	badfile						; bad system

read_exit:
pop	dx
pop	cx
pop	bx
pop	ax						; restore registers
ret

;
; get next block
;
getnextblock:
push	bx
push	cx
push	dx
push	es

mov	cl,[fattype]			; get fat type
cmp	cl,0x12				; fat12
je	getnext_fat12

; fat16 otherwise
shl	ax,1				; multiple by two
jmp	short ok

getnext_fat12:
mov	bx,ax				; entryno=block * (block/2)
shr	ax,1				; divide by two
add	ax,bx
mov	[entryno],ax

ok:
xor	ah,ah				;blockno=(next->reservedsectors*next->sectorsperblock)+(entryno / (next->sectorsperblock*next->sectorsize));		
mov	al,[0x7c00+BPB_SECTORSPERBLOCK]	
mul	word [0x7c00+BPB_RESERVEDSECTORS]
mov	cx,ax

;  entry_offset=(entryno % (next->sectorsperblock*next->sectorsize));	/* offset into fat */

xor	dx,dx
mov	ax,[entryno]
div	word [blocksize]		; entryno/blocksize
add	ax,cx
mov	[blockno],ax
mov	[entry_offset],dx

mov	cx,2
mov	bx,FAT_BUF			; buffer

push	bx				; save address

xor	ax,ax				; segment
mov	es,ax

next_fatblock:
mov	ax,[blockno]			; read fat sector and fat sector + 1 to buffer
mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]
call	readblock

inc	ax				; block+1
add	bx,[blocksize]
loop	next_fatblock

pop	bx				; restore address
add	bx,[entry_offset]		; get entry
mov	ax,[bx]

mov	ch,[fattype]			; get fat type
cmp	ch,0x16				; fat 16
je	check_end

; fat12 otherwise

shift_word:
push	ax
mov	ax,[block]
rcr	ax,1				; copy lsb to carry flag
pop	ax
jc	is_odd

and	ax,0xfff					; is even
jmp	short check_end

is_odd:
shr	ax,4

check_end:
pop	es
pop	dx
pop	cx
pop	bx
ret
	     
ccp_name db "CCPLOAD SYS"
bad_system db "System?",0
TIMES 510-($-$$) db 0
dw 0xaa55						; marker

end_prog:
