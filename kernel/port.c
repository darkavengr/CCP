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

/*
 * Write byte to port
 *
 * In: uint16_t port		Port to write to
       uint8_t val		Byte to write

 * Returns nothing
 * 
 */
void outb(uint16_t port,uint8_t val) {
 asm volatile("outb %0,%1":: "a"(val),"d"(port));
 return;
}


/*
 * Read byte from port
 *
 * In: uint16_t port		Port to read from
 *
 * Returns byte value read from port
 * 
 */
uint8_t inb(uint16_t port) {
uint8_t val;

 asm volatile("inb %1,%0":"=a"(val) : "d"(port));
 return(val);
}

/*
 * Write word to port
 *
 * In: uint16_t port		Port to write to
       uint16_t val		Word to write

 * Returns nothing
 * 
 */
void outw(uint16_t port,uint16_t val) {
 asm volatile("outw %0,%1":: "a"(val),"d"(port));
 return;
}

/*
 * Read word from port
 *
 * In: uint16_t port		Port to read from
 *
 * Returns word value read from port
 * 
 */
uint16_t inw(uint16_t port) {
 uint16_t val;

 asm volatile("inw %1,%0":"=a"(val) : "d"(port));
 return(val);
 }

/*
 * Write dword to port
 *
 * In: uint32_t port		Port to write to
       uint32_t val		Dword to write

 * Returns nothing
 * 
 */
void outd(uint16_t port,uint32_t val) {
 asm volatile("out %0,%1":: "a"(val),"d"(port));
 return;
}

/*
 * Read dword from port
 *
 * In: uint16_t port		Port to read from
 *
 * Returns dword value read from port
 * 
 */
uint32_t ind(uint16_t port) {
 uint32_t val;

 asm volatile("in %1,%0":"=a"(val) : "d"(port));
 return(val);
 }


