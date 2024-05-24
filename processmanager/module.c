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

/*
 * Kernel module loader
 */

#include <stdint.h>
#include "../processmanager/mutex.h"
#include "../devicemanager/device.h"
#include "../filemanager/vfs.h"
#include "../header/errors.h"
#include "../header/bootinfo.h"
#include "../header/kernelhigh.h"
#include "../header/debug.h"
#include "module.h"
#include <elf.h>

size_t load_kernel_module(char *filename,char *argsx);
size_t getnameofsymbol(char *strtab,char *sectionheader_strptr,char *buf,Elf32_Sym *symtab,Elf32_Shdr *sectionptr,size_t which,char *name);
size_t getkernelsymbol(char *name);
size_t add_external_module_symbol(char *name,size_t val);
size_t get_external_module_symbol(char *name);

SYMBOL_TABLE_ENTRY *externalmodulesymbols=NULL;
KERNELMODULE *kernelmodules=NULL;

/*
 * Load and execute kernel module
 *
 * In:	filename	Filename of kernel module
 *	argsx		Arguments
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t load_kernel_module(char *filename,char *argsx) {
size_t handle;
char *fullname[MAX_PATH];
char *name[MAX_PATH];
char c;
size_t count;
char *buf;
char *bufptr;
Elf32_Ehdr *elf_header;
Elf32_Shdr *shptr;
Elf32_Shdr *rel_shptr;
Elf32_Rel *relptr;
Elf32_Rela *relptra;
size_t whichsym;
size_t symtype;
Elf32_Sym *symtab=NULL;
Elf32_Sym *symptr=NULL;
char *strtab;
char *strptr;
char *entryptr;
size_t addr;
size_t *ref;
void *(*entry)(char *);
size_t numberofrelocentries;
size_t symval;
uint32_t codestart;
char *sectionheader_strptr;
size_t reloc_count;
char *rodata;
char *data;
char *next_free_address_data;
char *next_free_address_rodata;
KERNELMODULE *kernelmodulenext;
KERNELMODULE *kernelmodulelast;

disablemultitasking();

getfullpath(filename,fullname);

/* check if module is loaded */

kernelmodulenext=kernelmodules;

while(kernelmodulenext != NULL) {
	kernelmodulelast=kernelmodulenext;

	if(strcmpi(kernelmodulenext->filename,fullname) == 0) {		/* found module name */
		enablemultitasking();

		setlasterror(MODULE_ALREADY_LOADED);
		return(-1);
	}

	kernelmodulenext=kernelmodulenext->next;
}

handle=open(fullname,O_RDONLY);		/* open file */
if(handle == -1) {
	enablemultitasking();
	return(-1);		/* can't open */
}

buf=kernelalloc(getfilesize(handle));
if(buf == NULL) {
	close(handle);	

	enablemultitasking();
	return(-1);
}

if(read(handle,buf,getfilesize(handle)) == -1) {			/* read module into buffer */
	close(handle);

	kernelfree(buf);

	enablemultitasking();	
	return(-1);
}

close(handle);

elf_header=buf;

if(elf_header->e_ident[0] != 0x7F && elf_header->e_ident[1] != 0x45 && elf_header->e_ident[2] != 0x4C && elf_header->e_ident[3] != 0x46) {	/* not elf */

	kprintf_direct("kernel: Module is not valid ELF object file\n");

	setlasterror(INVALID_MODULE);
	kernelfree(buf);

	enablemultitasking();
	return(-1);
}

if((elf_header->e_type != ET_REL) || (elf_header->e_shnum == 0)) {	
	kprintf_direct("kernel: Module is not relocatable ELF object file\n");

	setlasterror(INVALID_MODULE);
	kernelfree(buf);

	enablemultitasking();
	return(-1);
}

/* find the location of the section header string table, it will be used to find the string table */

