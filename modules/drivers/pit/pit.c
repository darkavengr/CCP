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

#include <stdint.h>
#include <stddef.h>
#include "../pit/pit.h"
#include "mutex.h"
#include "device.h"
#include "vfs.h"
#include "string.h"

#define MODULE_INIT pit_init

/*
 * Initialize PIT (Programmable Interval Timer)
 *
 * In:  init	Initialization string
 *
 * Returns: nothing
 *
 */
void pit_init(char *init) {
outb(PIT_COMMAND_REGISTER,0x34);				/* set PIT timer interval */
outb(PIT_CHANNEL_0_REGISTER,PIT_VAL & 0xFF);
outb(PIT_CHANNEL_0_REGISTER,((PIT_VAL >> 8) & 0xFF));

return;
}

