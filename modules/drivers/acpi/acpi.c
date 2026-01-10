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
#include "string.h"

RSDP rsdp;
RSDT rsdt;
XSDT xsdt;
FACP *facp;
MADT *madt;
size_t PowerButtonMode;
char *ACPIConfigInvalidValue="acpi: Invalid configuration value for %s: %s\n";

/*
* Initialize ACPI
*
* In: init
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
char *tokens[MAX_SIZE][MAX_SIZE];

PowerButtonMode=POWERBUTTON_DEFAULT;	/* use default method for now */

if(init != NULL) {			/* args found */
	tc=tokenize_line(init,tokens," ");	/* tokenize line */

	for(count=0;count < tc;count++) {
		tokenize_line(tokens[count],op,"=");	/* subtokenize token */

		if(strncmp(op[0],"powerbutton",MAX_PATH) == 0) {		/* power button handling */
			if(strncmp(op[1],"poweroff",MAX_PATH) == 0) {	/* power off */
				PowerButtonMode=POWERBUTTON_OFF;		
			}
			else if(strncmp(op[1],"sleep",MAX_PATH) == 0) {	/* sleep */
				PowerButtonMode=POWERBUTTON_SLEEP;		
			}
			else if(strncmp(op[1],"reboot",MAX_PATH) == 0) {	/* reboot */
				PowerButtonMode=POWERBUTTON_REBOOT;		
			}
			else {
				kprintf_direct(ACPIConfigInvalidValue,op[0],op[1]);
			}
		}
	}
}

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

memcpy(rsdp,rsdp_ptr,sizeof(RSDP));	/* copy RSDP */

/* get RSDT */

if(rsdp.revision == 0) {		/* ACPI 1.0 */
	memcpy(rsdt,rsdp.rsdt_address+KERNEL_HIGH,sizeof(RSDT));

	rsdt.rsdt_entries=kernelalloc(rsdt.length);		/* allocate buffer for rsdt entries */

	if(rsdt.rsdt_entries == NULL) {
		kprintf_direct("acpi: Can't allocate memory for RSDT\n");
		return(-1);
	}

	memcpy(rsdt.rsdt_entries,rsdt+sizeof(RSDT),rsdt.length-sizeof(RSDT));		/* copy rsdt to struct */
	
	type=32;			/* 32-bit RSDR */
	length=rsdt.length/4;
	tableptr=rsdt.rsdt_entries;	/* point to RSDT entries */
}
else
{
	memcpy(xsdt,rsdp.xsdt_address+KERNEL_HIGH,xsdt.length-sizeof(XSDT));

	xsdt.xsdt_entries=kernelalloc(xsdt.length);		/* allocate buffer for XSDT entries */

	if(xsdt.xsdt_entries == NULL) {
		kprintf_direct("acpi: Can't allocate memory for XSDT\n");
		return(-1);
	}

	type=64;			/* 64-bit XSDT */
	length=xsdt.length/8;
	tableptr=xsdt.xsdt_entries;	/* point to XSDT entries */
}

/* Get copies of tables */

for(count=0;count < length;count++) {
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
	else
	{
		kprintf_direct("acpi: Unknown table %s\n",tableptr->signature);
		return(-1);
	}
	tableptr++;
}

set_interrupt(fadt.SCI_Interrupt,&sci_interrupt_handler);		/* set SCI interrupt handler */

if(fadt.smi_command != 0) {		/* if ACPI not enabled */
	  outb(fadt.smi_command,fadt.acpi_enable);	/* enable ACPI */

	  while (inw(fadt->pm1a_control_block) & 1 == 0) ;; /* wait for ACPI to enable */
}

if(dsdt.amlptr != NULL) parse_aml(dsdt.amlptr);		/* parse DSDT AML */
if(ssdt.amlptr != NULL) parse_aml(ssdt.amlptr);		/* parse SSDT AML */

return(0);
}

/*
* Power off
*
* In: Nothing
*
* Returns: -1 on error, 0 on success
*
*/
size_t ACPIPowerOff(void) {
shutdown(_SHUTDOWN);
}

/*
* Restart
*
* In: Nothing
*
* Returns: -1 on error, 0 on success
*
*/
size_t ACPIRestart(void) {
shutdown(_RESET);
}

/*
* Sleep
*
* In: Nothing
*
* Returns: -1 on error, 0 on success
*
*/
size_t ACPISleep(void) {
}

/*
* Get RSDP table pointer
*
* In: Nothing
*
*  Returns: Pointer to RSDP table
*
*/
RDSP *ACPIGetRSDP(void) {
return(&rsdp);
}

/*
* Get RSDT table pointer
*
* In: Nothing
*
*  Returns: Pointer to RSDT table
*
*/
RDST *ACPIGetRSDT(void) {
return(&rsdt);
}

/*
* Get XSDT table pointer
*
* In: Nothing
*
*  Returns: Pointer to XSDT table
*
*/

XSDT *ACPIGetXSDT(void) {
return(&xsdt);
}

/*
* Get FACP table pointer
*
* In: Nothing
*
*  Returns: Pointer to FACP table
*
*/

FACP *ACPIGetFACP(void) {
return(facp);
}

/*
* Get MADT table pointer
*
* In: Nothing
*
*  Returns: Pointer to MADT table
*
*/

MADT *ACPIGetMADT(void) {
return(madt);
}

