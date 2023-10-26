/*  CCP Version 0.0.1
    (C) Matthew Boote 2020

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

void outb(uint16_t port,uint8_t val) {
 asm volatile("outb %0,%1":: "a"(val),"d"(port));
 return;
}

uint8_t inb(uint16_t port) {
uint8_t val;

 asm volatile("inb %1,%0":"=a"(val) : "d"(port));
 return(val);
}

void outw(uint16_t port,uint16_t val) {
 asm volatile("outw %0,%1":: "a"(val),"d"(port));
 return;
}

uint16_t inw(uint16_t port) {
 uint16_t val;

 asm volatile("inw %1,%0":"=a"(val) : "d"(port));
 return(val);
 }

void outd(uint16_t port,uint32_t val) {
 asm volatile("out %0,%1":: "a"(val),"d"(port));
 return;
}

uint32_t ind(uint16_t port) {
 uint32_t val;

 asm volatile("in %1,%0":"=a"(val) : "d"(port));
 return(val);
 }


