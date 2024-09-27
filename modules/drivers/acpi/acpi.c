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

#define MODULE_INIT acpi_init

#include <stdint.h>
#include <stddef.h>

#include "kernelhigh.h"
#include "acpi.h"

RSDP rsdp;
RSDT rsdt;
XSDT xsdt;
FACP *facp;
MADT *madt;

/*
* Initialize ACPI
*
* In: char *init
*
*  Returns: -1 on error, 0 on success
*
*/
size_t acpi_init(char *init) {
size_t count;
char *rsdp_ptr;
size_t rsdt_type=0;
RSDT *tableptr;
FACP facp;
DSDT dsdt;
MADT madt;
FADT fadt;
SRAT srat;
SSDT ssdt;

/* ACPI intialization - find RSDP */

rsdp_ptr=RSDP_START_ADDRESS;		/* start by scanning the EBDA */

while(rsdp_ptr < RSDP_END_ADDRESS) { 
	if(memcmp(rsdp_ptr,"RSD PTR ",8) break;	/* found rsdp */	

	rsdp += 16;
}

if(rsdp_ptr >= RSDP_END_ADDRESS) {	/* signature not found */
	kprintf_direct("acpi: RSDP not found\n");
	return(-1);
}

memcpy(rsdp,rsdp_ptr,sizeof(RSDP));	/* copy rdsp */

/* get RSDT */

if(rsdp.revision == 0) {		/* ACPI 1.0 */
	memcpy(rsdt,rsdp.rsdt_address+KERNEL_HIGH,sizeof(RSDT));

	rsdt.rsdt_entries=kernelalloc(rsdt.length);		/* allocate buffer for rsdt entries */

	if(rsdt.rsdt_entries == NULL) {
		kprintf_direct("acpi: Can't allocate memory for RSDT\n");
		return(-1);
	}

	memcpy(rsdt.rsdt_entries,rsdt+sizeof(RSDT),rsdt.length-sizeof(RSDT));		/* copy rsdt to struct */
	
	type=32;			/* 32-bit rsdt */
	length=rsdt.length/4;
	tableptr=rsdt.rsdt_entries;	/* point to RSDT entries */
}
else
{
	memcpy(xsdt,rsdp.xsdt_address+KERNEL_HIGH,xsdt.length-sizeof(XSDT));

	xsdt.xsdt_entries=kernelalloc(xsdt.length);		/* allocate buffer for xsdt entries */

	if(xsdt.xsdt_entries == NULL) {
		kprintf_direct("acpi: Can't allocate memory for XSDT\n");
		return(-1);
	}

	type=64;			/* 64-bit xsdt */
	length=xsdt.length/8;
	tableptr=xsdt.xsdt_entries;	/* point to XSDT entries */
}

/* Get copies of tables */

for(count=0;count<length;count++) {
	if(memcmp(tableptr->signature,"FACP",4) == 0) {		/* FACP */
		memcpy(&facp,tableptr,sizeof(FACP));
	} 
	else if(memcmp(tableptr->signature,"MADT",4) == 0) {
		memcpy(&madt,tableptr,sizeof(MADT));
	}
	else if(memcmp(tableptr->signature,"DSDT",4) == 0) {
		memcpy(&facp,tableptr,sizeof(FACP));
	}
	else if(memcmp(tableptr->signature,"MADT",4) == 0) {
		memcpy(&madt,tableptr,sizeof(MADT));
	}
	else if(memcmp(tableptr->signature,"FADT",4) == 0) {
		memcpy(&fadt,tableptr,sizeof(FADT));
	}
	else if(memcmp(tableptr->signature,"SRAT",4) == 0) {
		memcpy(&srat,tableptr,sizeof(SRAT));
	}
	else if(memcmp(tableptr->signature,"SSDT",4) == 0) {
		memcpy(&ssdt,tableptr,sizeof(SSDT));
	}

	tableptr++;
}

set_interrupt(fadt.SCI_Interrupt,&sci_interrupt_handler);		/* set SCI interrupt handler */

if(fadt.smi_command != 0) {		/* if ACPI not enabled */
	  outb(fadt.smi_command,fadt.acpi_enable);	/* enable ACPI */

	  while (inw(fadt->pm1a_control_block) & 1 == 0) ;; /* wait for ACPI to enable */
}

if(dsdt.amlptr != NULL) parse_aml(dsdt.amlptr);		/* parse dsdt aml code */
if(ssdt.amlptr != NULL) parse_aml(ssdt.amlptr);		/* parse ssdt aml code */

return(0);
}

int ACPIShutdown(void) {
}

int ACPIRestart(void) {
}

int ACPISleep(void) {
}

