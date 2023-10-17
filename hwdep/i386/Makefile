ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-gcc
FLAGS = -c -w -ffreestanding -nostdlib -lgcc 
ASMFLAGS = -felf32

all: ../../obj/i386/ex.o ../../obj/i386/partition.o ../../obj/i386/port.o ../../obj/i386/sched.o ../../obj/i386/mutex.o ../../obj/i386/interrupts.o ../../obj/i386/stack.o  ../../obj/i386/aainit.o ../../obj/i386/aamemorymap.o ../../obj/i386/usermode.o  ../../obj/i386/cpustate.o ../../obj/i386/initializepaging.o ../../obj/i386/tss.o ../../obj/i386/gdt.o ../../obj/i386/yield.o  ../../obj/i386/CCPLOAD.SYS ../../obj/i386/boot
../../obj/i386/ex.o:
		$(CC) $(FLAGS) ex.c -o ../../obj/i386/ex.o

../../obj/i386/partition.o:
		$(CC) $(FLAGS) partition.c -o ../../obj/i386/partition.o

../../obj/i386/port.o:
		$(CC) $(FLAGS) port.c -o ../../obj/i386/port.o

../../obj/i386/sched.o:
		$(ASM) $(ASMFLAGS) sched.asm -o ../../obj/i386/sched.o

../../obj/i386/mutex.o:
		$(CC) $(FLAGS) mutex.c -o ../../obj/i386/mutex.o

../../obj/i386/aamemorymap.o:
		$(ASM) $(ASMFLAGS) memorymap.asm -o ../../obj/i386/aamemorymap.o

../../obj/i386/aainit.o:
		$(ASM) $(ASMFLAGS) init.asm -o ../../obj/i386/aainit.o

../../obj/i386/stack.o:
		$(ASM) $(ASMFLAGS) stack.asm -o ../../obj/i386/stack.o

../../obj/i386/tss.o:
		$(ASM) $(ASMFLAGS) tss.asm -o ../../obj/i386/tss.o

../../obj/i386/cpustate.o:
		$(ASM) $(ASMFLAGS) cpustate.asm -o ../../obj/i386/cpustate.o

../../obj/i386/interrupts.o:
		$(ASM) $(ASMFLAGS) interrupts.asm -o ../../obj/i386/interrupts.o

../../obj/i386/gdt.o:
		$(ASM) $(ASMFLAGS) gdt.asm -o ../../obj/i386/gdt.o

../../obj/i386/usermode.o:
		$(ASM) $(ASMFLAGS) usermode.asm -o ../../obj/i386/usermode.o

../../obj/i386/initializepaging.o:
		$(ASM) $(ASMFLAGS) initializepaging.asm -o ../../obj/i386/initializepaging.o

../../obj/i386/yield.o:
		$(ASM) $(ASMFLAGS) yield.asm -o ../../obj/i386/yield.o

../../obj/i386/CCPLOAD.SYS:
		$(ASM) ccpload.asm -o ../../bin/i386/CCPLOAD.SYS

../../obj/i386/boot:
		$(ASM) boot.asm -o ../../bin/i386/boot

