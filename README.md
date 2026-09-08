# CCP Version 0.0.1

CCP is a free (who would want to use it, let alone pay for it?!) portable, modular operating system that can be run on many differerent types of hardware.

## Disclaimer:
This project is not finished and is rather buggy. However, you are welcome to contribute. See "contributions" below.

## Installation:

### From a ready-made disk image:
Just download and run it on an emulator, or use dd or a similar tool to write it to a disk.

### From source code:

First, configure the Makefiles used to compile the source:

  ./configure
	
Compile:

  make

Create a disk image. CCP currently only supports booting from FAT volumes and from floppy and hard disks.

To create a floppy disk image floppy.img:

	dd if=/dev/zero of=floppy.img
	mount -o loop floppy.img /mnt
	cp bin/arch/CCP.SYS bin/arch/CCPLOAD.SYS bin/arch/COMMAND.RUN /mnt
	umount /mnt

## Contributions:

Contributions are welcome:

- The project needs USB drivers.
- The ACPI AML interpreter is incomplete.
- Swapping needs to be implemented

## Licence

CCP is free and open source under the GPL licence.

