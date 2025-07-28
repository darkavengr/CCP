/*
  CCP Version 0.0.1
    (C) Matthew Boote 2020-2022

    This file is part of CCP.

    CCP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CCP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CCP.  If not, see <https://www.gnu.org/licenses/>.
*/

#define IDT_ENTRY_PRESENT  		(1 << 7)
#define IDT_RING0	 		(0 << 5)
#define IDT_RING3	 		(3 << 5)
#define IDT_TASK_GATE	 		0x5
#define IDT_16BIT_INTERRUPT_GATE	0x6
#define IDT_16BIT_TRAP_GATE		0x7
#define IDT_32BIT_64BIT_INTERRUPT_GATE	0xE
#define IDT_32BIT_64BIT_TRAP_GATE	0xF

