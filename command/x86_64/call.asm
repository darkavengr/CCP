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
global outputrrdir
global inputrrdir
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

exit:
mov 	rax,0x4c
int	0x21
ret

seek:
mov	rax,[rsp+24]
mov	rbx,[rsp+8]
mov	rdx,[rsp+16]
add 	ax,0x4200
int	0x21
ret

getfileattributes:
mov	rdx,[rsp+8]

mov	rax,0x4300
int	0x21
ret

setfileattributes:
mov	rdx,[rsp+8]

mov	rax,0x4301
int	0x21
ret

getfiletimedate:
mov	rbx,[rsp+8]

mov	rax,0x5700
int	0x21
ret
 

setfiletimedate:
mov	rbx,[rsp+8]
mov	rcx,[rsp+16]
mov	rdx,[rsp+24]

mov	rax,0x5701
int	0x21
ret

close:
mov	rbx,[rsp+8]

mov 	rax,0x3e00
int	0x21
ret

create:
mov	rdx,[rsp+8]

mov	rax,0x3c00
int	0x21
ret

open:
mov	rdx,[rsp+8]
mov	rax,[rsp+16]

add	ax,0x3d00
int	0x21
ret

delete:
mov	rdx,[rsp+8]

mov	rax,0x4100
int	0x21
ret

exec:
mov	rdx,[rsp+8]
mov	rbx,[rsp+16]

mov	rax,0x4b00
int	0x21
ret

findfirst:
mov	rdx,[rsp+8]
mov	rbx,[rsp+16]

mov	rax,0x4e00
int	0x21
ret

findnext:
mov	rdx,[rsp+8]
mov	rbx,[rsp+16]

mov	rax,0x4f00
int	0x21
ret

getcwd:
mov	rdx,[rsp+8]

mov	rax,0x4700
int	0x21
ret

chdir:
mov	rdx,[rsp+8]
mov	rax,0x3b00
int	0x21
ret

rename:
mov	rsi,[rsp+8]
mov	rdi,[rsp+16]

mov	rax,0x5600
int	0x21
ret

write:
mov	rbx,[rsp+8]
mov	rdx,[rsp+16]
mov	rcx,[rsp+24]

mov	ah,0x40
int	0x21
ret

read:
mov	rbx,[rsp+8]
mov	rdx,[rsp+16]
mov	rcx,[rsp+24]

mov	rax,0x3f00
int	0x21
ret

mkdir:
mov	rdx,[rsp+8]

mov	rax,0x3900
int	0x21
ret

rmdir:
mov	rdx,[rsp+8]

mov	rax,0x3a00
int	0x21
ret

getargs:
mov	rdx,[rsp+8]

mov	rax,0x6500
int	0x21
ret

getversion:
mov	rax,0x3000
int	0x21
ret

alloc:
mov	rax,0x4800
mov	rbx,[rsp+8]
int	0x21
ret

localalloc:
mov	rax,0x4801
mov	rbx,[rsp+8]
int	0x21
ret

free:
mov	rdx,[rsp+8]
mov	rax,0x4900
int	0x21
ret

localfree:
mov	rdx,[rsp+8]
mov	rax,0x4901
int	0x21
ret

shutdown:
mov	rax,0x7003
int	0x21
ret

restart:
mov	rax,0x7002
int	0x21
ret

getlasterror:
mov	rax,0x4d00
int	0x21
ret

findfirstprocess:
mov	rbx,[rsp+8]		; buffer

mov	rax,0x7009
int	0x21
ret

findnextprocess:
mov	rdx,[rsp+8]			; buffer
mov	rdx,[rsp+16]		; handle

mov	rax,0x700A
int	0x21
ret

addblockdevice:
mov	rdi,[rsp+8]
mov	rsi,[rsp+16]
mov	rbx,[rsp+24]
mov	rdx,[rsp+32]

mov	rax,0x700B
int	0x21
ret

addcharacterdevice:
mov	rdi,[rsp+8]
mov	rsi,[rsp+16]
mov	rbx,[rsp+24]
mov	rdx,[rsp+32]

