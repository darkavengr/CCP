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
#include "device.h"
#include "vfs.h"
#include "errors.h"
#include "bootinfo.h"
#include "kernelhigh.h"
#include "debug.h"
#include "module.h"
#include "memorymanager.h"
#include "string.h"
#include <elf.h>

SYMBOL_TABLE_ENTRY *externalmodulesymbols=NULL;
KERNELMODULE *kernelmodules=NULL;
KERNELMODULE *kernelmodules_end=NULL;
KERNELMODULE *kernelmodulelast;

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
char *modulestartaddress;
char *bufptr;
Elf64_Ehdr *elf_header64;
Elf32_Ehdr *elf_header32;
Elf32_Shdr *shptr32;
Elf32_Shdr *rel_shptr32;
Elf32_Shdr *symsection32;
Elf32_Rel *relptr32;
Elf32_Rela *relptra32;
Elf32_Sym *symtab32=NULL;
Elf32_Sym *symptr32=NULL;
Elf64_Shdr *shptr64;
Elf64_Shdr *rel_shptr64;
Elf64_Shdr *symsection64;
Elf64_Rel *relptr64;
Elf64_Rela *relptra64;
Elf64_Sym *symtab64=NULL;
Elf64_Sym *symptr64=NULL;
size_t whichsym;
size_t symtype;
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
char *commondataptr;
size_t common_data_section_size;
size_t elf_type;
size_t number_of_section_headers;
size_t number_of_reloc_headers;
size_t sectiontype;
size_t stval;
size_t stsize;
KERNELMODULE kernelmodule;
size_t modulesize;

getfullpath(filename,fullname);

if(IsModuleLoaded(filename) == 0) {		/* check if module is loaded */
	setlasterror(MODULE_ALREADY_LOADED);
	return(-1);
}

handle=open(fullname,O_RDONLY | O_EXCLUSIVE);		/* open file */
if(handle == -1) {
	enablemultitasking();
	return(-1);		/* can't open */
}

modulesize=getfilesize(handle);			/* get module size */

modulestartaddress=kernelalloc(modulesize);		/* allocate memory for module */
if(modulestartaddress == NULL) {
	close(handle);	

	enablemultitasking();
	return(-1);
}

/* The entire module file is read into memory to allow fast cross-referencing */

if(read(handle,modulestartaddress,getfilesize(handle)) == -1) {			/* read module into buffer */
	close(handle);

	kernelfree(modulestartaddress);

	enablemultitasking();	
	return(-1);
}

close(handle);

elf_header32=modulestartaddress;
elf_header64=modulestartaddress;

if(elf_header32->e_ident[0] != 0x7F && 
   elf_header32->e_ident[1] != 0x45 &&
   elf_header32->e_ident[2] != 0x4C &&
   elf_header32->e_ident[3] != 0x46) {	/* not an ELF object file */

	kprintf_direct("module loader: Module %s is not a valid ELF object file\n",fullname);

	setlasterror(INVALID_EXECUTABLE);
	kernelfree(modulestartaddress);

	enablemultitasking();
	return(-1);
}

elf_type=elf_header32->e_ident[4];		/* get ELF type; 32 or 64-bit */

if( ((elf_type == ELFCLASS32) && (elf_header32->e_type != ET_REL)) || 
    ((elf_type == ELFCLASS64) && (elf_header64->e_type != ET_REL)) ) {	
	kprintf_direct("module loader: Module %s is not relocatable an ELF object file\n",fullname);

	setlasterror(INVALID_EXECUTABLE);
	kernelfree(modulestartaddress);

	enablemultitasking();
	return(-1);
}

if( ((elf_type == ELFCLASS32) && (elf_header32->e_shnum == 0)) ||
    ((elf_type == ELFCLASS64) && (elf_header64->e_shnum == 0))) {
	kprintf_direct("module loader: No section headers found in module %s\n",fullname);

	setlasterror(INVALID_EXECUTABLE);
	kernelfree(modulestartaddress);

	enablemultitasking();
	return(-1);
}

