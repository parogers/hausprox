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
#include <Wire.h>
#include "Arduino.h"

#include "utils.h"
#include "Clock.h"

/* The global clock instance */
Clock clock;

/* The real-time clock address on the I2C bus */
#define RTC_ADDRESS       B1101000

Clock::Clock()
{
  seconds = 0;
  minutes = 0;
  hours = 0;
  day = 0;
  month = 0;
  year = 0;
}

boolean Clock::update()
{
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(RTC_ADDRESS, 7);
  if (Wire.available() == 0) 
  {
    return false;
  }
  seconds = decodeBCD(Wire.read());
  minutes = decodeBCD(Wire.read());
  hours = decodeBCD(Wire.read());
  weekday = decodeBCD(Wire.read());
  day = decodeBCD(Wire.read());
  month = decodeBCD(Wire.read());
  year = decodeBCD(Wire.read());

  /* Make sure the time data makes sense */
  /* TODO - log inconsistent data */
  if (year > 99) year = 0;
  if (month > 12) month = 0;
  if (day > 31) day = 0;
  if (hours > 24) hours = 0;
  if (minutes > 60) minutes = 0;
  if (seconds > 60) seconds = 0;
  return true;
}

boolean Clock::setDateTime(char *buf)
{
  const char *delims = ":-/ ";
  char *str;
  
  str = strtok(buf, delims);
  if (str == NULL) return false;
  year = atoi(str);
  
  str = strtok(NULL, delims);
  if (str == NULL) return false;
  month = atoi(str);
  
  str = strtok(NULL, delims);
  if (str == NULL) return false;
  day = atoi(str);
  
  str = strtok(NULL, delims);
  if (str == NULL) return false;
  hours = atoi(str);
  
  str = strtok(NULL, delims);
  if (str == NULL) return false;
  minutes = atoi(str);
  
  str = strtok(NULL, delims);
  if (str == NULL) return false;
  seconds = atoi(str);
  
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write((byte)0);
  Wire.write(encodeBCD(seconds));
  Wire.write(encodeBCD(minutes));
  Wire.write(encodeBCD(hours));
  Wire.write((byte)0); // TODO - fix weekday
  Wire.write(encodeBCD(day));
  Wire.write(encodeBCD(month));
  Wire.write(encodeBCD(year));
  Wire.endTransmission();
  return true;
}

void Clock::formatDateTime(char *buf, int buflen)
{
  // Make sure the buffer is large enough to fit the data
  if (buflen >= 20) {
    sprintf(buf, "20%02d/%02d/%02d %02d:%02d:%02d ", year, month, day, hours, minutes, seconds);
  }
}

