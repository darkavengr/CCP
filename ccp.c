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
TIMEBUF create;
TIMEBUF modified;
TIMEBUF accessed;

create.year=1990;
create.month=7;
create.day=1;
create.hours=12;
create.minutes=30;
create.seconds=15;

modified=create;
accessed=create;

setfiletimedate("A:\\A long filename.txt",&create,&modified,&accessed);


if(exec("\\COMMAND.RUN","/P /K \\AUTOEXEC.BAT",FALSE) ==  -1) { /* can't run command interpreter */
	kprintf_direct("Missing or corrupt command interpreter, system halted (%d)",getlasterror());
	halt();
	while(1) ;;
}

asm("xchg %bx,%bx");
}

