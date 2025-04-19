global exit
global seek
global getfileattributes
global chmod
global getfiletimedate
global touch
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
global kill
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
mov	rax,rdi
mov	rbx,rsi

; rdx unchanged

add 	ax,0x4200
int	0x21
ret

getfileattributes:
mov	rdx,rdi

mov	rax,0x4300
int	0x21
ret

chmod:
mov	rdx,rdi

mov	rax,0x4301
int	0x21
ret

getfiletimedate:
mov	r11,rdi
mov	r12,rsi
mov	r14,rdx
mov	r15,rcx

mov	rbx,r11
mov	rcx,r12
mov	rdx,r14
mov	rsi,r15

mov	rax,0x5700
int	0x21
ret
 
touch:
mov	r11,rdi
mov	r12,rsi
mov	r14,rdx
mov	r15,rcx

mov	rbx,r11
mov	rcx,r12
mov	rdx,r14
mov	rsi,r15

mov	rax,0x5701
int	0x21
ret

close:
mov	rbx,rdi

mov 	rax,0x3e00
int	0x21
ret

create:
mov	rdx,rdi

mov	rax,0x3c00
int	0x21
ret

open:
mov	rdx,rdi
mov	rax,rsi

add	ax,0x3d00
int	0x21
ret

delete:
mov	rdx,rdi

mov	rax,0x4100
int	0x21
ret

exec:
mov	rdx,rdi
mov	rbx,rsi

mov	rax,0x4b00
int	0x21
ret

findfirst:
mov	rdx,rdi
mov	rbx,rsi

mov	rax,0x4e00
int	0x21
ret

findnext:
mov	rdx,rdi
mov	rbx,rsi

mov	rax,0x4f00
int	0x21
ret

getcwd:
mov	rdx,rdi

mov	rax,0x4700
int	0x21
ret

chdir:
mov	rdx,rdi
mov	rax,0x3b00
int	0x21
ret

rename:
xchg	rsi,rdi

mov	rax,0x5600
int	0x21
ret

write:
mov	r11,rdi
mov	r12,rsi
mov	r14,rdx
mov	r15,rcx

mov	rbx,r11
mov	rdx,r12
mov	rcx,r14

mov	ah,0x40
int	0x21
ret

read:
mov	r11,rdi
mov	r12,rsi
mov	r14,rdx
mov	r15,rcx

mov	rbx,r11
mov	rdx,r12
mov	rcx,r14

mov	rax,0x3f00
int	0x21
ret

mkdir:
mov	rdx,rdi

mov	rax,0x3900
int	0x21
ret

rmdir:
mov	rdx,rdi

mov	rax,0x3a00
int	0x21
ret

getargs:
mov	rdx,rdi

mov	rax,0x6500
int	0x21
ret

getversion:
mov	rax,0x3000
int	0x21
ret

alloc:
mov	rax,0x4800
mov	rbx,rdi
int	0x21
ret

free:
mov	rdx,rdi
mov	rax,0x4900
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
mov	rbx,rdi			; buffer

mov	rax,0x7009
int	0x21
ret

findnextprocess:
mov	rdx,rdi			; buffer
mov	rbx,rsi			; handle

mov	rax,0x700A
int	0x21
ret

addblockdevice:
mov	rbx,rdi

mov	rax,0x700B
int	0x21
ret

addcharacterdevice:
mov	rbx,rdi

mov	rax,0x700C
int	0x21
ret

yield:
mov	ax,0x7011
int	0x21
ret

getcurrentprocess:
mov	rax,0x701A
int	0x21
ret

writeconsole:
mov	rdx,rdi
mov	rax,0x9
int	0x21
ret

getfilesize:
mov	rbx,rdi

mov	rax,0x7018		; get filesize
int	0x21
ret

kill:
mov	rbx,rdi

mov	rax,0x7019
int	0x21
ret

load_kernel_module:
mov	rdx,rdi
mov	rbx,rsi
mov	rax,0x4b03
int	0x21
ret

dup:
mov	rbx,rdi
mov 	rax,0x45
int	0x21
ret

dup2:
mov	rbx,rdi
mov	rcx,rsi
mov 	rax,0x46
int	0x21
ret

tell:
mov	rbx,rdi
mov	rax,0x7003
int	0x21
ret

set_critical_error_handler:
mov	rax,0x2524
mov	rbx,rdi
int	0x21
ret

set_signal_handler:
mov	rax,0x8c00
mov	rdx,rdi
int	0x21
ret

signal:
mov	rax,0x8d00
mov	rbx,rdi
mov	rdx,rsi
int	0x21
ret

waitpid:
mov	rax,0x8a00
mov	rbx,rdi
int	0x21
ret

getpid:
mov	rax,0x5000
int	0x21
ret

getprocessinfomation:
mov	rax,0x6500
mov	rbx,rdi
int	0x21
ret

blockread:
mov	rax,0x9000
mov	rcx,rdi
mov	rdx,rsi
int	0x21
ret

pipe:
mov	ah,0x93
int	0x21
ret


blockwrite:
mov	rax,0x9100
mov	rcx,rdi
mov	rdx,rsi
int	0x21
ret

ioctl:
mov	rax,0x4401
mov	rbx,rdi
mov	rcx,rsi

; rdx is unchanged
int	0x21
ret

dma_alloc:
mov	rax,0x7000
mov	rbx,rdi
int	0x21
ret

remove_block_device:
mov	rax,0x700D
mov	rdx,rdi
int	0x21
ret

remove_char_device:
mov	rax,0x700E
mov	rdx,rdi
int	0x21
ret

register_filesystem:
mov	rax,0x700D
mov	rdx,rdi
int	0x21
ret

lock_mutex:
mov	rax,0x702e
mov	rdx,rdi
int	0x21
ret

unlock_mutex:
mov	rax,0x702f
mov	rdx,rdi
int	0x21
ret

allocatedriveletter:
mov	rax,0x7028
int	0x21
ret

findcharacterdevice:
mov	rax,0x7029
mov	rbx,rdi
mov	rdx,rsi
int	0x21
ret

getblockdevice:
mov	rax,0x702a
mov	rbx,rdi
mov	rdx,rsi
int	0x21
ret

getdevicebyname:
mov	rax,0x702b
mov	rbx,rdi
mov	rdx,rsi
int	0x21
ret

getenv:
mov	rax,0x7030
int	0x21
ret

