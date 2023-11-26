ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-ld
FILES=pci.c

../../../obj/i386/pci.o:
	$(CC) -c -w -ffreestanding -nostdlib -lgcc  $(FILES) -o $(patsubst %.c,%.o,../../../obj/i386/$(notdir $(FILES))) 

