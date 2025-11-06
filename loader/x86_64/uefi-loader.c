/*  	CCP Version 0.0.1
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

/* CCP UEFI loader */

#include <uefi.h>
#include <stdint.h>
#include <stddef.h>
#include <elf.h>
#include "bootinfo.h"
#include "pagesize.h"
#include "pagesize.h"
#include "errors.h"
#include "string.h"

/* based on  https://gitlab.com/bztsrc/posix-uefi/-/blob/master/examples/ */

BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS;
size_t last_error=0;
char *loader_name="uefi-loader:";
char *press_any_key_to_reboot="Press any key to restart";
void (*entry_point)(void);

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

memset(bootinfo,0,sizeof(BOOT_INFO);	/* clear boot information */

bootinfo->memorysize=detect_total_memory_size();	/* get memory size */

printf("%s: %s %s\n",loader_name,loading_message,ccp_kernel_filename);

if(load_kernel(ccp_kernel_filename) == -1) {		/* load kernel */
	printf("%s: Error loading %s: %s\n",loader_name,ccp_kernel_filename,errors[lasterror]);
	printf(press_any_key_to_reboot);

	getchar();

	RT->ResetSystem(EfiResetWarm,EFI_SUCCESS,0,NULL);		/* reboot */
}

if(load_initrd(initrd_filename) == -1) {		/* load initial RAM disk */
	printf("%s: No initrd found (%s), skipping it.");
}

exit_bs();					/* exit boot services */

entrypoint=bootinfo->kernel_start;		/* point to start */
entrypoint();					/* transfer control */
}

/*
 * Detect memory size
 *
 * In: Nothing
 *
 * Returns: Total memory size
 *
 */
uint64_t detect_total_memory_size(void) {
efi_status status;
efi_status_t status;
efi_memory_descriptor_t *memory_map=NULL;
efi_memory_descriptor_t *memptr=NULL;
size_t memory_map_size=0;
size_t map_key=0
size_t desc_size=0;
char *no_memory_map="%s: Unable to get memory map\n";
uint64_t mem_total=0;

status = BS->GetMemoryMap(&memory_map_size, NULL, &map_key, &desc_size, NULL);		/* get memory map size, map key and descriptor size */
if((status != EFI_BUFFER_TOO_SMALL) || (!memory_map_size)) {
	lasterror=NO_MEM;
	return(-1);
}

memory_map=malloc(desc_size+(4*desc_size));	/* allocate memory map buffer plus 4 extra */
if(memory_map == NULL) {
	lasterror=NO_MEM;
	return(-1);
}

/* loop through memory map and add it up */

memptr=memory_map;

while(memptr != ((uint8_t) memory_map+memory_map_size)) {	/* until end */
	mem_total += (memptr->NumberOfPages*PAGE_SIZE);
}

setlasterror(NO_ERROR);
return(mem_total);
}

/*
 * Load Elf64 kernel
 *
 * In: Filename	Filename
 *
 * Returns: 0 on success, -1 on failure
 *
 * Based on modules/binfmt/Elf64/Elf64.c
 *
 */
