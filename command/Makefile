ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-ld

all:
	$(CC) -c -w -ffreestanding  -g *.c
	$(ASM) -felf32 i386/*.asm
	$(CC) -T ld.script.command -o ../bin/i386/COMMAND.RUN -ffreestanding -nostdlib `find -iname \*.o` -lgcc -Wl,-Map -Wl,../command.map

