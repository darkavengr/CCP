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
#include "../filemanager/vfs.h"
#include "../header/errors.h"
#include "../header/bootinfo.h"
#include "../header/kernelhigh.h"
#include "module.h"
#include <elf.h>

size_t load_kernel_module(char *filename,char *argsx);
size_t getnameofsymbol(char *strtab,size_t which,char *name);
size_t getkernelsymbol(char *name);
size_t add_external_module_symbol(char *name,size_t val);
size_t getnameofsymbol(char *strtab,size_t which,char *name);
size_t get_external_module_symbol(char *name);

SYMBOL_TABLE_ENTRY *externalmodulesymbols=NULL;
SYMBOL_TABLE_ENTRY *externalmodulesymbols_end=NULL;

/*
 * Load and execute kernel module
 *
 * In: char *filename		Filename of kernel module
 *     char *argsx		Arguments
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
Elf32_Rel *relbuf;
Elf32_Rela *relbufa;
size_t whichsym;
size_t symtype;
Elf32_Sym *symtab=NULL;
Elf32_Sym *symptr=NULL;
char *strtab;
char *strptr;
char *entryptr;
size_t *addr;
size_t *ref;
void *(*entry)(char *);
size_t numberofrelocentries;
size_t symval;
uint32_t codestart;
char *sectionheader_strptr;
size_t reloc_count;
size_t codeaddress;

disablemultitasking();

getfullpath(filename,fullname);

handle=open(fullname,_O_RDONLY);		/* open file */
if(handle == -1) return(-1);		/* can't open */

buf=kernelalloc(getfilesize(handle));
if(buf == -1) return(-1);

if(read(handle,buf,getfilesize(handle)) == -1) return(-1); /* read error */

elf_header=buf;

if(elf_header->e_ident[0] != 0x7F && elf_header->e_ident[1] != 0x45 && elf_header->e_ident[2] != 0x4C && elf_header->e_ident[3] != 0x46) {	/* not elf */
 setlasterror(INVALID_DRIVER);
 kernelfree(buf);
 return(-1);
}

if(elf_header->e_type != ET_REL) {
 setlasterror(INVALID_DRIVER);
 kernelfree(buf);
 return(-1);
}

if(elf_header->e_shnum == 0) {	
 setlasterror(INVALID_DRIVER);
 kernelfree(buf);
 return(-1);
}

/* find the location of the section header string table.
   it will be used to find the string table */

shptr=(size_t) buf+elf_header->e_shoff+(elf_header->e_shstrndx*sizeof(Elf32_Shdr));	/* find string section in table */
sectionheader_strptr=(buf+shptr->sh_offset)+1;		/* point to section header string table */

/* find the symbol table and string table from section header table */

shptr=buf+elf_header->e_shoff;

for(count=0;count < elf_header->e_shnum;count++) {
 if(shptr->sh_type == SHT_SYMTAB && symtab == NULL) symtab=buf+shptr->sh_offset;

 if(shptr->sh_type == SHT_STRTAB) {		/* string table */

	bufptr=sectionheader_strptr+shptr->sh_name;	/* point to section header string table */

	if(strcmp(bufptr,"strtab") == 0) {
 	 strtab=buf+shptr->sh_offset+1;		/* find string table */
  	 break;
       }
 }

 shptr++;
}

/* check if symbol and string table present */

if(symtab == NULL) {
 setlasterror(INVALID_DRIVER);
 kernelfree(buf);
 return(-1); 
}

if(shptr == NULL) {
setlasterror(INVALID_DRIVER);
 kernelfree(buf);
 return(-1); 
}


shptr=buf+elf_header->e_shoff+(elf_header->e_shstrndx*elf_header->e_shentsize);	/* point to string table */
entryptr=(buf+shptr->sh_offset)+1;

count=1;

while(*entryptr != 0) {
 if((strcmpi(entryptr,".text") == 0) || (strcmpi(entryptr,".rel.text") == 0)) {	/* found name of section */
   shptr=buf+(elf_header->e_shoff+(count*elf_header->e_shentsize));	/* point to section header */   
   codestart=buf+shptr->sh_offset;	/* point to address */
   break;
  }

  entryptr=entryptr+(strlen(entryptr))+1;
  count++;
}