size_t load_kernel(char *filename) {
FILE *handle;
size_t count;
Elf64_Ehdr elf_header;
Elf64_Phdr *phbuf;
Elf64_Shdr *shbuf;
Elf64_Sym *symtab;
Elf64_Sym *symptr;
char *strtab;
void *rawbuf;
size_t symtabsize;

handle=fopen(filename,"rb");		/* open file */
if(!handle) {
	lasterror=INVALID_EXECUTABLE;
	return(-1);
}

if(fread(&elf_header,sizeof(Elf64_Ehdr),1,handle) != sizeof(Elf64_Ehdr)) {
	fprintf(stderr,elf_read_error,loader_name);

	fclose(handle);
	return(-1); /* read error */
}

/* check magic number, elf type and number of program headers */

if(elf_header.e_ident[0] != 0x7F && elf_header.e_ident[1] != 0x45 && elf_header.e_ident[2] != 0x4C && elf_header.e_ident[3] != 0x46) {	/*  elf */
	fclose(handle);

	lasterror=INVALID_EXECUTABLE;
	return(-1);
}

if(elf_header.e_type != ET_EXEC) {		/* not executable */
	fclose(handle);

	lasterror=INVALID_EXECUTABLE;
	return(-1);
}

if((elf_header.e_phnum == 0) || (elf_header.e_shnum == 0)) {			/* no program headers */
	fclose(handle);

	lasterror=INVALID_EXECUTABLE;
	return(-1);
}

/* allocate buffer for program headers */

phbuf=malloc((elf_header.e_phentsize*elf_header.e_phnum));
if(phbuf == NULL) {
	fclose(handle)

	lasterror=NO_MEM;
	return(-1);
}

if(fseek(handle,elf_header.e_phoff,SEEK_SET) == -1) {
	lasterror=SEEK_ERROR;

	free(phbuf);
	fclose(handle);
	return(-1);
}

/* read program headers */

if(read(handle,phbuf,(elf_header.e_phentsize*elf_header.e_phnum)) == -1) {
	lasterror=INVALID_EXECUTABLE;
		
	free(phbuf);
	fclose(handle);
	return(-1); /* read error */
}

/*
 * go through program headers and load them if they are PT_LOAD */

for(count=0;count<elf_header.e_phnum;count++) {
	if(phptr->p_type == PT_LOAD) {			/* if segment is loadable */
		if(bootinfo->kernel_start == 0) {	/* assumes first PT_LOAD segment is the kernel */
			bootinfo->kernel_start=phptr->p_offset;
			bootinfo->kernel_size=phptr->p_memsze;
		}

		fseek(handle,phptr->p_offset,SEEK_SET);			/* seek to position */

		if(read(handle,phptr->p_vaddr,phbuf->p_filesz) == -1) {	 /* read program segment */
			lasterror=READ_FAULT;
		
			free(phbuf);
			fclose(handle);
			return(-1); /* read error */
		}

	}

	phptr++;		/* point to next */
}

fclose(handle);

/* Load symbols */

bootinfo->symbol_start=bootinfo->kernel_start+bootinfo->kernel_size;		/* symbols start after the kernel */

/* allocate buffer for program headers */

shbuf=malloc((elf_header.e_shentsize*elf_header.e_shnum));
if(shbuf == NULL) {
	fclose(handle);

	lasterror=NO_MEM;
	return(-1);
}

if(fseek(handle,elf_header.e_shoff,SEEK_SET) == -1) {
	lasterror=SEEK_ERROR;

	free(shbuf);
	fclose(handle);
	return(-1);
}

/* read section headers */

if(read(handle,shbuf,(elf_header.e_shentsize*elf_header.e_shnum)) == -1) {
	lasterror=READ_FAULT;
		
	free(shbuf);
	fclose(handle);
	return(-1); /* read error */
}

/* Find string and symbol tables */

shptr=shbuf;

for(count=0;count<elf_header.e_shnum;count++) {
	if((shptr->sh_type == SHT_SYMTAB) || (shptr->sh_type == SHT_STRTAB)) {	/* symbol or string table */
	
		rawbuf=malloc(shptr->sh_size);
		if(rawbuf == NULL) {
			fclose(handle);

			lasterror=NO_MEM;
			return(-1);
		}	

		if(fseek(handle,shptr->sh_offset,SEEK_SET) == -1) {
			lasterror=SEEK_ERROR;
	
			free(shbuf);
			fclose(handle);
			return(-1);
		}


		if(read(handle,rawbuf,shptr->sh_size) == -1) {
			lasterror=READ_FAULT;
				
			free(shbuf);
			fclose(handle);
			return(-1); /* read error */
		}

		if(shptr->sh_type == SHT_SYMTAB) {		/* symbol table */
			symtab=rawbuf;	
			symtabsize=shptr->sh_size;

			bootinfo->number_of_symbols=symtabsize/sizeof(Elf64_Sym);	/* get number of symbols */
		}

		if(shptr->sh_type == SHT_STRTAB) strtab=rawbuf;	/* string table */
	}

	shptr++;		/* point to next */
}


/* Copy symbol names and symbols to buffer */

symtabptr=symtab;

while(symtabsize > 0) {
	strtabptr=(strtab+symtab->name)-1;	/* point to name */

	strcpyn(strptr,strtabptr,MAX_PATH);	/* copy name */
	
	strtabptr += (strlen(strptr)+1);	/* point to end of symbol name */

	size_t valptr=strtabptr;
	*valptr=symtabptr->st_value;		/* copy value */

	symtabsize -= sizeof(Elf64_Sym);
}

free(phbuf);
free(shbuf);
free(symtab);
free(strtab);

fclose(handle);

lasterror=NO_ERROR;
return(0);
}

/*
 * Load initial RAM disk
 *
 * In: Filename	Filename
 *
 * Returns: 0 on success, -1 on failure
 *
 * Based on modules/binfmt/Elf64/Elf64.c
 *
 */
size_t load_initrd(char *filename) {
FILE *handle;
void *buf=(bootinfo->kernel_start+bootinfo->kernel_size)+(bootinfo->symbol_start+bootinfo->bootinfo_symbol_size);
size_t filesize;

handle=fopen(filename,"rb");		/* open file */
if(!handle) {
	lasterror=FILE_NOT_FOUND;
	return(-1);
}

/* get file size */
if(fseek(handle,0,SEEK_END) != 0) {
	fclose(handle);

	lasterror=SEEK_ERROR;
	return(-1);
}

filesize=ftell(handle);
if(filesize == -1) {
	fclose(handle);

	lasterror=SEEK_ERROR;
	return(-1);
}

if(fseek(handle,0,SEEK_SET) != 0) {
	fclose(handle);

	lasterror=SEEK_ERROR;
	return(-1);
}

if(fread(buf,filesize,1,handle) != filesize) {			/* read initial RAM disk data */
	lasterror=READ_FAULT;

	fclose(handle);
	return(-1); /* read error */
}

fclose(handle);

bootinfo->initrd_start=buf;
bootinfo->initrd_size=filesize;

return(0);
}

