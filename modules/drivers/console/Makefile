ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-ld
FILES=console.c

../../../obj/i386/console.o:
	$(CC) -c -w -ffreestanding -nostdlib -lgcc  -g $(FILES) -o $(patsubst %.c,%.o,../../../obj/i386/$(notdir $(FILES))) 

