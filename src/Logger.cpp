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

/* Logger.cpp */

#include "utils.h"
#include "Arduino.h"
#include "Logger.h"
#include "CardReader.h"
#include "Clock.h"

PROGMEM const prog_char strCardType[] = {"[CARD] "};
PROGMEM const prog_char strAdminType[] = {"[ADMN] "};
PROGMEM const prog_char strMessageType[] = {"[MESG] "};
PROGMEM const prog_char strErrorType[] = {"[ERRR] "};
PROGMEM const prog_char strDoorType[] = {"[DOOR] "};
PROGMEM const prog_char strSerialPart[]  = {", serial="};
PROGMEM const prog_char strBufferPart[] = {", buffer="};
PROGMEM const prog_char strLogOpenFail[] = {"Failed to open SD log file\n"};

/* The global logger instance */
Logger logger;

Logger::Logger()
{
  sdEnabled = false;
}

void Logger::logMessage(int level, const prog_char *msg)
{
  logMessage(level, msg, NULL, NULL);
}

void Logger::logMessage(int level, const prog_char *msg, const char *serial, CardReader *reader)
{
  /* Get the current time from our chip */
  clock.update();

  /* We need a buffer to hold the file name (8+1+3=12 chars+null) and later the 
   * timestamp string (20 chars+null) */
  char buf[22];
  /* Extract the last two digits of the year */
  int year2d = clock.year - 100*(clock.year/100);
  sprintf(buf, "hp-%d-%02d.log", year2d, clock.month);

  /* Log to a file if the SD card is enabled */
  File file;

  if (sdEnabled) {
    file = SD.open(buf, FILE_WRITE);
    if (!sdEnabled) {
      /* Register an error */
    }
  }
  
//  if (!file) {
//    print_prog_str(&Serial, strLogOpenFail);
//  }
  
  // Write out the timestamp
  clock.formatDateTime(buf, sizeof(buf));
  Serial.print(buf);
  if (file) {
    file.print(buf);
  }

  // Write out the message type
  const prog_char *strType=NULL;
  switch(level) {
    case LOG_CARD:
      strType = strCardType;
      break;
    case LOG_ADMIN:
      strType = strAdminType;
      break;
    case LOG_ERROR:
      strType = strErrorType;
      break;
    case LOG_DOOR:
      strType = strDoorType;
      break;
    default:
      strType = strMessageType;
      break;
  }
  print_prog_str(strType);
  print_prog_str(msg);

  if (file) {
    print_prog_str(&file, strType);
    print_prog_str(&file, msg);
  }

  if (serial != NULL) {
    print_prog_str(strSerialPart);
    Serial.print(serial);
    
    if (file) {
      print_prog_str(&file, strSerialPart);
      file.print(serial);
    }
  }

  if (reader != NULL) {
    int n, numBits = reader->getBitsRead();
    char ch;
    print_prog_str(strBufferPart);
    for (n = 0; n < numBits; n++) 
    {
      if (reader->getData(n) == 1) ch = '1';
      else ch = '0';
      Serial.print(ch);
    }
    Serial.print('\n');
    if (file) {
      print_prog_str(&file, strBufferPart);
      for (n = 0; n < numBits; n++) 
      {
        if (reader->getData(n) == 1) ch = '1';
        else ch = '0';
        file.print(ch);
      }
      file.print('\n');
    }
  }

  Serial.print('\n');
  if (file) {
    file.print('\n');
    file.close();
  }
}

