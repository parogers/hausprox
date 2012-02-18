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
#ifndef __CLOCK_H__
#define __CLOCK_H__

/* The interface to the realtime clock chip (DS3234). It uses the SPI library to interface with the bus. The
 * bus is shared with the SD card reader so we have to be careful when using it. */
class Clock
{
  public:
    Clock();
    
    /* The hours, minutes, seconds on the last call to 'update' */
    byte seconds;
    byte minutes;
    byte hours;
    byte weekday; // don't trust this value
    
    /* The date on the last call to 'update'. Note the year ranges 0-99. */
    byte day;
    byte month;
    byte year;
  
    /* Update the stored time to the current time on the RTC chip */
    boolean update();

    /* Sets the date and time on the RTC given an input string. The string should look 
     * like "YY-MM-DD HH:MM:SS". This function returns true if the string parses correctly, 
     * false otherwise. Note that 'buf' is modified in either case. */
    boolean setDateTime(char *buf);

    /* Format the date/time in the buffer. If the buffer is not long enough (minimum 20 chars)
     * this function does nothing. */
    void formatDateTime(char *buf, int len);
};

/* The global clock instance */
extern Clock clock;

#endif

