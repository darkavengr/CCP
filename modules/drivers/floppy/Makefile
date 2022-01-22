ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-ld
FILES=floppy.c

all:
	$(CC) -c -w -ffreestanding -nostdlib -lgcc  $(FILES) -o $(patsubst %.c,%.o,../../../obj/i386/$(notdir $(FILES))) 

