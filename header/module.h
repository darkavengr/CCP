#include <stdint.h>
#include <elf.h>

#ifndef MODULE_H
	#define MODULE_H
	#define DO_386_32(S, A)	((S) + (A))
	#define DO_386_PC32(S, A, P)	((S) + (A) - (P))

	typedef struct {
		char *name[MAX_PATH];
		size_t address;
		struct SYMBOL_TABLE_ENTRY *next;
	} SYMBOL_TABLE_ENTRY;

	typedef struct {
		uint8_t filename[MAX_PATH];
		char *commondata;
		char *startaddress;
		size_t size;
		struct KERNELMODULE *next;
	} KERNELMODULE;
#endif

size_t load_kernel_module(char *filename,char *argsx);
size_t getnameofsymbol32(char *strtab,char *sectionheader_strptr,char *modulestartaddress,Elf32_Sym *symtab,Elf32_Shdr *sectionptr,size_t which,char *name);
size_t getnameofsymbol64(char *strtab,char *sectionheader_strptr,char *modulestartaddress,Elf64_Sym *symtab,Elf64_Shdr *sectionptr,size_t which,char *name);
size_t getkernelsymbol(char *name);
size_t add_external_module_symbol(char *name,size_t val);
size_t get_external_module_symbol(char *name);
size_t unload_kernel_module(char *filename);
Elf32_Shdr *GetModuleReference32(char *modulestartaddress,Elf32_Ehdr *elf_header,Elf32_Shdr *shptr);
Elf64_Shdr *GetModuleReference64(char *modulestartaddress,Elf64_Ehdr *elf_header,Elf64_Shdr *shptr);
size_t IsModuleLoaded(char *filename);
size_t AddModule(KERNELMODULE *entry);
KERNELMODULE *FindModuleFromAddress(char *address);

