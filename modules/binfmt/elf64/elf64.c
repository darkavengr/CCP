#include <elf.h>
#include <stdint.h>
#include <stddef.h>
#include "kernelhigh.h"
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "memorymanager.h"
#include "process.h"
#include "elf64.h"

#define MODULE_INIT elf64_init

int elf64_init(char *init) {
EXECUTABLEFORMAT exec;
uint8_t magicnumber[] = { 0x7f,'E','L','F',2,1,1 };

strncpy(exec.name,"ELF64",MAX_PATH);
memcpy(exec.magic,magicnumber,ELF_MAGIC_SIZE);

exec.magicsize=ELF_MAGIC_SIZE;
exec.callexec=&load_elf64;

if(register_executable_format(&exec) == -1) {
	kprintf_direct("elf64: Can't register binary format: %s\n",kstrerr(getlasterror()));

	setlasterror(INVALID_EXECUTABLE);
	return(-1);
}

setlasterror(NO_ERROR);
return(0);
}	

size_t load_elf64(char *filename) {
size_t handle;
uint32_t entry;
size_t type;
Elf64_Phdr *phbuf;
char *fullname[MAX_PATH];
size_t count;
Elf64_Ehdr elf_header;

getfullpath(filename,fullname);

handle=open(fullname,O_RDONLY);		/* open file */
if(handle == -1) return(-1);		/* can't open */

if(read(handle,&elf_header,sizeof(Elf64_Ehdr)) == -1) {
	close(handle);
	return(-1); /* read error */
}

/* check magic number, elf type and number of program headers */

if(elf_header.e_ident[0] != 0x7F && elf_header.e_ident[1] != 0x45 && elf_header.e_ident[2] != 0x4C && elf_header.e_ident[3] != 0x46) {	/*  elf */
	close(handle);

	setlasterror(INVALID_EXECUTABLE);
	return(-1);
}

if(elf_header.e_type != ET_EXEC) {		/* not executable */
	close(handle);
	setlasterror(INVALID_EXECUTABLE);
	return(-1);
}

if(elf_header.e_phnum == 0) {			/* no program headers */
	close(handle);
	setlasterror(INVALID_EXECUTABLE);
	return(-1);
}

/* allocate buffer for program headers */

phbuf=kernelalloc((elf_header.e_phentsize*elf_header.e_phnum));
if(phbuf == NULL) {
	close(handle);
	return(-1);
}

if(seek(handle,elf_header.e_phoff,SEEK_SET) == -1) {
	kernelfree(phbuf);
	close(handle);
	return(-1);
}

/* read program headers */

if(read(handle,phbuf,(elf_header.e_phentsize*elf_header.e_phnum)) == -1) {
	kernelfree(phbuf);
	close(handle);
	return(-1);
}

/*
 * go through program headers and load them if they are PT_LOAD */

for(count=0;count<elf_header.e_phnum;count++) {
	if(phbuf->p_type == PT_LOAD) {			/* if segment is loadable */

		if(phbuf->p_vaddr >= KERNEL_HIGH) {		/* invalid address */
			setlasterror(INVALID_EXECUTABLE);

			kernelfree(phbuf);
			close(handle);
			return(-1);
		}

		seek(handle,phbuf->p_offset,SEEK_SET);			/* seek to position */

		alloc_int(ALLOC_NORMAL,getpid(),phbuf->p_memsz,phbuf->p_vaddr);

		if(read(handle,phbuf->p_vaddr,phbuf->p_filesz) == -1) {	/* read error */
			if(getlasterror() != END_OF_FILE) {		
				kernelfree(phbuf);
	  
		    		close(handle);
		    		return(-1);
		   	}
		}

	}

	phbuf++;		/* point to next */
}

close(handle);

kernelfree(phbuf);
setlasterror(NO_ERROR);

return(elf_header.e_entry);
}

