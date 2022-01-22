ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-gcc

all:
		$(CC) -c -w -ffreestanding -nostdlib -lgcc  ex.c -o ../../obj/i386/ex.o
		$(CC) -c -w -ffreestanding -nostdlib -lgcc  partition.c -o ../../obj/i386/partition.o
		$(CC) -c -w -ffreestanding -nostdlib -lgcc  port.c -o ../../obj/i386/port.o
		$(CC) -c -w -ffreestanding -nostdlib -lgcc  sched.c -o ../../obj/i386/sched.o
		$(CC) -c -w -ffreestanding -nostdlib -lgcc  mutex.c -o ../../obj/i386/mutex.o
		$(ASM) -felf32 init.asm -o ../../obj/i386/aainit.o
		$(ASM) -felf32 yield.asm -o ../../obj/i386/yield.o
		$(ASM) ccpload.asm -o ../../bin/i386/CCPLOAD.SYS
		$(ASM) boot.asm -o ../../bin/i386/boot

