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
#include "mutex.h"
#include "vfs.h"
#include "errors.h"
#include "bootinfo.h"
#include "kernelhigh.h"
#include "debug.h"
#include "module.h"
#include "memorymanager.h"
#include "string.h"

#include <elf.h>

size_t load_kernel_module(char *filename,char *argsx);
size_t getnameofsymbol(char *strtab,char *sectionheader_strptr,char *buf,Elf32_Sym *symtab,Elf32_Shdr *sectionptr,size_t which,char *name);
size_t getkernelsymbol(char *name);
size_t add_external_module_symbol(char *name,size_t val);
size_t get_external_module_symbol(char *name);

SYMBOL_TABLE_ENTRY *externalmodulesymbols=NULL;
KERNELMODULE *kernelmodules=NULL;
KERNELMODULE *kernel_module_end=NULL;

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
size_t count;
char *buf;
char *bufptr;
Elf32_Ehdr *elf_header;
Elf32_Shdr *shptr;
Elf32_Shdr *rel_shptr;
Elf32_Shdr *symsection;
Elf32_Rel *relptr;
Elf32_Rela *relptra;
size_t whichsym;
size_t symtype;
Elf32_Sym *symtab=NULL;
Elf32_Sym *symptr=NULL;
char *strtab;
char *strptr;
size_t *ref;
void *(*entry)(char *);
size_t symval;
char *codestart;
char *sectionheader_strptr;
size_t reloc_count;
char *rodata;
char *data;
char *next_free_address_rodata;
char *next_free_address_data;
KERNELMODULE *kernelmodulenext;
KERNELMODULE *kernelmodulelast;
char *commondataptr;
size_t common_data_section_size;

disablemultitasking();

getfullpath(filename,fullname);

/* check if module is loaded */

kernelmodulenext=kernelmodules;

