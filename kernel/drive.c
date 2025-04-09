// Auto-generated file- do not edit!
#define NULL 0

extern ata_init();
extern pci_init();
extern clock_init();
extern console_init();
extern floppy_init();
extern keyb_init();
extern null_init();
extern pic_init();
extern pit_init();
extern serialio_init();
extern elf32_init();
extern elf64_init();
extern fat_init();
void driver_init(void) {
ata_init(NULL);
pci_init(NULL);
clock_init(NULL);
console_init(NULL);
floppy_init(NULL);
keyb_init(NULL);
null_init(NULL);
pic_init(NULL);
pit_init(NULL);
serialio_init(NULL);
elf32_init(NULL);
elf64_init(NULL);
fat_init(NULL);
}
