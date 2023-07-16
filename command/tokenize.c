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


/*
 * Tokenize string into array
 *
 * In: char *linebuf			String to tokenize
       char *tokens[MAX_PATH][MAX_PATH]	Array to hold tokens
       char *split			Character delimiter
 *
 * Returns number of tokens
 */
int tokenize_line(char *linebuf,char *tokens[MAX_PATH][MAX_PATH],char *split) {
char *token;
int tc;
int count;
char c;
char *d;
char *splitptr;

tc=0;

memset(tokens,0,MAX_PATH);

token=linebuf;

d=tokens[0];

while(*token != 0) { 		
  if(*token == 0 ) break;

  c=-1;
  splitptr=split;

  while(c != 0) {
   c=*splitptr++;
   	
   if((*token == c ||  *token == c)) {
    token++;
    tc++;
    d=tokens[tc];
   }
  }

  *d++=*token++;
 }

tc++;
return(tc);
}
