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
global pipe
global switch_to_next_task

switch_to_next_task:
mov 	ah,0x31
int	0x21
ret

exit:
mov 	ah,0x4c
int	0x21
ret

seek:
mov	eax,[esp+12]
mov	ebx,[esp+4]
mov	edx,[esp+8]
add 	ax,0x4200
int	0x21
ret

getfileattributes:
mov	edx,[esp+4]

mov	eax,0x4300
int	0x21
ret

setfileattributes:
mov	edx,[esp+4]

mov	eax,0x4301
int	0x21
ret

getfiletimedate:
mov	ebx,[esp+4]

mov	eax,0x5700
int	0x21
ret
 
setfiletimedate:
mov	ebx,[esp+4]
mov	ecx,[esp+8]
mov	edx,[esp+12]
mov	esi,[esi+16]

mov	eax,0x5701
int	0x21
ret

close:
mov	ebx,[esp+4]

mov 	ah,0x3e
int	0x21
ret

create:
mov	edx,[esp+4]

mov	eax,0x3c00
int	0x21
ret

open:
mov	edx,[esp+4]
mov	eax,[esp+8]

add	ax,0x3d00
int	0x21
ret

delete:
mov	edx,[esp+4]

mov	ah,0x41
int	0x21
ret

exec:
mov	edx,[esp+4]
mov	ebx,[esp+8]

mov	ax,0x4b00
int	0x21
ret

findfirst:
mov	edx,[esp+4]
mov	ebx,[esp+8]

mov	ah,0x4e
int	0x21
ret

findnext:
mov	edx,[esp+4]
mov	ebx,[esp+8]

mov	ah,0x4f
int	0x21
ret

getcwd:
mov	edx,[esp+4]

mov	ah,0x47
int	0x21
ret

chdir:
mov	edx,[esp+4]
mov	ah,0x3b
int	0x21
ret

rename:
mov	esi,[esp+4]
mov	edi,[esp+8]

mov	ah,0x56
int	0x21
ret

write:
mov	ebx,[esp+4]
mov	edx,[esp+8]
mov	ecx,[esp+12]

mov	ah,0x40
int	0x21

call	getlasterror
ret

read:
mov	ebx,[esp+4]
mov	edx,[esp+8]
mov	ecx,[esp+12]

mov	ah,0x3f
int	0x21
ret

mkdir:
mov	edx,[esp+4]

mov	ah,0x39
int	0x21
ret

rmdir:
mov	edx,[esp+4]

mov	ah,0x3a
int	0x21
ret

getargs:
mov	edx,[esp+4]

mov	ah,0x65
int	0x21
ret

getversion:
mov	ah,0x30
int	0x21
ret

alloc:
mov	ah,0x48
mov	ebx,[esp+4]
int	0x21
ret

free:
mov	ah,0x49
mov	edx,[esp+4]
int	0x21
ret

shutdown:
mov	eax,0x7003
int	0x21
ret

restart:
mov	eax,0x7002
int	0x21
ret

getlasterror:
mov	ah,0x4d
int	0x21
ret

findfirstprocess:
mov	ebx,[esp+4]		; buffer

mov	eax,0x7009
int	0x21
ret

findnextprocess:
mov	edx,[esp+4]			; buffer
mov	ebx,[esp+8]		; handle

mov	eax,0x700A
int	0x21
ret

addblockdevice:
mov	ebx,[esp+4]

mov	eax,0x700B
int	0x21
ret

addcharacterdevice:
mov	ebx,[esp+4]

mov	eax,0x700C
int	0x21
ret

switchtonextprocess:
mov	ax,0x7011
int	0x21
ret

mov	ebx,[esp+4]
mov	edx,[esp+8]

pop	ebx			; buffer
pop	edx			; filename
mov	eax,0x7012
int	0x21
ret

getcurrentprocess:
mov	eax,0x701A
int	0x21
ret

writeconsole:
mov	edx,[esp+4]
mov	ah,0x9
int	0x21
ret

getfilesize:
mov	ebx,[esp+4]

mov	eax,0x7018		; get filesize
int	0x21
ret

kill:
mov	ebx,[esp+4]

mov	eax,0x7019
int	0x21
ret

load_kernel_module:
mov	edx,[esp+4]
mov	ebx,[esp+8]
mov	eax,0x4b03
int	0x21
ret

dup:
mov	ebx,[esp+4]
mov 	ah,0x45
int	0x21
ret

dup2:
mov	ebx,[esp+4]
mov	ecx,[esp+8]
mov 	ah,0x46
int	0x21
ret

tell:
mov	ebx,[esp+4]
mov	eax,0x7003
int	0x21
ret

set_critical_error_handler:
mov	eax,0x2524
mov	ebx,[esp+4]
int	0x21
ret

set_signal_handler:
mov	ah,0x8c
mov	edx,[esp+4]
int	0x21
ret

signal:
mov	ah,0x8d
mov	ebx,[esp+4]
mov	edx,[esp+8]
int	0x21
ret

waitpid:
mov	ah,0x8a
mov	ebx,[esp+4]
int	0x21
ret

getpid:
mov	ah,0x50
int	0x21
ret

getprocessinfomation:
mov	ah,0x65
mov	ebx,[esp+4]
int	0x21
ret

blockread:
mov	ah,0x90
mov	ecx,[esp+4]
mov	edx,[esp+8]
int	0x21
ret

pipe:
mov	ah,0x93
int	0x21
ret


blockwrite:
mov	ah,0x91
mov	ecx,[esp+4]
mov	edx,[esp+8]
int	0x21
ret

ioctl:
mov	eax,0x4400
mov	ebx,[esp+4]
mov	ecx,[esp+8]
mov	edx,[esp+12]
int	0x21
ret

dma_alloc:
mov	eax,0x7000
mov	ebx,[esp+4]
int	0x21
ret

remove_block_device:
mov	eax,0x700D
mov	edx,[esp+4]
int	0x21
ret

remove_char_device:
mov	eax,0x700E
mov	edx,[esp+4]
int	0x21
ret

register_filesystem:
mov	eax,0x700D
mov	edx,[esp+4]
int	0x21
ret

lock_mutex:
mov	eax,0x702e
mov	edx,[esp+4]
int	0x21
ret

unlock_mutex:
mov	eax,0x702f
mov	edx,[esp+4]
int	0x21
ret

allocatedriveletter:
mov	eax,0x7028
int	0x21
ret

findcharacterdevice:
mov	eax,0x7029
mov	ebx,[esp+4]
mov	edx,[esp+8]
int	0x21
ret

getblockdevice:
mov	eax,0x702a
mov	ebx,[esp+4]
mov	edx,[esp+8]
int	0x21
ret

getdevicebyname:
mov	eax,0x702b
mov	ebx,[esp+4]
mov	edx,[esp+8]
int	0x21
ret

getenv:
mov	eax,0x7030
int	0x21
ret

