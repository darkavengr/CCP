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
    along with CCP.  If not, see <https:www.gnu.org/licenses/>.
*/

/*
 *
 * C C P - Computer control program
 *
 */
#include <stdint.h>
#include <stddef.h>
#include "errors.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "memorymanager.h"
#include "process.h"
#include "debug.h"
#include "string.h"

void halt(void);

/*
 * High-level kernel initalization
 *
 * In: nothing
 *
 * Returns nothing
 */

void kernel(void) {
FILERECORD commandrun;
size_t returnvalue;

kprintf_direct("load_kernel_module() returned %d\n",load_kernel_module("A:\\TEST.O",""));
asm("xchg %bx,%bx");

if(findfirst("\\AUTOEXEC.BAT",&commandrun) == 0) {
	returnvalue=exec("\\COMMAND.RUN","/P /K \\AUTOEXEC.BAT",FALSE);
}
else
{
	returnvalue=exec("\\COMMAND.RUN","/P",FALSE);
}

if(returnvalue ==  -1) {
	kprintf_direct("Missing or corrupt command interpreter, system halted, (error 0x%X:%s)",getlasterror(),kstrerr(getlasterror()));

	asm("xchg %bx,%bx");
	halt();
	while(1) ;;
}

asm("xchg %bx,%bx");
}