shptr=(size_t) buf+elf_header->e_shoff+(elf_header->e_shstrndx*sizeof(Elf32_Shdr));	/* find string section in table */
sectionheader_strptr=(buf+shptr->sh_offset)+1;		/* point to section header string table */

/* find the symbol table,string table, text, data and rodata sections from section header table */

shptr=buf+elf_header->e_shoff;

for(count=0;count < elf_header->e_shnum;count++) {
	if(shptr->sh_type == SHT_SYMTAB && symtab == NULL) symtab=buf+shptr->sh_offset;

	if(shptr->sh_type == SHT_STRTAB) {		/* string table */
		bufptr=sectionheader_strptr+shptr->sh_name;	/* point to section header string table */

		if(strcmp(bufptr,"strtab") == 0) strtab=buf+shptr->sh_offset+1;		/* string table */
 	}

	bufptr=sectionheader_strptr+shptr->sh_name;	/* point to section header string table */

	if(strcmp(bufptr,"text") == 0) {		/* found code section */ 
		codestart=buf+shptr->sh_offset;
	}

	if(strcmp(bufptr,"rodata") == 0) rodata=buf+shptr->sh_offset;	/* found rodata section */ 

	if(strcmp(bufptr,"data") == 0) data=buf+shptr->sh_offset;	/* found data section */ 
		
	shptr++;
}


/* check if symbol and string table present */

if(symtab == NULL) {
	kprintf_direct("kernel: Module has no symbol section(s)\n");

	setlasterror(INVALID_MODULE);
	kernelfree(buf);

	enablemultitasking();
	return(-1); 
}

if(strtab == NULL) {
	kprintf_direct("kernel: Module has no string section(s)\n");

	setlasterror(INVALID_MODULE);
	kernelfree(buf);

	enablemultitasking();
	return(-1); 
}

next_free_address_data=data;			/* get next free pointers */
next_free_address_rodata=rodata;

shptr=buf+elf_header->e_shoff;

/* Relocate elf references

	The relocation is done by:
	  Searching through section table to find SHT_REL or SHL_RELA sections 
	  Processing each of the relocation entries in that section. The entries are relative to the start of the section.
	  The section is referenced in the relocation by adding the relocation address to the start of the entry.		
*/

