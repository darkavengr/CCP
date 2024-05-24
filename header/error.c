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

char *kstrerr(size_t error);

char *errors[] = { "No error",\
"Invalid function",\
"File not found",\
"Path not found",\
"No handles",\
"Access denied",\
"Invalid handle",\
"Heap header corrupt",\
"Out of memory",\
"Unknown drive format",\
"Access error",\
"Invalid file",\
"Directory not empty",\
"Invalid drive specification",\
"Can't rename across drives",\
"End of directory",\
"Write protect error",\
"Invalid device",\
"Drive not ready",\
"Invalid CRC",\
"File already exists",\
"Directory is full",\
"Drive is full",\
"Input past end of file",\
"Device I/O error",\
"Invalid file",\
"Invalid executable",\
"Device already exists",\
"Invalid process",\
"Invalid device",\
"Device is in use",\
"Invalid kernel module",\
"Kernel module already loaded",\
"No processes",\
"End of file reached",\
"No drives",\
"Seek past end",\
"Can't close device",\
"Invalid block number",\
"File already open",\
"General failure",\
"Not a directory",\
"Not implemented",\
"File in use",\
"Invalid executable format",\
"Unknown filesystem",\
"Directory not empty",\
"Invalid address",\
"Kernel module already loaded",
"Not a device"
 };

/*
 * Display error
 *
 * In:  nothing
 *
 * Returns pointer to error message
 */

char *kstrerr(size_t error) {
return(errors[error]);
}
