;
; CCP boot loader
;

%include "../kernel/hwdep/i386/bootinfo.inc"
%include "i386/fatinfo.inc"

NULL equ 0
LOAD_ADDRESS		equ 0x1000			;  address to to load to

PTABLE_BOOTABLE		equ 0
PTABLE_START_HEAD	equ 1
PTABLE_START_SECTOR	equ 2
PTABLE_START_CYLINDER	equ 3
PTABLE_SYSTEM_ID	equ 4
PTABLE_END_HEAD		equ 5
PTABLE_END_SECTOR	equ 6
PTABLE_END_CYLINDER	equ 7
PTABLE_START_LBA	equ 8
PTABLE_SECTOR_COUNT	equ 12

RETRY_COUNT equ 3						; number of times to retry

; addresses of variables

fattype equ end_prog + 41
blocksize equ end_prog + 42
TEMP_BUF equ end_prog + 44

org	0x7c00						; origin
 jmp	short	over
 db	0x90						; nop

;
; don't fill in these values, patch this boot loader into an existing FAT32 boot sector
;
 times 90-3 db 0

over:
mov	[BOOT_INFO_PHYSICAL_DRIVE],dl			; save drive

test	dl,0x80
jb	not_hd_init					; don't add starting LBA if not hard drive

; si points to partition table entry for this volume

add	si,PTABLE_START_LBA					; point to start LBA
mov	di,BOOT_INFO_BOOT_DRIVE_START_LBA

lodsw
add	ax,[si+PTABLE_START_LBA]				; add starting LBA low word
stosw				; copy low word
lodsw
adc	ax,[si+PTABLE_START_LBA+2]				; add starting LBA high word
stosw				; copy high word

not_hd_init:
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

mov	eax,[0x7c00+BPB_FAT32_ROOTDIR]

;
; read sectors from root directory and search
;

read_next_block:
call	add_data_area_start					; add data area

mov	bx,TEMP_BUF					; read root directory
call	readblock					; read block

mov	si,bx						; point to buffer
add	si,[blocksize]
	
next_entry:
push	si

;
; compare name
;

cld
mov	si,bx
mov	di,ccp_name					; point to name
mov	cx,11						; entry filename size
rep	cmpsb						; compare entry filename to ccp_name
je	found_file					; load file

add	bx,FAT_DIRECTORY_ENTRY_SIZE			; point to next directory entry

pop	si
cmp	si,bx						; if at end of block
jle	next_entry

call	getnextblock					; get next block

cmp	eax,0x0FFFFFF8					; if at end
jne	read_next_block

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

hlt					; halt

found_file:

; get start block

mov	ax,[bx+FAT_DIRECTORY_BLOCK_HIGH_WORD]
shl	eax,16
mov	ax,[bx+FAT_DIRECTORY_BLOCK_LOW_WORD]

;
; load ccpload.sys
;

mov	bx,LOAD_ADDRESS					; buffer

next_block:
push	eax

call	add_data_area_start				; add start of data area
call	readblock					; read block

add	si,[blocksize]					; point to next

pop	eax
call	getnextblock					; get next block

cmp	eax,0x0FFFFFF8					; if at end
jl	next_block					; loop if not

jmp	0:LOAD_ADDRESS				; jump to second-stage loader

;
; read block
;
; eax=block
; es:bx=buffer

readblock:
push	eax						; save registers
push	ebx
push	ecx
push	edx

mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]

cmp	dl,0x80
jb	not_hd						; don't add starting LBA if not hard drive

add	eax,[BOOT_INFO_BOOT_DRIVE_START_LBA]		; add partition starting LBA

not_hd:
push	eax
push	es						; save es:bx, int 0x13 ah=0x8 overwrites it
push	bx

mov	ah,0x8						; get drive parameters
int	0x13

pop	bx						; restore es:bx
pop	es
pop	eax						; save block number
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

shr	ah,2						; add top two bits of cylinder number to sector number
and	ah,0xc0
or	ch,ah

pop	ebx

xor	di,di						; reset count

next_retry:
mov	ah,0x2						; read sectors
mov	al,[0x7c00+BPB_SECTORSPERBLOCK]			; number of sectors to read
mov	dl,[BOOT_INFO_PHYSICAL_DRIVE]
int	0x13
jnc	read_exit

inc 	di					; increment counter

not_end:
cmp	di,RETRY_COUNT
jne	next_retry					; loop

jmp	badfile						; bad system

read_exit:
pop	edx
pop	ecx
pop	ebx
pop	eax						; restore registers
ret

;
; Add data area start to block number
;
; eax=block number
;
; returns: block number+data area
;
add_data_area_start:
sub	eax,2

movzx	ecx,byte [0x7c00+BPB_SECTORSPERBLOCK]			; sectors per block
mul	ecx

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
ret

;
; get next block
;
getnextblock:
push	ebx
push	ecx
push	edx

;entryno=block*4;				
shl	eax,2				; multiple by four

;blockno=bpb->reservedsectors+(entryno / bpb->sectorsize);

xor	edx,edx
movzx	ecx,word [0x7c00+BPB_SECTORSIZE]
div	ecx

add	ax,[0x7c00+BPB_RESERVEDSECTORS]

;entry_offset=entryno % bpb->sectorsize;	/* offset into fat */

; eax=blockno
; edx=entry_offset

mov	bx,TEMP_BUF			; buffer
call	readblock

add	ebx,edx				; get entry

mov	eax,[bx]

and	eax,0x0FFFFFFF

pop	edx
pop	ecx
pop	ebx
ret

ccp_name db "CCPLOAD SYS"

bad_system db "?",0
TIMES 510-($-$$) db 0
dw 0xaa55						; marker

end_prog:
