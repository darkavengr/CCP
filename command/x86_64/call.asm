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
;xchg	bx,bx
mov 	rax,0x4c
int	0x21
ret

seek:
;xchg	bx,bx
mov	rax,rdi
mov	rbx,rsi

; rdx unchanged

add 	ax,0x4200
int	0x21
ret

getfileattributes:
;xchg	bx,bx
mov	rdx,rdi

mov	rax,0x4300
int	0x21
ret

chmod:
;xchg	bx,bx
mov	rdx,rdi

mov	rax,0x4301
int	0x21
ret

getfiletimedate:
;xchg	bx,bx
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
;xchg	bx,bx
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
;xchg	bx,bx
mov	rbx,rdi

mov 	rax,0x3e00
int	0x21
ret

create:
;xchg	bx,bx
mov	rdx,rdi

mov	rax,0x3c00
int	0x21
ret

open:
;xchg	bx,bx
mov	rdx,rdi
mov	rax,rsi

add	ax,0x3d00
int	0x21
ret

delete:
;xchg	bx,bx
mov	rdx,rdi

mov	rax,0x4100
int	0x21
ret

exec:
;xchg	bx,bx
mov	rdx,rdi
mov	rbx,rsi

mov	rax,0x4b00
int	0x21
ret

findfirst:
;xchg	bx,bx
mov	rdx,rdi
mov	rbx,rsi

mov	rax,0x4e00
int	0x21
ret

findnext:
;xchg	bx,bx
mov	rdx,rdi
mov	rbx,rsi

mov	rax,0x4f00
int	0x21
ret

getcwd:
;xchg	bx,bx
mov	rdx,rdi

mov	rax,0x4700
int	0x21
ret

chdir:
;xchg	bx,bx
mov	rdx,rdi
mov	rax,0x3b00
int	0x21
ret

rename:
;xchg	bx,bx
xchg	rsi,rdi

mov	rax,0x5600
int	0x21
ret

write:
;xchg	bx,bx
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
;xchg	bx,bx
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
;xchg	bx,bx
mov	rdx,rdi

mov	rax,0x3900
int	0x21
ret

rmdir:
;xchg	bx,bx
mov	rdx,rdi

mov	rax,0x3a00
int	0x21
ret

getargs:
;xchg	bx,bx
mov	rdx,rdi

mov	rax,0x6500
int	0x21
ret

getversion:
;xchg	bx,bx
mov	rax,0x3000
int	0x21
ret

alloc:
;xchg	bx,bx
mov	rax,0x4800
mov	rbx,rdi
int	0x21
ret

free:
;xchg	bx,bx
mov	rdx,rdi
mov	rax,0x4900
int	0x21
ret

shutdown:
;xchg	bx,bx
mov	rax,0x7003
int	0x21
ret

restart:
;xchg	bx,bx
mov	rax,0x7002
int	0x21
ret

getlasterror:
;xchg	bx,bx
mov	rax,0x4d00
int	0x21
ret

findfirstprocess:
;xchg	bx,bx
mov	rbx,rdi			; buffer

mov	rax,0x7009
int	0x21
ret

findnextprocess:
;xchg	bx,bx
mov	rdx,rdi			; buffer
mov	rbx,rsi			; handle

mov	rax,0x700A
int	0x21
ret

addblockdevice:
;xchg	bx,bx
mov	rbx,rdi

mov	rax,0x700B
int	0x21
ret

addcharacterdevice:
;xchg	bx,bx
mov	rbx,rdi

mov	rax,0x700C
int	0x21
ret

yield:
;xchg	bx,bx
mov	ax,0x7011
int	0x21
ret

getcurrentprocess:
;xchg	bx,bx
mov	rax,0x701A
int	0x21
ret

writeconsole:
;xchg	bx,bx
mov	rdx,rdi
mov	rax,0x9
int	0x21
ret

getfilesize:
;xchg	bx,bx
mov	rbx,rdi

mov	rax,0x7018		; get filesize
int	0x21
ret

kill:
;xchg	bx,bx
mov	rbx,rdi

mov	rax,0x7019
int	0x21
ret

load_kernel_module:
;xchg	bx,bx
mov	rdx,rdi
mov	rbx,rsi
mov	rax,0x4b03
int	0x21
ret

dup:
;xchg	bx,bx
mov	rbx,rdi
mov 	rax,0x45
int	0x21
ret

dup2:
;xchg	bx,bx
mov	rbx,rdi
mov	rcx,rsi
mov 	rax,0x46
int	0x21
ret

tell:
;xchg	bx,bx
mov	rbx,rdi
mov	rax,0x7003
int	0x21
ret

set_critical_error_handler:
;xchg	bx,bx
mov	rax,0x2524
mov	rbx,rdi
int	0x21
ret

set_signal_handler:
;xchg	bx,bx
mov	rax,0x8c00
mov	rdx,rdi
int	0x21
ret

signal:
;xchg	bx,bx
mov	rax,0x8d00
mov	rbx,rdi
mov	rdx,rsi
int	0x21
ret

waitpid:
;xchg	bx,bx
mov	rax,0x8a00
mov	rbx,rdi
int	0x21
ret

getpid:
;xchg	bx,bx
mov	rax,0x5000
int	0x21
ret

getprocessinfomation:
;xchg	bx,bx
mov	rax,0x6500
mov	rbx,rdi
int	0x21
ret

blockread:
;xchg	bx,bx
mov	rax,0x9000
mov	rcx,rdi
mov	rdx,rsi
int	0x21
ret

pipe:
;xchg	bx,bx
mov	ah,0x93
int	0x21
ret


blockwrite:
;xchg	bx,bx
mov	rax,0x9100
mov	rcx,rdi
mov	rdx,rsi
int	0x21
ret

ioctl:
;xchg	bx,bx
mov	rax,0x4401
mov	rbx,rdi
mov	rcx,rsi

; rdx is unchanged
int	0x21
ret

dma_alloc:
;xchg	bx,bx
mov	rax,0x7000
mov	rbx,rdi
int	0x21
ret

remove_block_device:
;xchg	bx,bx
mov	rax,0x700D
mov	rdx,rdi
int	0x21
ret

remove_char_device:
;xchg	bx,bx
mov	rax,0x700E
mov	rdx,rdi
int	0x21
ret

register_filesystem:
;xchg	bx,bx
mov	rax,0x700D
mov	rdx,rdi
int	0x21
ret

lock_mutex:
;xchg	bx,bx
mov	rax,0x702e
mov	rdx,rdi
int	0x21
ret

unlock_mutex:
;xchg	bx,bx
mov	rax,0x702f
mov	rdx,rdi
int	0x21
ret

allocatedriveletter:
;xchg	bx,bx
mov	rax,0x7028
int	0x21
ret

findcharacterdevice:
;xchg	bx,bx
mov	rax,0x7029
mov	rbx,rdi
mov	rdx,rsi
int	0x21
ret

getblockdevice:
;xchg	bx,bx
mov	rax,0x702a
mov	rbx,rdi
mov	rdx,rsi
int	0x21
ret

getdevicebyname:
;xchg	bx,bx
mov	rax,0x702b
mov	rbx,rdi
mov	rdx,rsi
int	0x21
ret

getenv:
;xchg	bx,bx
mov	rax,0x7030
int	0x21
ret

