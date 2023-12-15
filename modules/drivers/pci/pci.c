/*  CCP Version 0.0.1
	   (C) Matthew Boote 2020-2023
	   This file is part of CCP.
	   CCP is free software: you can redistribute it and/or modify
	   it under the terms of the GNU General Public License as published by
	   the Free Software Foundation, either version 3 of the License, or
	   (at your option) any later version.
	   CCP is distributed in the hope that it will be useful,
	   but WITHOUT ANY WARRANTY; without even the implied warranty of
	   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	   GNU General Public License for more details.
	   You should have received a copy of the GNU General Public License
	   along with CCP.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include "../../../header/errors.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"
#include "pci.h"

#define MODULE_INIT pci_init

uint32_t pci_get_deviceid(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_vendor(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_status(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_command(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_class_code(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_subclass(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_progif(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_revisionid(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_bar0(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_bar1(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_bar2(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_bar3(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_bar4(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_bar5(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_cardbus_cis_pointer(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_subsystemid(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_get_subsytem_vendor_id(uint32_t bus,uint32_t device,uint32_t function);
uint16_t pci_get_header_type(uint32_t bus,uint32_t device,uint32_t function);
uint16_t pci_get_secondary_bus(uint32_t bus,uint32_t device,uint32_t function);
uint32_t pci_read_word(uint32_t bus,uint32_t device,uint32_t function,uint32_t reg);
size_t pci_find_device(uint32_t bus,uint32_t device,uint32_t function,void *buf);
void pci_init(char *initstring);
size_t add_pci_device(uint16_t bus,uint32_t device,uint32_t function);
uint32_t pci_write_word(uint32_t bus,uint32_t device,uint32_t function,uint32_t reg,uint32_t data);
uint32_t pci_put_command(uint32_t bus,uint32_t device,uint32_t function,uint32_t data);
uint32_t pci_put_status(uint32_t bus,uint32_t device,uint32_t function,uint32_t data);

/*
 * Initialize PCI
 *
 * In:  char *init	Initialization string
 *
 * Returns: nothing
 *
 */
void pci_init(char *initstring) {
uint32_t function;
uint32_t bus;
uint32_t sbus;
uint16_t pciword;
uint32_t device;
int count;
uint16_t headertype;

for(bus=0;bus < 256;bus++) {
	for(device=0;device != 31;device++) {
		for(function=0;function<8;function++) {	
			if(pci_get_vendor(bus,device,function) == 0xffff) continue;		/* no device */

			if(pci_get_class_code(bus,device,function) == 6 && pci_get_class_code(bus,device,function) == 4) { /* pci to pci */
				sbus=pci_get_secondary_bus(bus,device,function);
	 
				for(function=0;function<8;function++) {	
					add_pci_device(sbus,device,function);
				}
			}
	   	   
	   		add_pci_device(bus,device,function);
		}
	}
}


}
/*
 * Get PCI device ID
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI device ID
 *
 */

uint32_t pci_get_deviceid(uint32_t bus,uint32_t device,uint32_t function) {
return((pci_read_word(bus,device,function,0) & 0xFFFF0000) >> 16);
}

/*
 * Get PCI vendor ID
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI vendor ID
 *
 */
uint32_t pci_get_vendor(uint32_t bus,uint32_t device,uint32_t function) {
return(pci_read_word(bus,device,function,0) & 0xFFFF);
}

/*
 * Get PCI status
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI status
 *
 */
uint32_t pci_get_status(uint32_t bus,uint32_t device,uint32_t function) {
return((pci_read_word(bus,device,function,4) & 0xFFFF0000) >> 16);
}

/*
 * Get PCI command
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI command
 *
 */ 
uint32_t pci_get_command(uint32_t bus,uint32_t device,uint32_t function) {
return((pci_read_word(bus,device,function,4) & 0xFFFF));
}

/*
 * Put PCI command
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
	data		PCI command to set
 *
 * Returns: nothing
 *
 */
uint32_t pci_put_command(uint32_t bus,uint32_t device,uint32_t function,uint32_t data) {
uint32_t command=pci_read_word(bus,device,function,4);

data |= ((pci_read_word(bus,device,function,4) & 0xffff0000) | data);

return(pci_write_word(bus,device,function,4,data));
}

/*
 * Put PCI status
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
	data		PCI status to set
 *
 * Returns: nothing
 *
 */
uint32_t pci_put_status(uint32_t bus,uint32_t device,uint32_t function,uint32_t data) {
data |= ((pci_read_word(bus,device,function,4) & 0xffff) | (data << 16));

return(pci_write_word(bus,device,function,4,data));
}

/*
 * Get PCI class code
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI class code
 *
 */
uint32_t pci_get_class_code(uint32_t bus,uint32_t device,uint32_t function) {
return((pci_read_word(bus,device,function,8) & 0xFFFF0000) >> 24);
}

/*
 * Get PCI device subclass
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI device subclass
 *
 */
uint32_t pci_get_subclass(uint32_t bus,uint32_t device,uint32_t function) {
return((pci_read_word(bus,device,function,8) & 0x00FF0000) >> 16);
}

/*
 * Get PCI programming interface byte
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI programming interface byte
 *
 */
uint32_t pci_get_progif(uint32_t bus,uint32_t device,uint32_t function) {
return((pci_read_word(bus,device,function,8) & 0x0000FF00) >> 16);
}

/*
 * Get PCI revision ID
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI revision ID
 *
 */
uint32_t pci_get_revisionid(uint32_t bus,uint32_t device,uint32_t function) {
return((pci_read_word(bus,device,function,8) & 0xFF) >> 16);
}

