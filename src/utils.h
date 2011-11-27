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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <avr/pgmspace.h>
#include "Stream.h"
#include "WProgram.h"

int read_line(Stream *stream, char *buf, int size);
void print_prog_str(Stream *stream, const prog_char *str);

#endif

