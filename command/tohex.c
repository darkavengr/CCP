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
#include "command.h"
#include "../header/errors.h"

/*
 * Convert number to hexadecimal string
 *
 * In: uint32_t hex	Number to convert
       char *buf	Buffer to store converted number
 *
 * Returns nothing
 */

uint32_t tohex(uint32_t hex,char *buf) {
uint32_t count;
uint32_t h;
uint32_t shiftamount;
uint32_t mask;

char *b;
char *hexbuf[] = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"};

mask=0xF0000000;
shiftamount=28;

b=buf;

for(count=1;count<9;count++) {
 h=((hex & mask) >> shiftamount);	/* shift nibbles to end */

 shiftamount=shiftamount-4;
 mask=mask >> 4;  
 strcpy(b++,hexbuf[h]);
}

 return(NULL);
}

