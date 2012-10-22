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
#include "Const.h"

/* The global logger instance */
Logger logger;

Logger::Logger()
{
  sdEnabled = false;
  serialLogging = true;
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
  sprintf(buf, "hp-%02d-%02d.log", year2d, clock.month);

  /* Log to a file if the SD card is enabled */
  File file;

  if (sdEnabled) {
    file = SD.open(buf, FILE_WRITE);
  }
  
//  if (!file) {
//    print_prog_str(&Serial, strLogOpenFail);
//  }
  
  // Write out the timestamp
  clock.formatDateTime(buf, sizeof(buf));
  if (serialLogging) {
    Serial.print(buf);
  }
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
  if (serialLogging) {
    print_prog_str(strType);
    print_prog_str(msg);
  }

  if (file) {
    print_prog_str(&file, strType);
    print_prog_str(&file, msg);
  }

  if (serial != NULL) {
    if (serialLogging) {
      print_prog_str(strSerialPart);
      Serial.print(serial);
    }
    
    if (file) {
      print_prog_str(&file, strSerialPart);
      file.print(serial);
    }
  }

  if (reader != NULL) {
    if (serialLogging) {
      // Print the card buffer contents to the serial port
      reader->printBuffer(Serial);
    }
    if (file) {
      // Print the card buffer to the log file
      reader->printBuffer(file);
    }
  }

  if (serialLogging) {
    Serial.println();
  }
  if (file) {
    file.print('\n');
    file.close();
  }
}

