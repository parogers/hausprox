/*
 * haus|prox - Electronic door access control system
 * Copyright (C) 2011  Peter Rogers @thinkhaus
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <avr/pgmspace.h>
#include "Stream.h"
#include "Arduino.h"

class DebouncedInput
{
  private:
    // The last stable input state
    boolean state;
    // Whether the input state changed on the last call to 'update'
    boolean changed;
    // How many counts the input has remained stable
    int bounceCount;
    // When the input state was last updated
    unsigned long lastTime;
  
  public:
    DebouncedInput();

    /* Called periodically to update the internal input state */    
    void update(boolean state);
    /* Returns the stable button state */
    boolean getState() { return state; }
    /* Checks whether the (stable) input state changed on the last call to 'update' */
    boolean hasChanged() { return changed; }
};

/* Reads a line of input from the stream up to 'size-1' bytes. Note this string 
 * is always null-terminated. Returns the number of chars read. */
int read_line(Stream *stream, char *buf, int size);
/* Prints a string stored in program memory */
void print_prog_str(Stream *stream, const prog_char *str);
/* Decodes a binary coded decimal number */
byte decodeBCD(byte data);
/* Encode a value as BCD */
byte encodeBCD(byte data);

#endif