if((elf_type == ELFCLASS32) && (sizeof(size_t) != 4)) {
	kprintf_direct("module loader: Can't load 32-bit module on 64-bit systems\n");

	setlasterror(INVALID_EXECUTABLE);
	kernelfree(modulestartaddress);

	enablemultitasking();
	return(-1);
}

if((elf_type == ELFCLASS64) && (sizeof(size_t) != 8)) {
	kprintf_direct("module loader: Can't load 32-bit module on 64-bit systems\n");

	setlasterror(INVALID_EXECUTABLE);
	kernelfree(modulestartaddress);

	enablemultitasking();
	return(-1);
}

/* find the location of the section header string table, it will be used to find the string table */

if(elf_type == ELFCLASS32) {
	shptr32=(size_t) modulestartaddress+elf_header32->e_shoff+(elf_header32->e_shstrndx*sizeof(Elf32_Shdr));	/* find string section in table */
	sectionheader_strptr=(modulestartaddress+shptr32->sh_offset)+1;		/* point to section header string table */
}
if(elf_type == ELFCLASS64) {
	shptr64=(size_t) modulestartaddress+elf_header64->e_shoff+(elf_header64->e_shstrndx*sizeof(Elf64_Shdr));	/* find string section in table */
	sectionheader_strptr=(modulestartaddress+shptr64->sh_offset)+1;		/* point to section header string table */
}

/* find the symbol table,string table, text, data and rodata sections from section header table */

if(elf_type == ELFCLASS32) {
	shptr32=modulestartaddress+elf_header32->e_shoff;

	number_of_section_headers=elf_header32->e_shnum;
}
if(elf_type == ELFCLASS64) {
	shptr64=modulestartaddress+elf_header64->e_shoff;

	number_of_section_headers=elf_header64->e_shnum;
}

for(count=0;count < number_of_section_headers;count++) {

	if(elf_type == ELFCLASS32) {
		if(shptr32->sh_type == SHT_SYMTAB && symtab32 == NULL) symtab32=modulestartaddress+shptr32->sh_offset;

		if(shptr32->sh_type == SHT_STRTAB) {		/* string table */
			bufptr=sectionheader_strptr+shptr32->sh_name;	/* point to section header string table */

			if(strncmp(bufptr,"strtab",MAX_PATH) == 0) strtab=modulestartaddress+shptr32->sh_offset+1;		/* string table */
	 	}

		bufptr=sectionheader_strptr+shptr32->sh_name;	/* point to section header string table */

		if(strncmp(bufptr,"text",MAX_PATH) == 0) codestart=modulestartaddress+shptr32->sh_offset; /* found code section */ 	

		if(strncmp(bufptr,"rodata",MAX_PATH) == 0) {
			rodata=modulestartaddress+shptr32->sh_offset;	/* found rodata section */ 		
			next_free_address_rodata=rodata;

			if(shptr32->sh_size > common_data_section_size) common_data_section_size=shptr32->sh_size;
		}

		if(strncmp(bufptr,"data",MAX_PATH) == 0) {
			data=modulestartaddress+shptr32->sh_offset;		/* found data section */ 
			next_free_address_data=data;		/* get next free pointers */

			if(shptr32->sh_size > common_data_section_size) common_data_section_size=shptr32->sh_size;
		}

		shptr32++;
	}
	else if(elf_type == ELFCLASS64) {
		if(shptr64->sh_type == SHT_SYMTAB && symtab64 == NULL) symtab64=modulestartaddress+shptr64->sh_offset;

		if(shptr64->sh_type == SHT_STRTAB) {		/* string table */
			bufptr=sectionheader_strptr+shptr64->sh_name;	/* point to section header string table */
			if(strncmp(bufptr,"strtab",MAX_PATH) == 0) strtab=modulestartaddress+shptr64->sh_offset+1;		/* string table */
	 	}

		bufptr=sectionheader_strptr+shptr64->sh_name;	/* point to section header string table */

		if(strncmp(bufptr,"text",MAX_PATH) == 0) codestart=modulestartaddress+shptr64->sh_offset;	/* found code section */ 	

		if(strncmp(bufptr,"rodata",MAX_PATH) == 0) {
			rodata=modulestartaddress+shptr64->sh_offset;	/* found rodata section */ 		
			next_free_address_rodata=rodata;

			if(shptr64->sh_size > common_data_section_size) common_data_section_size=shptr64->sh_size;
		}

		if(strncmp(bufptr,"data",MAX_PATH) == 0) {
			data=modulestartaddress+shptr64->sh_offset;		/* found data section */ 
			next_free_address_data=data;		/* get next free pointers */

			if(shptr64->sh_size > common_data_section_size) common_data_section_size=shptr64->sh_size;
		}

		shptr64++;
	}

}