while(kernelmodulenext != NULL) {

	if(strncmpi(kernelmodulenext->filename,fullname,MAX_PATH) == 0) {		/* found module name */
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

buf=kernelalloc(getfilesize(handle));		/* allocate memory for module */
if(buf == NULL) {
	close(handle);	

	enablemultitasking();
	return(-1);
}

/* The entire module file is read into memory to allow fast cross-referencing */

if(read(handle,buf,getfilesize(handle)) == -1) {			/* read module into buffer */
	close(handle);

	kernelfree(buf);

	enablemultitasking();	
	return(-1);
}

close(handle);

elf_header=buf;

if(elf_header->e_ident[0] != 0x7F && elf_header->e_ident[1] != 0x45 && elf_header->e_ident[2] != 0x4C && elf_header->e_ident[3] != 0x46) {	/* not elf */

	kprintf_direct("module loader: Module %s is not a valid ELF object file\n",fullname);

	setlasterror(INVALID_EXECUTABLE);
	kernelfree(buf);

	enablemultitasking();
	return(-1);
}

if(elf_header->e_type != ET_REL) {	
	kprintf_direct("module loader: Module %s is not relocatable an ELF object file\n",fullname);

	setlasterror(INVALID_EXECUTABLE);
	kernelfree(buf);

	enablemultitasking();
	return(-1);
}

if(elf_header->e_shnum == 0) {	
	kprintf_direct("module loader: No section headers found in module %s\n",fullname);

	setlasterror(INVALID_EXECUTABLE);
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

		if(strncmp(bufptr,"strtab",MAX_PATH) == 0) strtab=buf+shptr->sh_offset+1;		/* string table */
 	}

	bufptr=sectionheader_strptr+shptr->sh_name;	/* point to section header string table */

	if(strncmp(bufptr,"text",MAX_PATH) == 0) codestart=buf+shptr->sh_offset;	/* found code section */ 	

	if(strncmp(bufptr,"rodata",MAX_PATH) == 0) {
		rodata=buf+shptr->sh_offset;	/* found rodata section */ 		
		next_free_address_rodata=rodata;

		if(shptr->sh_size > common_data_section_size) common_data_section_size=shptr->sh_size;
	}

	if(strncmp(bufptr,"data",MAX_PATH) == 0) {
		data=buf+shptr->sh_offset;		/* found data section */ 
		next_free_address_data=data;		/* get next free pointers */

		if(shptr->sh_size > common_data_section_size) common_data_section_size=shptr->sh_size;
	}

	shptr++;
}

/* check if symbol and string table present */

if(symtab == NULL) {
	kprintf_direct("module loader: Module has no symbol section(s)\n");

	setlasterror(INVALID_EXECUTABLE);
	kernelfree(buf);
	kernelfree(kernel_module_end->commondata);

	enablemultitasking();
	return(-1); 
}

if(strtab == NULL) {
	kprintf_direct("module loader: Module has no string section(s)\n");
	kernelfree(buf);
	kernelfree(kernel_module_end->commondata);

	setlasterror(INVALID_EXECUTABLE);

	enablemultitasking();
	return(-1); 
}

/* add new module */

if(kernelmodules == NULL) {			/* first in list */
	kernelmodules=kernelalloc(sizeof(KERNELMODULE));
	if(kernelmodules == NULL) {
		kernelfree(buf);
		kernelfree(kernel_module_end->commondata);

		enablemultitasking();	
		return(-1);
	}

	kernel_module_end=kernelmodules;
}
else
{
	kernel_module_end->next=kernelalloc(sizeof(KERNELMODULE));
	if(kernel_module_end->next == NULL) {
		kernelfree(buf);
		kernelfree(kernel_module_end->commondata);

		enablemultitasking();	
		return(-1);
	}

	kernel_module_end=kernel_module_end->next;
}

strncpy(kernel_module_end->filename,fullname,MAX_PATH);
kernel_module_end->next=NULL;

kernel_module_end->commondata=kernelalloc(common_data_section_size);
if(kernel_module_end->commondata == NULL) {
	kernelfree(buf);
	kernelfree(kernel_module_end->commondata);
	kernelfree(kernelmodulelast->next);
	
	setlasterror(INVALID_EXECUTABLE);
	enablemultitasking();
	return(-1);
}

commondataptr=kernel_module_end->commondata;

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
 
		if(shptr->sh_type == SHT_REL) {
			relptr=buf+shptr->sh_offset;
		}
		else if(shptr->sh_type == SHT_RELA) {
			relptra=buf+shptr->sh_offset;
		}
		
		/* perform relocations for each entry */

		for(reloc_count=0;reloc_count < (shptr->sh_size/sizeof(Elf32_Rel));reloc_count++) {

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
		
					add_external_module_symbol(name,symval+KERNEL_HIGH);		/* add external symbol to list */
				}
				else
				{								
					if((symtype == STT_FUNC) || ((symptr->st_info  & 0xf) == STT_FUNC) || (strncmpi(name,"text",MAX_PATH) == 0)) {	/* code symbol */						
						if(strncmp(name,"rodata",MAX_PATH) == 0) {
							symval=rodata+symptr->st_value;			
						}
						else if(strncmp(name,"data",MAX_PATH) == 0) {
							symval=data+symptr->st_value;			
						}
						else
						{
							symval=codestart+symptr->st_value;							
						}
					}
					else if((symtype == STT_OBJECT) || (symptr->st_shndx == SHN_COMMON) || ((symptr->st_info  & 0xf) == STT_FUNC)) {		/* data symbol */
						if(strncmp(name,"rodata",MAX_PATH) == 0) {
							if(symptr->st_value == 0) {
								next_free_address_rodata += symptr->st_size;
								symval=next_free_address_rodata;

								if(add_external_module_symbol(name,symval) == -1) next_free_address_rodata -= symptr->st_size;

								next_free_address_rodata += symptr->st_size;

							}
							else
							{
								symval=rodata+symptr->st_value;			
							}
						
						}
						else
						{	
							if(symptr->st_shndx == SHN_COMMON) {
								symval=get_external_module_symbol(name);
								if(symval == -1) {
									next_free_address_data += symptr->st_size;
									symval=next_free_address_data;
		
									add_external_module_symbol(name,symval);
								}
							}
							else
							{
								/* point to section name for symbol */
								symsection=(buf+elf_header->e_shoff)+(symptr->st_shndx*sizeof(Elf32_Shdr));

								bufptr=sectionheader_strptr+symsection->sh_name;			

								if(strncmp(bufptr,"data",MAX_PATH) == 0) {									
									symval=get_external_module_symbol(name);

									if(symval == -1) {
										symval=data+symptr->st_value;			
											
										add_external_module_symbol(name,symval);	
									}
								}
								else if(strncmp(bufptr,"bss",MAX_PATH) == 0) {
									symval=get_external_module_symbol(name);
									if(symval == -1) {
										symval=commondataptr+symptr->st_value;

										add_external_module_symbol(name,symval);
									}	
								}
							}
						}
				
					
					}	
							
				}

				/* update reference in section */
			
				if(symtype == R_386_NONE) {
				;
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
					kprintf_direct("module loader: unknown relocation type %d in module %s\n",symtype,filename);
			
					kernelfree(buf);
					kernelfree(kernel_module_end->commondata);
					kernelfree(kernelmodulelast->next);

					enablemultitasking();

					setlasterror(INVALID_EXECUTABLE);
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

//if(strncmpi(filename,"Z:\\SERIALIO.O",MAX_PATH) == 0) asm("xchg %bx,%bx");

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

strncpy(name,(strtab+symptr->st_name)-1,MAX_PATH);

if(strncmp(name,"",MAX_PATH) == 0) {		/* not a symbol table entry */
	shptr=sectionheader_strptr;
	shptr += sectionptr->sh_name;	
	
	strncpy(name,shptr,MAX_PATH);
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
	if(strncmp(symptr,name,MAX_PATH) == 0) {				/* symbol found */
		symptr += (strlen(name)+1);		/* skip over name */
  
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

		if(strncmp(next->name,name,MAX_PATH) == 0) return(-1);		/* symbol exists */
		next=next->next;
	}

	last->next=kernelalloc(sizeof(SYMBOL_TABLE_ENTRY));
 	if(last->next == NULL) return(-1);

	next=last->next;
}

/* add details to end */

strncpy(next->name,name,MAX_PATH);
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
	if(strncmp(next->name,name,MAX_PATH) == 0) return(next->address);

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

	if(strncmpi(kernelmodulenext->filename,fullname,MAX_PATH) == 0) {		/* found module name */
		kernelfree(kernel_module_end->commondata);
		kernelmodulelast->next=kernelmodulenext->next;		/* remove from list */

		kernelfree(kernelmodulenext);
		setlasterror(NO_ERROR);
		return(0);
	}

	kernelmodulenext=kernelmodulenext->next;
}

enablemultitasking();

setlasterror(FILE_NOT_FOUND);
return(-1);
}