for(count=0;count<elf_header->e_shnum;count++) {

	if((shptr->sh_type == SHT_REL) || (shptr->sh_type == SHT_RELA)) {					/* found section */
		/* perform relocations using relocation table in section */
 
		numberofrelocentries=shptr->sh_size/sizeof(Elf32_Rel);

		if(shptr->sh_type == SHT_REL) {
			relptr=buf+shptr->sh_offset;
		}
		else if(shptr->sh_type == SHT_RELA) {
			relptra=buf+shptr->sh_offset;
		}

		for(reloc_count=0;reloc_count<numberofrelocentries;reloc_count++) {

				rel_shptr=(buf+elf_header->e_shoff)+(shptr->sh_info*sizeof(Elf32_Shdr));
				
				if(shptr->sh_type == SHT_REL) {			
			  		whichsym=relptr->r_info >> 8;			/* which symbol */
					symtype=relptr->r_info & 0xff;		/* symbol type */

					ref=(buf+rel_shptr->sh_offset)+relptr->r_offset;
				}
				else if(shptr->sh_type == SHT_RELA) { 

			  		whichsym=relptra->r_info >> 8;			/* which symbol */
					symtype=relptra->r_info & 0xff;		/* symbol type */

		  			ref=(buf+rel_shptr->sh_offset)+relptra->r_offset;
  				}
					
				/* Get the value to place at the location ref into symval */

				symptr=(size_t) symtab+(whichsym*sizeof(Elf32_Sym));

				/* get symbol value */

				getnameofsymbol(strtab,sectionheader_strptr,buf,symtab,\
					       (buf+elf_header->e_shoff)+(symptr->st_shndx*sizeof(Elf32_Shdr)),whichsym,name);	/* get name of symbol */				


				if(symptr->st_shndx == SHN_UNDEF) {	/* external symbol */							
					symval=getkernelsymbol(name);

					if(symval == -1) {	/* if it's not a kernel symbol, check if it is a external module symbol */
				 		symval=get_external_module_symbol(name);
						if(symval == -1) symval=symptr->st_value;
					}
		
					add_external_module_symbol(name,symval);		/* add external symbol to list */
				}
				else
				{								
				
					if((symtype == STT_FUNC) || ((symptr->st_info  & 0xf) == STT_FUNC)) {	/* code symbol */						
						symval=codestart+symptr->st_value;							
					}
					else if((symtype == STT_OBJECT) || (symtype == STT_COMMON) || ((symptr->st_info  & 0xf) == STT_FUNC)) {		/* data symbol */
						if(strcmp(name,"rodata") == 0) {
							
							if(symptr->st_value == 0) {
								next_free_address_rodata += symptr->st_size;
								symval=next_free_address_rodata;

								if(add_external_module_symbol(name,symval) == -1) next_free_address_rodata -= symptr->st_size;
							}
							else
							{
								symval=rodata+symptr->st_value;			
							}
						
							next_free_address_data += symptr->st_size;
						}
						else
						{	
							if(symptr->st_value == 0) {
								symval=next_free_address_data;

								if(add_external_module_symbol(name,symval) == -1) next_free_address_rodata -= symptr->st_size;
							}
							else
							{
								symval=data+symptr->st_value;			
							}
						}

					}	

					if(shptr->sh_type == SHT_RELA) symval += relptra->r_addend;
				}				

				/* update reference in section */

				if(symtype == R_386_NONE) {
				;;
				}
				else if(symtype == R_386_32) {
					*ref=DO_386_32(symval,*ref);
				}
			 	else if(symtype == R_386_PC32) {

					if(shptr->sh_type == SHT_REL) {
						*ref=DO_386_PC32(symval,*ref,(size_t) ref);
					}
					else if(shptr->sh_type == SHT_RELA) {
	  					*ref=DO_386_PC32(symval,*ref,symval);
					}
				}
				else
				{
					kprintf_direct("kernel: unknown relocation type %d in module %s\n",symtype,filename);
			
					enablemultitasking();

					setlasterror(INVALID_MODULE);
					return(-1);
				}

				if(shptr->sh_type == SHT_REL) {	  				
					relptr++;
				}

				else if(shptr->sh_type == SHT_RELA) {
					relptra++;
				}

		}		
 	}
		
	shptr++;
}

/* add new module */

if(kernelmodules == NULL) {			/* first in list */
	kernelmodules=kernelalloc(sizeof(KERNELMODULE));
	if(kernelmodules == NULL) {
		close(handle);
		kernelfree(buf);
		enablemultitasking();	
		return(-1);
	}

	kernelmodulelast=kernelmodules;
}
else
{
	kernelmodulelast->next=kernelalloc(sizeof(KERNELMODULE));
	if(kernelmodulelast->next == NULL) {
		close(handle);
		kernelfree(buf);
		enablemultitasking();	
		return(-1);
	}

	kernelmodulelast=kernelmodulelast->next;
}

strcpy(kernelmodulelast->filename,fullname);

kernelmodulelast->next=NULL;

/* call module entry point */

entry=codestart;
enablemultitasking();
return(entry(argsx));
}

/*
 * Get name of symbol from kernel module
 *
 * In:  strtab		     Pointer to ELF string table
	sectionheader_strptr Section header string table
	buf		     Pointer to start of module
	symtab		     Symbol table
	sectionptr	     Section header
	which		     Symbol index
	name		     Symbol name buffer
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t getnameofsymbol(char *strtab,char *sectionheader_strptr,char *buf,Elf32_Sym *symtab,Elf32_Shdr *sectionptr,size_t which,char *name) {
size_t count;
char *strptr=strtab;
Elf32_Sym *symptr;
char *shptr;

symptr=(size_t) symtab+(sizeof(Elf32_Sym)*which);		/* point to symbol table entry */