/* check if symbol and string table present */

if(((elf_type == ELFCLASS32) && (symtab32 == NULL)) || ((elf_type == ELFCLASS64) && (symtab64 == NULL))  ) {
	kprintf_direct("module loader: Module has no symbol section(s)\n");

	setlasterror(INVALID_EXECUTABLE);
	kernelfree(modulestartaddress);
	kernelfree(kernelmodules_end->commondata);

	enablemultitasking();
	return(-1); 
}

if(strtab == NULL) {
	kprintf_direct("module loader: Module has no string section(s)\n");
	kernelfree(modulestartaddress);
	kernelfree(kernelmodules_end->commondata);

	setlasterror(INVALID_EXECUTABLE);

	enablemultitasking();
	return(-1); 
}

/* add new module */

if(kernelmodules == NULL) {			/* first in list */
	kernelmodules=kernelalloc(sizeof(KERNELMODULE));
	if(kernelmodules == NULL) {
		kernelfree(modulestartaddress);
		kernelfree(kernelmodules_end->commondata);

		enablemultitasking();	
		return(-1);
	}

	kernelmodules_end=kernelmodules;
}
else
{
	kernelmodules_end->next=kernelalloc(sizeof(KERNELMODULE));
	if(kernelmodules_end->next == NULL) {
		kernelfree(modulestartaddress);
		kernelfree(kernelmodules_end->commondata);

		enablemultitasking();	
		return(-1);
	}

	kernelmodules_end=kernelmodules_end->next;
}

strncpy(kernelmodules_end->filename,fullname,MAX_PATH);

kernelmodules_end->next=NULL;

kernelmodules_end->commondata=kernelalloc(common_data_section_size);		/* allocate buffer for data section */
if(kernelmodules_end->commondata == NULL) {
	kernelfree(modulestartaddress);
	kernelfree(kernelmodules_end->commondata);
	kernelfree(kernelmodulelast->next);
	
	enablemultitasking();
	return(-1);
}

commondataptr=kernelmodules_end->commondata;

/* Relocate elf references

	The relocation is done by:
	  Searching through section table to find SHT_REL or SHL_RELA sections 
	  Processing each of the relocation entries in that section. The entries are relative to the start of the section.
	  The section is referenced in0 the relocation by adding the relocation address to the start of the entry.		
*/

/* find string section in table */
if(elf_type == ELFCLASS32) {
	strptr=(size_t) modulestartaddress+elf_header32->e_shoff+(elf_header32->e_shstrndx*sizeof(Elf32_Shdr));

	shptr32=modulestartaddress+elf_header32->e_shoff;
}
if(elf_type == ELFCLASS64) {
	strptr=(size_t) modulestartaddress+elf_header64->e_shoff+(elf_header64->e_shstrndx*sizeof(Elf64_Shdr));

	shptr64=modulestartaddress+elf_header64->e_shoff;
}

