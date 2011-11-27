/*
haus|prox - Electronic door access control system
Copyright (C) 2011  Peter Rogers @thinkhaus

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/pgmspace.h>
#include "utils.h"

int read_line(Stream *stream, char *buf, int size)
{
  int pos = 0;
  
  if (size == 0) {
    return 0;
  }
  
  while(pos < size-1) 
  {
    int ch = stream->read();
    if (ch == -1) {
      break;
    } 
    buf[pos++] = (char)ch;
    if (ch == '\n') {
      break;
    }
  }
  buf[pos] = 0;
  return pos;
}

void print_prog_str(Stream *stream, const prog_char str[])
{
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++)) != 0)
    stream->print(c,BYTE);
}

