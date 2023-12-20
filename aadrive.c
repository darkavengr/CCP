// Auto-generated file- do not edit!
extern void set_current_kernel_module();
#define NULL 0

void driver_init(void) {
set_in_module_flag();
set_current_kernel_module("modules/drivers/ata");
ata_init(NULL);
set_current_kernel_module("modules/drivers/pci");
pci_init(NULL);
set_current_kernel_module("modules/drivers/clock");
clock_init(NULL);
set_current_kernel_module("modules/drivers/console");
console_init(NULL);
set_current_kernel_module("modules/drivers/floppy");
floppy_init(NULL);
set_current_kernel_module("modules/drivers/keyb");
keyb_init(NULL);
set_current_kernel_module("modules/drivers/null");
null_init(NULL);
set_current_kernel_module("modules/drivers/pci");
pci_init(NULL);
set_current_kernel_module("modules/drivers/pic");
pic_init(NULL);
set_current_kernel_module("modules/drivers/pit");
pit_init(NULL);
set_current_kernel_module("modules/drivers/pic");
pic_init(NULL);
set_current_kernel_module("modules/drivers/serialio");
serialio_init(NULL);
set_current_kernel_module("modules/filesystems/fat");
fat_init(NULL);
set_current_kernel_module("modules/binfmt/elf32");
elf32_init(NULL);
clear_in_module_flag();
}