if(count == elf_header->e_shnum) {
 kprintf_direct("Missing symbol .text or .rel.text in %s\n",filename);

  setlasterror(INVALID_DRIVER);

  kernelfree(buf);
  return(-1);
 }

shptr=buf+elf_header->e_shoff;

/* Relocate elf references

  Search through section table to find SHT_REL or SHL_RELA sections */

for(count=0;count < elf_header->e_shnum;count++) {

// kprintf_direct("shptr=%X\n",shptr);

 if(shptr->sh_type == SHT_REL) {
//   kprintf_direct("Found SHT_REL section\n");
 
   numberofrelocentries=shptr->sh_size/sizeof(Elf32_Rel);

 //  kprintf_direct("Number of reloc entries=%d\n",numberofrelocentries);

   for(reloc_count=0;reloc_count<numberofrelocentries;reloc_count++) {
   	      relbuf=(size_t) buf+shptr->sh_offset+(count*sizeof(Elf32_Rel));

		/* 	Get the location where the symbol value is to be placed  */

	     whichsym=relbuf->r_info >> 8;			/* which symbol */
             symtype=relbuf->r_info & 0xff;		/* symbol type */

	     asm("xchg %bx,%bx");

	     rel_shptr=(size_t) buf + shptr->sh_offset+((size_t) reloc_count*(sizeof(Elf32_Shdr)));	/* find section associated with relocation */

	     kprintf_direct("reloc_count=%d\n",reloc_count);

	     kprintf_direct("rel_shptr=%X\n",rel_shptr);	     
	     asm("xchg %bx,%bx");

	     codeaddress=(size_t) buf+rel_shptr->sh_offset;		/* find address from sction */
	     *ref=codeaddress+relbuf->r_offset;				/* location of symbol */


	      /*	Get the value to place at the location ref into symval */

	     asm("xchg %bx,%bx");
             symptr=(size_t) symtab+(whichsym*sizeof(Elf32_Sym));

	     kprintf_direct("relbuf->r_offset=%X\n",relbuf->r_offset);
	     kprintf_direct("whichsym=%X\n",whichsym);
	     kprintf_direct("symptr=%X\n",symptr);


             if(symtype == SHN_UNDEF) {		/* it's an extern */
	     	      kprintf_direct("strtab=%X\n",strtab);
		 
                      getnameofsymbol(strtab,whichsym,name);	/* get name of symbol */

		      kprintf_direct("external=%s\n",name);
		//      asm("xchg %bx,%bx");
		      symval=getkernelsymbol(name);
		
		      kprintf_direct("symval=%s %X\n",name,symval);

	              if(symval == -1) {
  		        kprintf_direct("not kernel symbol\n");
		//	asm("xchg %bx,%bx");

			/* if it's not a kernel symbol, check if it is a external module symbol */
		       symval=get_external_module_symbol(name);
			
	               if(symval == -1)	symval=symptr->st_value;
                      }

            }
	    else 
            {   
                          symval=symptr->st_value;
	    }

//	     asm("xchg %bx,%bx");
	     kprintf_direct("rebuf=%X\n",relbuf->r_offset);
	     kprintf_direct("ref=%X\n",ref);
   	     kprintf_direct("%s\n",name);

	     add_external_module_symbol(name,symval);		/* add external symbol to list */

/* update reference in section */

	     switch(symtype) {
	      case R_386_NONE:
	       break;
		
	      case R_386_32:
	       *ref=DO_386_32(symval,*ref);
	       break;

	      case R_386_PC32:
	       *ref=DO_386_PC32(symval,*ref,(size_t) ref);
	       break;

     	      default:
	       kprintf_direct("kernel: unknown relocation type %d in %s\n",symtype,filename);

	       enablemultitasking();
	       setlasterror(INVALID_DRIVER);
	       return(-1);
             }

//	kprintf_direct("**********\n");
//	asm("xchg %bx,%bx");

      	relbuf++;
       }

  asm("xchg %bx,%bx");
 }


 if(shptr->sh_type == SHT_RELA) {
 //   kprintf_direct("Found SHT_RELA section\n");
  //  asm("xchg %bx,%bx");

    numberofrelocentries=shptr->sh_size/sizeof(Elf32_Rela);
    relbufa=buf+shptr->sh_offset;

   for(reloc_count=0;reloc_count<numberofrelocentries;reloc_count++) {
	     whichsym=relbufa->r_info >> 8;			/* which symbol */
             symtype=relbufa->r_info & 0xff;		/* symbol type */

	     addr=buf+shptr->sh_info;		/* find location to do relocation */
	     ref=codestart+relbufa->r_offset;	/* location of symbol */

	      /*	Get the value to place at the location ref into symval */

             symptr=symtab+(whichsym*sizeof(Elf32_Sym));
	
             if((symptr->st_info >> 4) == STB_GLOBAL) {		/* it's an extern */
                      getnameofsymbol(strtab,whichsym,name);	/* get name of symbol */

		      symval=getkernelsymbol(name);
	              if(symval == -1) {
			symval=get_external_module_symbol(name);			

	                if(symval == -1) {
		          kprintf_direct("Unknown external symbol %s in %s\n",filename,name);

		          enablemultitasking();
                          setlasterror(INVALID_DRIVER);
		          kernelfree(buf);
		          return(-1); 
		        }
                     }
            }
	    else 
            {   
	       symval=symptr->st_value;
	    }

             symval += relbufa->r_addend;
/* update reference in section */

	     add_external_module_symbol(name,symval);		/* add external symbol to list */

	     switch(symtype) {
	      case R_386_NONE:
	       break;
		
	      case R_386_32:
	       *ref=DO_386_32(symval,*ref);
	       break;


	      case R_386_PC32:
		*ref = DO_386_PC32(symval,*ref,symval);
		break;


     	      default:
	       enablemultitasking();
	       setlasterror(INVALID_DRIVER);
	       return(-1);
             }

	       relbufa++;	
       }
   }
      	
	
shptr++;				
}

