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
#include "../../../header/errors.h"
#include "../../../processmanager/mutex.h"
#include "../../../devicemanager/device.h"

#define MODULE_INIT null_init

void null_init(char *init);
void nul(size_t op,void *buf,size_t size);

/*
 * Initialize null device
 *
 * In:  init 	Initialization string
 *
 *  Returns: nothing
 *
 */
void null_init(char *init) {
CHARACTERDEVICE device;

strcpy(&device.name,"NUL");
device.charioread=&nul;
device.chariowrite=NULL;
device.ioctl=NULL;
device.flags=0;
device.data=NULL;
device.next=NULL;

add_char_device(&device);			
}

/*
 * Null device handler
 *
 * In: op 	Operation (0=read,1=write)
       buf	Buffer
       size	Size
 *
 *  Returns: nothing
 *
 */
void nul(size_t op,void *buf,size_t size) {
return;
}



