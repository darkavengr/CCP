ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-ld
FILES=pic.c
ASMFILES=i386/pic.asm

../../../obj/i386/pic.o  ../../../obj/i386/aapic386.o:
	$(ASM) -felf32 -o ../../../obj/i386/aapic386.o $(ASMFILES)
	$(CC) -c -w -ffreestanding -nostdlib -lgcc  $(FILES) -o $(patsubst %.c,%.o,../../../obj/i386/$(notdir $(FILES))) 

