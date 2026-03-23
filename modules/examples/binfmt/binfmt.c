#include <stdint.h>
#include <stddef.h>
#include "errors.h"
#include "binfmt.h"

#define MODULE_INIT example_binfmt_init

/*
 * Initilize example binary format module
 *
 * In: init	Initialization string
 *
 * Returns: 0 on success or -1 on failure
 *
 */
size_t example_binfmt_init(char *init) {
EXECUTABLEFORMAT exec;

/* Regiter binary format */

strncpy(exec.name,"Example binary format module",MAX_PATH);		/* module name */
strncpy(exec.magic,"EXAMPLE",MAX_PATH);					/* magic number */

exec.callexec=&example_binfmt_load;					/* binary format load handler */

if(register_executable_format(&exec) == -1) {			/* register executable format */
	kprintf_direct("example_binfmt: Can't register binary format: %s\n",kstrerr(getlasterror()));
	return(-1);
}

setlasterror(NO_ERROR);
return(0);
}	

/*
 * Load executable
 *
 * In: filename		Filename
 *
 * Returns: executable entry point on success or -1 on failure
 *
 */
size_t load_example_binfmt(char *filename) {

setlasterror(NO_ERROR);
return(0xBADBADBADBAD);
}

