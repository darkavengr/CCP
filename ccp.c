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
#include "header/errors.h"
#include "filemanager/vfs.h"
#include "processmanager/process.h"
#include "header/debug.h"
#include "modules/filesystems/fat/fat.h"

/*
 * High-level kernel initalization
 *
 * In: nothing
 *
 * Returns nothing
 */

void kernel(void) {
if(exec("\\COMMAND.RUN","/P /K \\AUTOEXEC.BAT",FALSE) ==  -1) {
	kprintf_direct("Missing or corrupt command interpreter, system halted (%d)",getlasterror());

	asm("xchg %bx,%bx");
	halt();
	while(1) ;;
}

asm("xchg %bx,%bx");
}

