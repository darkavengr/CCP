ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-gcc
FILES=drive.c memorymanager/memorymanager.c ccp.c devicemanager/device.c filemanager/vfs.c processmanager/process.c header/string.c  hwdep/i386/page.c

all:
	if [ ! -e bin ]; then mkdir bin;fi
	if [ ! -e bin/i386 ]; then mkdir bin/i386;fi

	if [ ! -e obj ]; then mkdir obj;fi
	if [ ! -e obj/i386 ]; then mkdir obj/i386;fi

	./gendrivelist   modules/drivers/ata modules/drivers/pci modules/drivers/clock modules/drivers/console modules/drivers/floppy modules/drivers/keyb modules/drivers/null modules/drivers/pci modules/drivers/pic modules/drivers/pit modules/drivers/pic modules/drivers/serialio modules/filesystems/fat
	echo $(OBJFILES)

	for f in  modules/drivers/ata modules/drivers/pci modules/drivers/clock modules/drivers/console modules/drivers/floppy modules/drivers/keyb modules/drivers/null modules/drivers/pci modules/drivers/pic modules/drivers/pit modules/drivers/pic modules/drivers/serialio modules/filesystems/fat; do make -C $$f;done
	$(foreach f,$(FILES),$(CC) -c -w -ffreestanding -nostdlib -lgcc  $f -o $(patsubst %.c,%.o, obj/i386/$(notdir $(f)));)

	make -C hwdep/i386

	$(CC) -T hwdep/i386/ld.script -o bin/i386/CCP.SYS -ffreestanding -nostdlib `ls obj/i386/*.o` -lgcc -Wl,-Map -Wl,ccp.map
	make -C command
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


