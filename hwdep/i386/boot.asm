;
; CCP boot loader
;
 %define offset

  NULL equ 0
 LOAD_SEG		equ 0			; segment:offset address to to load to
 LOAD_ADDRESS		equ 1000h

 _FILENAME 		equ 0				; fat entry offsets
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

RETRY_COUNT equ 3						; number of times to retry

 _JUMP 			equ 0					; bpb offsets											
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
DRIVE equ end_prog + 40
fattype equ end_prog + 41
FAT_BUF equ end_prog + 42
ROOTDIRADDR equ end_prog + 512 + 43


 _ENTRY_SIZE 		equ 32
 MAX_CLUSTER_SIZE 	equ 32767			; maxiumum cluster size
 EOF equ 0xffffffff					; end of file

 TRUE		equ 1
 FALSE		equ 0

 _READONLY	equ 1				; file attributes
 _HIDDEN 	equ 2
 _SYSTEM 	equ 4
 VOL_LABEL	equ 8
 _DIRECTORY 	equ 16
 _ARCHIVE	equ 32

org	7c00h						; origin
 jmp	short	over					; eb 3c
 db	90h						; nop
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
mov	[0xAC],dl					; save drive

mov	sp,0fffeh					; stack

mov	ax,cs						; cs
mov	ds,ax						; ds=cs
mov	es,ax
mov	ss,ax

mov	ax,[7c00h+_FATNAME+3]				; get fat type
xchg	ah,al						; swap bytes
; sub ax,3120h is the same as:
;  sub ax,3030h
; sub ax,0f0h

sub	ax,3120h					; from ascii to number
mov	[fattype],al					; save fat type

xor	ah,ah
mov	al,[7c00h+_SECTORSPERBLOCK]			; sectors per block
mul	word [7c00h+_SECTORSIZE]
mov	[blocksize],ax

;
; search root directory for ccp.sys
;

; find root directory

mov	bx,[7C00H+_SECTORSPERFAT]			; number of sectors per FAT
xor	ah,ah
mov	al,[7c00h+_NUMBEROFFATS]			; number of fats
mul	bx

add	al,[7c00h+_RESERVEDSECTORS]			; reserved sectors

mov	bx,ax
xor	ah,ah
mov	al,[7c00h+_SECTORSPERBLOCK]			; sectors per block
mul	bx
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
mov	di,offset ccp_name				; point to name
mov	cx,11						; size
rep	cmpsb						; compare
je	found_file					; load file

add	bx,_ENTRY_SIZE					; point
cmp	bx,[blocksize]					; if at end of block
jle	next_entry

inc	word [entrycount]				; next entry
mov	dx,[entrycount]
mov	ax,[7c00h+_ROOTDIRENTRIES]

cmp	dx,[7c00h+_ROOTDIRENTRIES]
je	read_next_block					; loop

;
; not found
;

badfile:		
mov	si,offset bad_system

output_message:
mov	ah,0eh						; output character
lodsb							; get character
int	10h

test	al,al						; if at end
jne	output_message					; loop if not

xor	ax,ax
int	16h						; get key
int	19h						; reset

found_file:
mov	ax,[bx+_BLOCK_LOW_WORD]				; get block
mov	[block],ax

;
; load ccp.sys

mov	ax,LOAD_SEG					; segment
mov	es,ax						; put segment
mov	si,LOAD_ADDRESS					; buffer

next_block:
mov	bx,[block]

add	bx,[7c00h+_RESERVEDSECTORS]			; reserved sectors

xor	ah,ah
mov	cx,[7C00H+_SECTORSPERFAT]			; number of sectors per FAT
mov	al,[7c00h+_NUMBEROFFATS]			; number of fats
mul	cx
add	bx,ax

mov	ax,_ENTRY_SIZE
mul	word [7C00H+_ROOTDIRENTRIES]			; number of root directory entries
xor	dx,dx
mov	cx,[7c00h+_SECTORSIZE]
div	cx
add	bx,ax
sub	bx,2						; data area starts at block 2

mov	ax,bx						; block number
mov	bx,si						; buffer
call	readblock					; read block

add	si,[blocksize]					; point to next
jnc	short no_inc					; carry set means next segment

increment_segment:
mov	ax,es
add	ax,1000h
mov	es,ax

xor	bx,bx
no_inc:
mov	ax,[block]					; start block
call	getnextblock					; get next block
mov	[block],ax

mov	cl,[fattype]					; get fat type
cmp	cl,16h
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

;
; convert block number to cylinder,head,sector
;

push	bx

xor 	dx,dx		
mov 	bx,[7c00h+_SECTORSPERTRACK]			; sectors per track
div	bx

mov	cl,dl						; save sector
inc	cl

xor 	dx,dx
mov 	bx,[7c00h+_NUMBEROFHEADS]			; number of heads
div 	bx

mov	dh,dl						; head
mov	ch,al			

xor	di,di						; reset count

pop	bx
next_retry:

mov	ah,2h						; read sectors
mov	al,[7c00h+_SECTORSPERBLOCK]			; block
mov	dl,[DRIVE]
int	13h
jnc	read_exit

inc 	di					; increment counter

cmp	dh,[7c00h+_SECTORSPERTRACK]		; at end of track
jl	not_end

inc	ch					; next track
xor	dh,dh

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
cmp	cl,12h				; fat12
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
mov	al,[7c00h+_SECTORSPERBLOCK]	
mul	word [7c00h+_RESERVEDSECTORS]
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
mov	dl,[DRIVE]
call	readblock

inc	ax				; block+1
add	bx,[blocksize]
loop	next_fatblock

pop	bx				; restore address
add	bx,[entry_offset]		; get entry
mov	ax,[bx]

mov	ch,[fattype]			; get fat type
cmp	ch,16h				; fat 16
je	check_end

; fat12 otherwise

shift_word:
push	ax
mov	ax,[block]
rcr	ax,1				; copy lsb to carry flag
pop	ax
jc	is_odd

and	ax,0fffh					; is even
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
TIMES 510-($-$$) db 0ffh
dw 0aa55h						; marker

end_prog:
