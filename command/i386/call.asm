global exit
global seek
global getfileattributes
global chmod
global getfiletimedate
global setfiletimedate
global close
global create
global delete
global findfirst
global findnext
global getcwd
global chdir
global rename
global write
global read
global mkdir
global rmdir
global getargs
global getversion
global alloc
global free
global shutdown
global restart
global getlasterror
global findfirstprocess
global findnextprocess
global bringforward
global addblockdevice
global addcharacterdevice
global switchtonextprocess
global dup
global dup2
global getcurrentprocess
global writeconsole
global exec
global open
global getfilesize
global outputredir
global inputredir
global kill
global loaddriver
global setconsolecolour
global tell
global set_critical_error_handler
global set_signal_handler
global signal
global getpid
global getprocessinfomation
global ioctl
global load_kernel_module
global remove_block_device
global add_block_device
global register_filesystem
global lock_mutex
global unlock_mutex
global getenv

exit:
mov 	eax,0x4c
int 	21h 
ret

seek:
mov	eax,[esp+12]
mov	ebx,[esp+4]
mov	edx,[esp+8]
add 	ax,0x4200
int 	21h
ret

getfileattributes:
mov	edx,[esp+4]

mov	eax,0x4300
int	21h
ret

setfileattributes:
mov	edx,[esp+4]

mov	eax,0x4301
int	21h
ret

getfiletimedate:
mov	ebx,[esp+4]

mov	eax,0x5700
int	21h
ret
 

setfiletimedate:
mov	ebx,[esp+4]
mov	ecx,[esp+8]
mov	edx,[esp+12]

mov	eax,0x5701
int 	21h
ret

close:
mov	ebx,[esp+4]

mov 	eax,3e00h
int 	21h
ret

create:
mov	edx,[esp+4]

mov	eax,3c00h
int 	21h
ret

open:
mov	edx,[esp+4]
mov	eax,[esp+8]

add	ax,3d00h
int 	21h
ret

delete:
mov	edx,[esp+4]

mov	eax,4100h
int 	21h
ret

exec:
mov	edx,[esp+4]
mov	ebx,[esp+8]

mov	eax,0x4b00
int 	21h
ret

findfirst:
mov	edx,[esp+4]
mov	ebx,[esp+8]

mov	eax,4e00h
int 	21h
ret

findnext:
mov	edx,[esp+4]
mov	ebx,[esp+8]

mov	eax,4f00h
int 	21h
ret

getcwd:
mov	edx,[esp+4]

mov	eax,4700h
int 	21h
ret

chdir:
mov	edx,[esp+4]
mov	eax,3b00h
int 	21h
ret

rename:
mov	esi,[esp+4]
mov	edi,[esp+8]

mov	eax,5600h
int 	21h
ret

write:
mov	ebx,[esp+4]
mov	edx,[esp+8]
mov	ecx,[esp+12]

mov	ah,40h
int 	21h
ret

read:
mov	ebx,[esp+4]
mov	edx,[esp+8]
mov	ecx,[esp+12]

mov	eax,3f00h
int 	21h
ret

mkdir:
mov	edx,[esp+4]

mov	eax,3900h
int 	21h
ret

rmdir:
mov	edx,[esp+4]

mov	eax,3a00h
int 	21h
ret

getargs:
mov	edx,[esp+4]

mov	eax,6500h
int 	21h
ret

getversion:
mov	eax,3000h
int 	21h
ret

alloc:
mov	eax,4800h
mov	ebx,[esp+4]
int	 21h
ret

free:
mov	edx,[esp+4]
mov	eax,4900h
int	21h
ret


shutdown:
mov	eax,0x7003
int 	21h
ret

restart:
mov	eax,0x7002
int	 21h
ret

getlasterror:
mov	eax,4d00h
int 	21h
ret

findfirstprocess:
mov	edx,[esp+4]		; buffer
mov	eax,0x7009
int 	21h
ret

findnextprocess:
mov	edx,[esp+4]			; buffer
mov	eax,0x700A
int 	21h
ret

addblockdevice:
mov	edi,[esp+4]
mov	esi,[esp+8]
mov	ebx,[esp+12]
mov	edx,[esp+16]

mov	eax,0x700B
int 	21h
ret

addcharacterdevice:
mov	edi,[esp+4]
mov	esi,[esp+8]
mov	ebx,[esp+12]
mov	edx,[esp+16]

mov	eax,0x700C
int 	21h
ret

switchtonextprocess:
mov	 ax,0x7011
int	 21h
ret

mov	ebx,[esp+4]
mov	edx,[esp+8]

pop	ebx			; buffer
pop	edx			; filename
mov	eax,0x7012
int	 21h
ret

getconsolereadhandle:
mov	eax,0x7014
int 	21h
ret

setconsolereadhandle:
mov	ebx,[esp+4]
mov	eax,0x7016
int 	21h
ret

getconsolewritehandle:
mov	eax,0x7017
int 	21h
ret

setconsolewritehandle:
mov	ebx,[esp+4]
mov	eax,0x7018
int 	21h
ret

getcurrentprocess:
mov	eax,0x701A
int 	21h
ret

writeconsole:
mov	edx,[esp+4]
mov	eax,9h
int 	21h
ret

getfilesize:
mov	ebx,[esp+4]

mov	eax,7018h		; get filesize
int	21h
ret

kill:
mov	ebx,[esp+4]

mov	eax,7019h
int	21h
ret

setconsolecolour:
mov	eax,0x7020
mov	ebx,[esp+4]
int	21h
ret

load_kernel_module:
mov	edx,[esp+4]
mov	ebx,[esp+8]
mov	eax,0x4b03
int 	21h
ret

dup:
mov	ebx,[esp+4]
mov 	eax,45h
int 	21h
ret

dup2:
mov	ebx,[esp+4]
mov	ecx,[esp+8]
mov 	eax,46h
int 	21h
ret

tell:
mov	ebx,[esp+4]
mov	eax,0x7003
int 	21h
ret

set_critical_error_handler:
mov	eax,2524h
mov	ebx,[esp+4]
int	21h
ret

set_signal_handler:
mov	eax,8c00h
mov	edx,[esp+4]
int	21h
ret

signal:
mov	eax,8d00h
mov	ebx,[esp+4]
mov	edx,[esp+8]
int	21h
ret

waitpid:
mov	eax,8a00h
mov	ebx,[esp+4]
int	21h
ret

getpid:
mov	eax,5000h
int	21h
ret

getprocessinfomation:
mov	eax,6500h
mov	ebx,[esp+4]
int	21h
ret

blockread:
mov	eax,9000h
mov	ecx,[esp+4]
mov	edx,[esp+8]
int	21h
ret

blockwrite:
mov	eax,9100h
mov	ecx,[esp+4]
mov	edx,[esp+8]
int	21h
ret

ioctl:
mov	eax,4401h
mov	ebx,[esp+4]
mov	ecx,[esp+8]
mov	edx,[esp+12]
int	21h
ret

dma_alloc:
mov	eax,7000h
mov	ebx,[esp+4]
int	21h
ret

remove_block_device:
mov	eax,700Dh
mov	edx,[esp+4]
int	21h
ret

remove_char_device:
mov	eax,700Eh
mov	edx,[esp+4]
int	21h
ret

register_filesystem:
mov	eax,700Dh
mov	edx,[esp+4]
int	21h
ret

lock_mutex:
mov	eax,702eh
mov	edx,[esp+4]
int	21h
ret

unlock_mutex:
mov	eax,702fh
mov	edx,[esp+4]
int	21h
ret

allocatedriveletter:
mov	eax,7028h
int	21h
ret

findcharacterdevice:
mov	eax,7029h
mov	ebx,[esp+4]
mov	edx,[esp+8]
int	21h
ret

getblockdevice:
mov	eax,702ah
mov	ebx,[esp+4]
mov	edx,[esp+8]
int	21h
ret

getdevicebyname:
mov	eax,702bh
mov	ebx,[esp+4]
mov	edx,[esp+8]
int	21h
ret

getenv:
mov	eax,7030h
int	21h
ret

