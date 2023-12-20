// Auto-generated file- do not edit!
extern void set_current_module_name();
#define NULL 0

void driver_init(void) {
set_current_module_name("modules/drivers/ata");
ata_init(NULL);
set_current_module_name("modules/drivers/pci");
pci_init(NULL);
set_current_module_name("modules/drivers/clock");
clock_init(NULL);
set_current_module_name("modules/drivers/console");
console_init(NULL);
set_current_module_name("modules/drivers/floppy");
floppy_init(NULL);
set_current_module_name("modules/drivers/keyb");
keyb_init(NULL);
set_current_module_name("modules/drivers/null");
null_init(NULL);
set_current_module_name("modules/drivers/pci");
pci_init(NULL);
set_current_module_name("modules/drivers/pic");
pic_init(NULL);
set_current_module_name("modules/drivers/pit");
pit_init(NULL);
set_current_module_name("modules/drivers/pic");
pic_init(NULL);
set_current_module_name("modules/drivers/serialio");
serialio_init(NULL);
set_current_module_name("modules/drivers/test");
test_init(NULL);
set_current_module_name("modules/filesystems/fat");
fat_init(NULL);
set_current_module_name("modules/binfmt/elf32");
elf32_init(NULL);
clear_in_module_flag();
}