for(count=0;count<(number_of_section_headers-1);count++) {

	if(elf_type == ELFCLASS32) {
		sectiontype=shptr32->sh_type;			/* get section type */
	}
	else if(elf_type == ELFCLASS64) {
		sectiontype=shptr64->sh_type;			/* get section type */
	}

	if(  (elf_type == ELFCLASS32 && shptr32->sh_type == SHT_REL) ||
	     (elf_type == ELFCLASS32 && shptr32->sh_type == SHT_RELA) ||
	     (elf_type == ELFCLASS64 && shptr64->sh_type == SHT_REL) ||
	     (elf_type == ELFCLASS64 && shptr64->sh_type == SHT_RELA)) { 
		/* perform relocations using relocation table in section */
 
		if((elf_type == ELFCLASS32) && ((shptr32->sh_type == SHT_REL) || (shptr32->sh_type == SHT_RELA))) {
			if(shptr32->sh_type == SHT_REL) {
				relptr32=modulestartaddress+shptr32->sh_offset;
			}
			else if(shptr32->sh_type == SHT_RELA) {
				relptra32=modulestartaddress+shptr32->sh_offset;
			}

			number_of_reloc_headers=(shptr32->sh_size/sizeof(Elf32_Rel));
		}
		else if((elf_type == ELFCLASS64) && ((shptr64->sh_type == SHT_REL) || (shptr64->sh_type == SHT_RELA))) {
			if(shptr64->sh_type == SHT_REL) {
				relptr64=modulestartaddress+shptr64->sh_offset;
			}
			else if(shptr64->sh_type == SHT_RELA) {
				relptra64=modulestartaddress+shptr64->sh_offset;
			}

			number_of_reloc_headers=(shptr64->sh_size/sizeof(Elf64_Rel));
		}

		/* perform relocations for each entry */

		for(reloc_count=0;reloc_count < number_of_reloc_headers;reloc_count++) {

			if((elf_type == ELFCLASS32) && ((shptr32->sh_type == SHT_REL) || (shptr32->sh_type == SHT_RELA))) {
				rel_shptr32=(modulestartaddress+elf_header32->e_shoff)+(shptr32->sh_info*sizeof(Elf32_Shdr));
				
				if(shptr32->sh_type == SHT_REL) {
			  		whichsym=relptr32->r_info >> 8;			/* which symbol */
					symtype=relptr32->r_info & 0xff;		/* symbol type */

					ref=(modulestartaddress+rel_shptr32->sh_offset)+relptr32->r_offset;

				}
				else if(shptr32->sh_type == SHT_RELA) { 
			  		whichsym=relptra32->r_info >> 8;			/* which symbol */
					symtype=relptra32->r_info & 0xff;			/* symbol type */

		  			ref=(modulestartaddress+rel_shptr32->sh_offset)+relptra32->r_offset;
  				}

				symptr32=(size_t) symtab32+(whichsym*sizeof(Elf32_Sym)); /* Get the value to place at the location ref into symval */
			
				stval=symptr32->st_value;
				stsize=symptr32->st_size;

				getnameofsymbol32(strtab,sectionheader_strptr,modulestartaddress,symtab32,\
				       (modulestartaddress+elf_header32->e_shoff)+(symptr32->st_shndx*sizeof(Elf32_Shdr)),whichsym,name);	/* get name of symbol */				
			}
			else if((elf_type == ELFCLASS64) && ((shptr64->sh_type == SHT_REL) || (shptr64->sh_type == SHT_RELA))) {
				rel_shptr64=(modulestartaddress+elf_header64->e_shoff)+(shptr64->sh_info*sizeof(Elf64_Shdr));
				
				if(shptr64->sh_type == SHT_REL) {			
			  		whichsym=relptr64->r_info >> 8;			/* which symbol */
					symtype=relptr64->r_info & 0xff;		/* symbol type */

					ref=(modulestartaddress+rel_shptr64->sh_offset)+relptr64->r_offset;
				}
				else if(shptr64->sh_type == SHT_RELA) { 

			  		whichsym=relptra64->r_info >> 8;			/* which symbol */
					symtype=relptra64->r_info & 0xff;			/* symbol type */

		  			ref=(modulestartaddress+rel_shptr64->sh_offset)+relptra64->r_offset;
  				}

				symptr64=(size_t) symtab64+(whichsym*sizeof(Elf64_Sym)); /* Get the value to place at the location ref into symval */
			
				stval=symptr64->st_value;
				stsize=symptr64->st_size;

				getnameofsymbol64(strtab,sectionheader_strptr,modulestartaddress,symtab64,\
				       (modulestartaddress+elf_header64->e_shoff)+(symptr64->st_shndx*sizeof(Elf64_Shdr)),whichsym,name);	/* get name of symbol */				
			}					

			/* external symbol */							
			if( ((elf_type == ELFCLASS32) && (symptr32->st_shndx == SHN_UNDEF)) || ((elf_type == ELFCLASS64) && (symptr64->st_shndx == SHN_UNDEF))) {
				symval=getkernelsymbol(name);

				if(symval == -1) {	/* if it's not a kernel symbol, check if it is a external module symbol */
					symval=get_external_module_symbol(name);
					if(symval == -1) symval=stval;
				}
		
				add_external_module_symbol(name,symval+KERNEL_HIGH,stsize);		/* add external symbol to list */
			}
			else
			{	
				if(  (symtype == STT_FUNC) || 
				     ((elf_type == ELFCLASS32) && ((symptr32->st_info  & 0xf) == STT_FUNC)) ||
				     ((elf_type == ELFCLASS64) && ((symptr64->st_info  & 0xf) == STT_FUNC)) ||
				     (strncmpi(name,"text",MAX_PATH) == 0)) {	/* code symbol */						

					if(strncmp(name,"rodata",MAX_PATH) == 0) {
						symval=rodata+stval;
					}
					else if(strncmp(name,"data",MAX_PATH) == 0) {
						symval=data+stval;
					}
					else
					{
						symval=codestart+stval;
					}
				}
				else if((symtype == STT_OBJECT) || 
					((elf_type == ELFCLASS32) && (symptr32->st_shndx == SHN_COMMON)) ||
					((elf_type == ELFCLASS32) && ((symptr32->st_info  & 0xf) == STT_FUNC)) ||
					((elf_type == ELFCLASS64) && (symptr64->st_shndx == SHN_COMMON)) ||
					((elf_type == ELFCLASS64) && ((symptr64->st_info  & 0xf) == STT_FUNC))) { /* data symbol */

					if(strncmp(name,"rodata",MAX_PATH) == 0) {
						if(stval == 0) {
							next_free_address_rodata += stsize;
							symval=next_free_address_rodata;

							if(add_external_module_symbol(name,symval,stsize) == -1) next_free_address_rodata -= stsize;

							next_free_address_rodata += stsize;
						}
						else
						{
							symval=rodata+stval;		
						}
						
					}
					else
					{	
						if( ((elf_type == ELFCLASS32) && (symptr32->st_shndx == SHN_COMMON)) ||
						    ((elf_type == ELFCLASS64) && (symptr64->st_shndx == SHN_COMMON))) {

							symval=get_external_module_symbol(name);
							if(symval == -1) {
								next_free_address_data += stsize;
								symval=next_free_address_data;
	
								add_external_module_symbol(name,symval,stsize);
							}
						}
						else
						{
							/* point to section name for symbol */

							if(elf_type == ELFCLASS32) {
								symsection32=(modulestartaddress+elf_header32->e_shoff)+(symptr32->st_shndx*sizeof(Elf32_Shdr));	
								bufptr=sectionheader_strptr+symsection32->sh_name;			
							}
							else if(elf_type == ELFCLASS64) {
								symsection64=(modulestartaddress+elf_header64->e_shoff)+(symptr64->st_shndx*sizeof(Elf64_Shdr));	
								bufptr=sectionheader_strptr+symsection64->sh_name;			
							}

							if(strncmp(bufptr,"data",MAX_PATH) == 0) {									
								symval=get_external_module_symbol(name);

								if(symval == -1) {
									symval=data+stval;
										
									add_external_module_symbol(name,symval,stsize);	
								}
							}
							else if(strncmp(bufptr,"bss",MAX_PATH) == 0) {
								symval=get_external_module_symbol(name);
								if(symval == -1) {
									symval=commondataptr+stval;

									add_external_module_symbol(name,symval,stsize);
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
				if(sectiontype == SHT_REL) {
					*ref=DO_386_PC32(symval,*ref,(size_t) ref);
				}
				else if(sectiontype == SHT_RELA) {
  					*ref=DO_386_PC32(symval,*ref,symval);
				}
			}
			else
			{
				kprintf_direct("module loader: unknown relocation type %d in module %s\n",symtype,filename);
		
				kernelfree(modulestartaddress);
				kernelfree(kernelmodules_end->commondata);
				kernelfree(kernelmodulelast->next);

				enablemultitasking();
				setlasterror(INVALID_EXECUTABLE);
				return(-1);
			}

			if(sectiontype == SHT_REL) {	
				if(elf_type == ELFCLASS32) {
					relptr32++;
				}
				else if(elf_type == ELFCLASS64) {
					relptr32++;
				}
			}
			else if(sectiontype == SHT_RELA) {
				if(elf_type == ELFCLASS32) {
					relptra64++;
				}
				else if(elf_type == ELFCLASS64) {
					relptra64++;
				}
			}

		}		
 	}
	

	if(elf_type == ELFCLASS32) {
		shptr32++;
	}
	else if(elf_type == ELFCLASS64) {
		shptr64++;
	}	
	
}

/* Add kernel module entry */

strncpy(kernelmodule.filename,filename,MAX_PATH);		/* filename */
kernelmodule.startaddress=modulestartaddress;			/* start address */
kernelmodule.size=modulesize;					/* size */
if(AddModule(&kernelmodule) == -1) {				/* Add module entry */
	kernelfree(codestart);
	return(-1);
}

/* call module entry point */
entry=codestart;
return(entry(argsx));
}

/*
 * Get name of 32-bit symbol from kernel module
 *
 * In:  strtab		     Pointer to ELF string table
	sectionheader_strptr Section header string table
	modulestartaddress   Pointer to start of module
	symtab		     Symbol table
	sectionptr	     Section header
	which		     Symbol index
	name		     Symbol name buffer
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t getnameofsymbol32(char *strtab,char *sectionheader_strptr,char *modulestartaddress,Elf32_Sym *symtab,Elf32_Shdr *sectionptr,size_t which,char *name) {
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
 * Get name of 64-bit symbol from kernel module
 *
 * In:  strtab		     Pointer to ELF string table
	sectionheader_strptr Section header string table
	modulestartaddress   Pointer to start of module
	symtab		     Symbol table
	sectionptr	     Section header
	which		     Symbol index
	name		     Symbol name buffer
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t getnameofsymbol64(char *strtab,char *sectionheader_strptr,char *modulestartaddress,Elf64_Sym *symtab,Elf64_Shdr *sectionptr,size_t which,char *name) {
size_t count;
char *strptr=strtab;
Elf64_Sym *symptr;
char *shptr;

symptr=(size_t) symtab+(sizeof(Elf64_Sym)*which);		/* point to symbol table entry */

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

for(count=0;count < bootinfo->number_of_symbols;count++) {
	if(strncmp(symptr,name,MAX_PATH) == 0) {				/* symbol found */
		symptr += (strlen(name)+1);		/* skip over name */
  
		valptr=symptr;			/* cast pointer to size_t */
		return(*valptr);			/* return value */
 	}

	symptr += strlen(symptr)+(sizeof(size_t)*2)+1;	/* skip over name, size and symbol */
}

return(-1);
}

/*
 * Find symbol from addrtess
 *
 * In:  address		Address
 *
 * Returns symbol value on success, NULL otherwise
 * 
 */
char *FindSymbolFromAddress(char *address) {
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS+KERNEL_HIGH;		/* point to boot information */
char *symptr;
size_t count;
size_t *valptr;
size_t symboladdress;
size_t symbolsize;
SYMBOL_TABLE_ENTRY *next;
char *nameptr;

symptr=bootinfo->symbol_start;
symptr += KERNEL_HIGH;		/* point to symbol table */

for(count=0;count < bootinfo->number_of_symbols;count++) {
	nameptr=symptr;

	symptr += (strlen(symptr)+1);		/* skip over name */
	valptr=symptr;			/* cast pointer to size_t */
	
	symboladdress=(size_t) *valptr;	/* get address */
	valptr++;
	symbolsize=(size_t) *valptr;		/* get size */

	if((address >= symboladdress) &&  (address <= (symboladdress+symbolsize))) return(nameptr);	/* found address */

	symptr += (sizeof(size_t)*2);	/* point to next */
}

next=externalmodulesymbols;

while(next != NULL) {
	if((next->address >= symboladdress) &&  (next->address <= (symboladdress+next->size))) return(next->name);	/* found address */

	next=next->next;
}

return(NULL);
}

/*
 * Add external module symbol
 *
 * In: name	Name of symbol
 *     val	Symbol value
	size	
 * Returns symbol value on success, -1 otherwise
 * 
 */
size_t add_external_module_symbol(char *name,size_t val,size_t size) {
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
next->size=size;
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
		kernelfree(kernelmodules_end->commondata);
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

/*
 * Get 32-bit module reference
 *
 * In:  modulestartaddress	Module start addres
 * 	elf_header		Pointer to module's ELF header
	shptr			Pointer to module's section header
 *
 * Returns 0 on success, -1 otherwise
 * 
 */
Elf32_Shdr *GetModuleReference32(char *modulestartaddress,Elf32_Ehdr *elf_header,Elf32_Shdr *shptr) {
return((modulestartaddress+elf_header->e_shoff)+(shptr->sh_info*sizeof(Elf32_Shdr)));
}

/*
 * Get 64-bit module reference
 *
 * In:  modulestartaddress	Module start addres
 * 	elf_header		Pointer to module's ELF header
	shptr			Pointer to module's section header
 *
 * Returns 0 on success, -1 otherwise
 * 
 */
Elf64_Shdr *GetModuleReference64(char *modulestartaddress,Elf64_Ehdr *elf_header,Elf64_Shdr *shptr) {
return((modulestartaddress+elf_header->e_shoff)+(shptr->sh_info*sizeof(Elf64_Shdr)));
}

/*
 * Add kernel module entry
 *
 * In: entry	Kenel module information
 *
 * Returns 0 on success, -1 otherwise
 * 
 */
size_t AddModule(KERNELMODULE *entry) {
if(kernelmodules == NULL) {		/* first entry */
	kernelmodules=kernelalloc(sizeof(KERNELMODULE));
	if(kernelmodules == NULL) return(-1);

	kernelmodulelast=kernelmodules;
	kernelmodules_end=kernelmodules;
}
else
{
	kernelmodules_end->next=kernelalloc(sizeof(KERNELMODULE));
	if(kernelmodules_end->next == NULL) return(-1);

	kernelmodulelast=kernelmodules_end;
	kernelmodules_end=kernelmodules_end->next;
}

memcpy(kernelmodules_end,entry,sizeof(KERNELMODULE));		/* copy module information */
kernelmodules_end->next=NULL;			/* set end of list */

setlasterror(NO_ERROR);
return(0);
}

/*
 * Check if module is loaded
 *
 * In: modulefilename	Module filename
 *
 * Returns 0 if module is loaded, -1 otherwise
 * 
 */
size_t IsModuleLoaded(char *filename) {
KERNELMODULE *kernelmodulenext;

kernelmodulenext=kernelmodules;

while(kernelmodulenext != NULL) {
	if(strncmpi(kernelmodulenext->filename,filename,MAX_PATH) == 0) return(0);		/* found module name */

	kernelmodulenext=kernelmodulenext->next;
}

return(-1);
}

/*
 * Find module from addresss
 *
 * In: address		Address
 *
 * Returns pointer to module if address is within module, NULL otherwise
 * 
 */
KERNELMODULE *FindModuleFromAddress(char *address) {
KERNELMODULE *kernelmodulenext;

kernelmodulenext=kernelmodules;

while(kernelmodulenext != NULL) {
	//kprintf_direct("%s %X %X %X\n",kernelmodulenext->filename,kernelmodulenext->startaddress,kernelmodulenext->size,address);

	/* found module */
	if((address >= kernelmodulenext->startaddress) && (address <= (kernelmodulenext->startaddress+kernelmodulenext->size))) return(kernelmodulenext);

	kernelmodulenext=kernelmodulenext->next;
}

return(NULL);
}
