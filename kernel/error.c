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
    along with CCP. If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stddef.h>

char *kstrerr(size_t error);

char *errors[] = {
"No error",\
"Invalid function number",\
"File not found",\
"Path not found",\
"Too many open files",\
"Access denied",\
"Invalid handle",\
"Memory control blocks destroyed",\
"Insufficient memory",\
"Invalid parameter",\
"Invalid address",\
"Invalid drive format",\
"Invalid access mode for open()",\
"Invalid data",\
"Invalid process",\
"Invalid drive specified",\
"Attempt to remove current directory",\
"Attempt to rename file or directory across drives",\
"End of directory",\
"Write-protect error",\
"Invalid device",\
"Not ready",\
"Module function not implemented",\
"CRC error",\
"Invalid executable",\
"Seek error",\
"End of file",\
"Invalid block number",\
"Kernel module already loaded",\
"Device read fault",\
"Device write fault",\
"General failure",\
"Sharing violation",\
"Lock violation",\
"Invalid disk change",\
"Not a directory",\
"Is a directory",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"",\
"File is open",\
"",\
"",\
"",\
"",\
"",\
"",\
"File already exists"
"",\
"Can't create directory entry",\
};


/*
 * Get error string from error code
 *
 * In: Error number
 *
 * Returns pointer to error message
 */

char *kstrerr(size_t error) {
return(errors[error]);
}