strcpy(name,(strtab+symptr->st_name)-1);

if(strcmp(name,"") == 0) {		/* not a symbol table entry */
	shptr=sectionheader_strptr;
	shptr += sectionptr->sh_name;	
	
	strcpy(name,shptr);
	return(0);
}
	
return(0);
}


/*
 * Get kernel symbol
 *
 * In: char *name	Name of kernel symbol
 *
 * Returns symbol value on success, -1 otherwise
 * 
 */

size_t getkernelsymbol(char *name) {
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;		/* point to boot information */
char *symptr;
size_t count;
size_t *valptr;

symptr=bootinfo->symbol_start;
symptr += KERNEL_HIGH;		/* point to symbol table */

/* loop through symbols and find the kernel symbol */

for(count=0;count<bootinfo->number_of_symbols;count++) {
	if(strcmp(symptr,name) == 0) {				/* symbol found */

		symptr += strlen(name)+1;		/* skip over name */
  
		valptr=symptr;			/* cast pointer to size_t */
		return(*valptr);			/* return value */
 	}

 symptr += strlen(symptr)+sizeof(size_t)+1;		/* skip over name and symbol */
}

return(-1);
}

/*
 * Add external module symbol
 *
 * In: char *name	Name of symbol
 *
 * Returns symbol value on success, -1 otherwise
 * 
 */
size_t add_external_module_symbol(char *name,size_t val) {
SYMBOL_TABLE_ENTRY *next;
SYMBOL_TABLE_ENTRY *last;

if(externalmodulesymbols == NULL) {		/* first in list */
	externalmodulesymbols=kernelalloc(sizeof(SYMBOL_TABLE_ENTRY));
	if(externalmodulesymbols == NULL) return(-1);

	next=externalmodulesymbols;
}
else
{
	next=externalmodulesymbols;

	while(next != NULL) {
		last=next;

		if(strcmp(next->name,name) == 0) return(-1);		/* symbol exists */
		next=next->next;
	}

	last->next=kernelalloc(sizeof(SYMBOL_TABLE_ENTRY));
 	if(last->next == NULL) return(-1);

	next=last->next;
}

/* add details to end */

strcpy(next->name,name);
next->address=val;
next->next=NULL;

//kprintf_direct("added symbol %s\n",name);

setlasterror(NO_ERROR);
return(0);
}

/*
 * Get external module symbol
 *
 * In: char *name	Name of kernel symbol
 *
 * Returns symbol value on success, -1 otherwise
 * 
 */
size_t get_external_module_symbol(char *name) {
SYMBOL_TABLE_ENTRY *next;

next=externalmodulesymbols;

while(next != NULL) {
	if(strcmp(next->name,name) == 0) return(next->address);

	next=next->next;
}

return(-1);
}

/*
 * Unload kernel module
 *
 * In: filename	Filename
 *
 * Returns 0 on success, -1 otherwise
 * 
 */
size_t unload_kernel_module(char *filename) {
KERNELMODULE *kernelmodulenext;
KERNELMODULE *kernelmodulelast;
char *fullname[MAX_PATH];

disablemultitasking();

getfullpath(filename,fullname);

kernelmodulenext=kernelmodules;

while(kernelmodulenext != NULL) {
	kernelmodulelast=kernelmodulenext;

	if(strcmpi(kernelmodulenext->filename,fullname) == 0) {		/* found module name */
		kernelmodulelast->next=kernelmodulenext->next;		/* remove from list */

		kernelfree(kernelmodulenext);

		enablemultitasking();

		setlasterror(NO_ERROR);
		return(0);
	}

	kernelmodulenext=kernelmodulenext->next;
}

enablemultitasking();

setlasterror(INVALID_MODULE);
return(-1);
}