mov	rax,0x700C
int	0x21
ret

switchtonextprocess:
mov	 ax,0x7011
int	0x21
ret

mov	rbx,[rsp+8]
mov	rdx,[rsp+16]

pop	rbx			; buffer
pop	rdx			; filename
mov	rax,0x7012
int	0x21
ret

getconsolereadhandle:
mov	rax,0x7014
int 	0x21
ret

setconsolereadhandle:
mov	rbx,[rsp+8]
mov	rax,0x7016
int 	0x21
ret

getconsolewritehandle:
mov	rax,0x7017
int 	0x21
ret

setconsolewritehandle:
mov	rbx,[rsp+8]
mov	rax,0x7018
int	0x21
ret

getcurrentprocess:
mov	rax,0x701A
int	0x21
ret

writeconsole:
mov	rdx,[rsp+8]
mov	rax,0x9
int	0x21
ret

getfilesize:
mov	rbx,[rsp+8]

mov	rax,0x7018		; get filrsize
int	0x21
ret

kill:
mov	rbx,[rsp+8]

mov	rax,0x7019
int	0x21
ret

setconsolecolour:
mov	rax,0x7020
mov	rbx,[rsp+8]
int	0x21
ret

load_kernel_module:
mov	rdx,[rsp+8]
mov	rbx,[rsp+16]
mov	rax,0x4b03
int	0x21
ret

dup:
mov	rbx,[rsp+8]
mov 	rax,0x45
int	0x21
ret

dup2:
mov	rbx,[rsp+8]
mov	rcx,[rsp+16]
mov 	rax,0x46
int	0x21
ret

tell:
mov	rbx,[rsp+8]
mov	rax,0x7003
int	0x21
ret

set_critical_error_handler:
mov	rax,0x2524
mov	rbx,[rsp+8]
int	0x21
ret

set_signal_handler:
mov	rax,0x8c00
mov	rdx,[rsp+8]
int	0x21
ret

signal:
mov	rax,0x8d00
mov	rbx,[rsp+8]
mov	rdx,[rsp+16]
int	0x21
ret

waitpid:
mov	rax,0x8a00
mov	rbx,[rsp+8]
int	0x21
ret

getpid:
mov	rax,0x5000
int	0x21
ret

getprocessinfomation:
mov	rax,0x6500
mov	rbx,[rsp+8]
int	0x21
ret

blockread:
mov	rax,0x9000
mov	rcx,[rsp+8]
mov	rdx,[rsp+16]
int	0x21
ret

pipe:
mov	ah,0x93
int	0x21
ret


blockwrite:
mov	rax,0x9100
mov	rcx,[rsp+8]
mov	rdx,[rsp+16]
int	0x21
ret

ioctl:
mov	rax,0x4401
mov	rbx,[rsp+8]
mov	rcx,[rsp+16]
mov	rdx,[rsp+24]
int	0x21
ret

dma_alloc:
mov	rax,0x7000
mov	rbx,[rsp+8]
int	0x21
ret

remove_block_device:
mov	rax,0x700D
mov	rdx,[rsp+8]
int	0x21
ret

remove_char_device:
mov	rax,0x700E
mov	rdx,[rsp+8]
int	0x21
ret

register_filesystem:
mov	rax,0x700D
mov	rdx,[rsp+8]
int	0x21
ret

lock_mutex:
mov	rax,0x702e
mov	rdx,[rsp+8]
int	0x21
ret

unlock_mutex:
mov	rax,0x702f
mov	rdx,[rsp+8]
int	0x21
ret

allocatedriveletter:
mov	rax,0x7028
int	0x21
ret

findcharacterdevice:
mov	rax,0x7029
mov	rbx,[rsp+8]
mov	rdx,[rsp+16]
int	0x21
ret

getblockdevice:
mov	rax,0x702a
mov	rbx,[rsp+8]
mov	rdx,[rsp+16]
int	0x21
ret

getdevicebyname:
mov	rax,0x702b
mov	rbx,[rsp+8]
mov	rdx,[rsp+16]
int	0x21
ret

getenv:
mov	rax,0x7030
int	0x21
ret

