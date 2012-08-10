/*
 * haus|prox - Electronic door access control system
 * Copyright (C) 2011  Peter Rogers (peter.rogers@gmail.com)
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

#include <avr/pgmspace.h>
#include "utils.h"

/******************/
/* DebouncedInput */
/******************/

DebouncedInput::DebouncedInput()
{
  state = false;
  changed = false;
  bounceCount = 0;
  lastTime = millis();
}

void DebouncedInput::update(boolean st)
{
  unsigned long tm = millis();
  /* Update the state once every millisecond */
  if (tm == lastTime) {
    return;
  }
  lastTime = tm;
  
  changed = false;
  if (st != state) {
    // The button state has changed. Increment the bounce counter
    if (bounceCount++ > 100) {
      // Set the new stable button state
      changed = true;
      state = st;
      bounceCount = 0;
    }
  } else {
    /* Restart the bounce count */
//    if (bounceCount > 0)
//      Serial.println(bounceCount);
    bounceCount = 0;
  }
}

/*************/
/* Functions */
/*************/

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
    stream->print(c);
}

void println_prog_str(const prog_char str[])
{
  print_prog_str(str);
  Serial.println();
}

void print_prog_str(const prog_char str[])
{
  print_prog_str(&Serial, str);
}

boolean prog_str_equals(const prog_char *str1, const char *str2)
{
  char ch1, ch2;
  if(!str1 || !str2) return false;
  while(1)
  {
    ch1 = pgm_read_byte(str1++);
    ch2 = *str2++;
    if (!ch1 && !ch2) break;
    if (ch1 != ch2) return false;
  }
  return true;
}

byte decodeBCD(byte data)
{
  return 10*(data >> 4) + (data & 0xF);
}

byte encodeBCD(byte data)
{
  return ((data/10) << 4) | ((data%10) & 0xF);
}

void trim(char *str)
{
  // The whitespace characters
  char *whitespace = "\n\r \t";
  int n = strlen(str)-1;
  
  // Trim trailing whitespace
  while(n >= 0 && strchr(whitespace, str[n]) != NULL) str[n--] = 0;
  
  // Skip past the whitespace and find the start of the string
  for(n = 0; str[n] != 0; n++)
  {
    if (strchr(whitespace, str[n]) == NULL)
    {
      // Found a non-whitespace character. Shift the entire string back so that character is 
      // placed at position 0 in the string.
      int start = n;
      for(; str[n-1] != 0; n++) str[n-start] = str[n];
      break;
    }
  }
}