close(handle);

asm("xchg %bx,%bx");

/* find entry point */
count=1;
strptr=strtab;

while(*strptr != 0) {
 kprintf_direct("%s\n",strptr);
 asm("xchg %bx,%bx");

 if((strcmpi(strptr,"init") == 0)) {	/* found name of section */  
   symptr=(size_t) symtab+(whichsym*sizeof(Elf32_Sym));
   codestart=symptr->st_value;

   asm("xchg %bx,%bx");

   entry=codestart;
   entry(argsx);
  
   setlasterror(NO_ERROR);
   return(0);
  }

  strptr=strptr+(strlen(strptr))+1;
  count++;
}

setlasterror(INVALID_DRIVER);
return(-1);
}

/*
 * Get name of symbol from kernel module
 *
 * In: strtab	Pointer to ELF sting table
       which	Symbol index
       name	Symbol name
 *
 * Returns 0 on success, -1 otherwise
 * 
 */

size_t getnameofsymbol(char *strtab,size_t which,char *name) {
 size_t count=0;
 char *strptr=strtab;
	
 for(count=0;count<which;count++) {
  strptr += strlen(strptr)+1;
 }

 strcpy(name,strptr);
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

if(externalmodulesymbols == NULL) {		/* first in list */
 externalmodulesymbols=kernelalloc(sizeof(SYMBOL_TABLE_ENTRY));
 if(externalmodulesymbols == NULL) return(-1);

 externalmodulesymbols_end=externalmodulesymbols;
}
else
{
 externalmodulesymbols->next=kernelalloc(sizeof(SYMBOL_TABLE_ENTRY));
 if(externalmodulesymbols->next == NULL) return(-1);

 externalmodulesymbols_end=externalmodulesymbols->next;
}

/* add details to end */

strcpy(externalmodulesymbols_end->name,name);
externalmodulesymbols_end->address=val;

setlasterror(NO_ERROR);
return(-1);
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
  kprintf_direct("%s\n",next->name);

 if(strcmp(next->name,name) == 0) return(next->address);

 next=next->next;
}

return(-1);
}




