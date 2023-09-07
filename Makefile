ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-gcc
FILES=drive.c memorymanager/memorymanager.c ccp.c devicemanager/device.c filemanager/vfs.c filemanager/initrd.c processmanager/process.c processmanager/multitasking.c processmanager/module.c header/string.c header/bootinfo.c  hwdep/i386/page.c

#
# Recipe to make everything
#
all: hwdep drivers ccp command

#
# Recipe to make kernel
#
ccp: obj/i386/drive.o obj/i386/memorymanager.o obj/i386/ccp.o obj/i386/device.o obj/i386/vfs.o obj/i386/process.o obj/i386/multitasking.o obj/i386/string.o
	$(CC) -T hwdep/i386/ld.script -o bin/i386/CCP.SYS -ffreestanding -nostdlib `ls obj/i386/*.o` -lgcc -Wl,-Map -Wl,ccp.map
	./gensym bin/i386/CCP.SYS

#
# Recipe to make hardware-dependent code
#

.PHONY: hwdep
hwdep:
	make -C hwdep/i386

#
# Recipe to make drivers
#
.PHONY: drivers
drivers:
	./gendrivelist   modules/drivers/ata modules/drivers/pci modules/drivers/clock modules/drivers/console modules/drivers/floppy modules/drivers/keyb modules/drivers/null modules/drivers/pci modules/drivers/pic modules/drivers/pit modules/drivers/pic modules/drivers/serialio modules/filesystems/fat modules/binfmt/elf32
	echo $(OBJFILES)

	for f in  modules/drivers/ata modules/drivers/pci modules/drivers/clock modules/drivers/console modules/drivers/floppy modules/drivers/keyb modules/drivers/null modules/drivers/pci modules/drivers/pic modules/drivers/pit modules/drivers/pic modules/drivers/serialio modules/filesystems/fat modules/binfmt/elf32; do make -C $$f;done
	$(foreach f,$(FILES),$(CC) -c -w -w -ffreestanding -nostdlib -lgcc  $f -o $(patsubst %.c,%.o, obj/i386/$(notdir $(f)));)

#
# Recipe to make command interpreter
#
.PHONY: command
command:
	make -C command

#
# Rules for building the core files
#

obj/i386/bootinfo.o:
	$(CC) -c -w header/bootinfo.c

obj/i386/drive.o:
	$(CC) -c -w drive.c

obj/i386/memorymanager.o:
	$(CC) -c -w  memorymanager/memorymanager.c

obj/i386/ccp.o:
	$(CC) -c -w ccp.c

obj/i386/device.o:
	$(CC) -c -w devicemanager/device.c

obj/i386/vfs.o:
	$(CC) -c -w filemanager/vfs.c

obj/i386/initrd.o:
	$(CC) -c -w filemanager/initrd.c

obj/i386/process.o:
	$(CC) -c -w processmanager/process.c

obj/i386/multitasking.o:
	$(CC) -c -w processmanager/multitasking.c

obj/i386/module.o:
	$(CC) -c -w processmanager/module.c

obj/i386/string.o:
	$(CC) -c -w header/string.c

$(patsubst %.c,%.o,../../../obj/i386/$(notdir $(FILES))):
	$(CC) -c -w  hwdep/i386/page.c


clean:
	rm bin/i386/*
	rm obj/i386/*
	rm *.map
	rm drive.c
	rm command/*.o

distclean:
	rm obj/i386/*
	rm bin/i386/*
	rm `find -iname Makefile`
	rm drive.c
	rm *.map
	rm command/*.o


