ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-ld
FILES=fat.c

../../../obj/i386/fat.o:

	$(CC) -c -w -ffreestanding -nostdlib -lgcc  $(FILES) -o $(patsubst %.c,%.o,../../../obj/i386/$(notdir $(FILES))) 