/*
 * Get PCI BAR (Bus Address Register) 0
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI BAR 0
 *
 */
uint32_t pci_get_bar0(uint32_t bus,uint32_t device,uint32_t function) {
return(pci_read_word(bus,device,function,0x10));
}

/*
 * Get PCI BAR (Bus Address Register) 1
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI BAR 1
 *
 */
uint32_t pci_get_bar1(uint32_t bus,uint32_t device,uint32_t function) {
return(pci_read_word(bus,device,function,0x14));
}

/*
 * Get PCI BAR (Bus Address Register) 2
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI BAR 2
 *
 */
uint32_t pci_get_bar2(uint32_t bus,uint32_t device,uint32_t function) {
return(pci_read_word(bus,device,function,0x18));
}

uint32_t pci_get_bar3(uint32_t bus,uint32_t device,uint32_t function) {
return(pci_read_word(bus,device,function,0x1C));

}

/*
 * Get PCI BAR (Bus Address Register) 4
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI BAR 4
 *
 */

uint32_t pci_get_bar4(uint32_t bus,uint32_t device,uint32_t function) {
return(pci_read_word(bus,device,function,0x20));
}

/*
 * Get PCI BAR (Bus Address Register) 5
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI BAR 0
 *
 */
uint32_t pci_get_bar5(uint32_t bus,uint32_t device,uint32_t function) {
return(pci_read_word(bus,device,function,0x24));
}

/*
 * Get PCI CardBus CIS information pointer
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
	
 * Returns: PCI CardBus CIS pointer
 *
 */
uint32_t pci_get_cardbus_cis_pointer(uint32_t bus,uint32_t device,uint32_t function) {
return(pci_read_word(bus,device,function,0x28));
}

/*
 * Get PCI sub-system ID
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI sub-system ID
 *
 */
uint32_t pci_get_subsystemid(uint32_t bus,uint32_t device,uint32_t function) {
return(pci_read_word(bus,device,function,0x2C) >> 16);
}

/*
 * Get PCI sub-system vendor ID
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI sub-system vendor ID
 *
 */
uint32_t pci_get_subsytem_vendor_id(uint32_t bus,uint32_t device,uint32_t function) {
return(((pci_read_word(bus,device,function,0x2C) & 0xFFFF)));
}

/*
 * Get PCI header type
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI header type
 *
 */
uint16_t pci_get_header_type(uint32_t bus,uint32_t device,uint32_t function) {
return(((pci_read_word(bus,device,function,0xC) & 0xFF00FFFF) >> 24));
}

/*
 * Get PCI secondary bus
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI secondary bus
 *
 */
uint16_t pci_get_secondary_bus(uint32_t bus,uint32_t device,uint32_t function) {
return((pci_read_word(bus,device,function,18) & 0xFF) >> 8);
}

/*
 * Read dword from PCI
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: dword
 *
 */
uint32_t pci_read_word(uint32_t bus,uint32_t device,uint32_t function,uint32_t reg) {
uint32_t address;

address=(bus << 16) | (device << 11) | (function << 8) | (reg & 0xfc) | 0x80000000;	/* Create PCI address from device,function and register */

outd(PCI_COMMAND,address);			/* send address to PCI */
return(ind(PCI_DATA));				/* get PCI data */
}

/*
 * Write dword to PCI
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
	data		Data to write to PCI
 *
 * Returns: PCI sub-system ID
 *
 */
uint32_t pci_write_word(uint32_t bus,uint32_t device,uint32_t function,uint32_t reg,uint32_t data) {
uint32_t address;

address=(bus << 16) | (device << 11) | (function << 8) | (reg & 0xfc) | 0x80000000;

outd(PCI_COMMAND,address);
outd(PCI_DATA,data);
}

/*
 * Find PCI device
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function
 *
 * Returns: PCI sub-system ID
 *
 */
size_t pci_find_device(uint32_t bus,uint32_t device,uint32_t function,void *buf) {
size_t count;
size_t headertype;
size_t headersize;
uint32_t *b;

headertype=pci_get_header_type(bus,device,function);		/* check if information exists */
if(headertype == 0xffff) return(-1);

if(headertype == 0 || headertype == 1) {			/* Size for type 0 and type 1 headers */
	headersize=64;
}
else								
{
	headersize=sizeof(pci_headertype02);
}

/* read pci data */

b=buf;

for(count=0;count<headersize;count++) {
	outd(PCI_COMMAND,(0x80000000+(bus << 23)+(device << 15)+(function << 10)+(count << 7)));

 *b++=ind(PCI_DATA);
}

return(0);
}

/*
 * Register PCI as character device
 *
 * In:  bus		PCI bus
	device		PCI device
	function 	PCI function	
 *
 * Returns: -1 on error, 0 on success
 *
 */
size_t add_pci_device(uint16_t bus,uint32_t device,uint32_t function) {
size_t *bufptr;
CHARACTERDEVICE chardevice;
size_t count;

ksprintf(chardevice.name,"PCI-%d-%d-%d",bus,device,function);
chardevice.flags=DEVICE_USE_DATA;
chardevice.ioctl=NULL;

chardevice.data=kernelalloc(sizeof(CHARACTERDEVICE));		/* allocate buffer */
if(chardevice.data == NULL) {
	kprintf_direct("\npci: Out of memory\n");
	return(-1); 
}

bufptr=chardevice.data;

for(count=0;count<(256/4);count++) {
	outb(PCI_COMMAND,count);		/* get byte from pci device */
	 *bufptr++=ind(PCI_DATA);
}

chardevice.next=NULL;
add_char_device(&chardevice);
}

