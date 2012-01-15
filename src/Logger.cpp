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

/* Logger.cpp */

#include "utils.h"
#include "WProgram.h"
#include "Logger.h"
#include "CardReader.h"
//#include "Clock.h"

PROGMEM const prog_char strCardType[] = {"[CARD] "};
PROGMEM const prog_char strAdminType[] = {"[ADMN] "};
PROGMEM const prog_char strMessageType[] = {"[MESG] "};
PROGMEM const prog_char strErrorType[] = {"[ERRR] "};
PROGMEM const prog_char strDoorType[] = {"[DOOR] "};
PROGMEM const prog_char strSerialPart[]  = {", serial="};
PROGMEM const prog_char strBufferPart[] = {", buffer="};
PROGMEM const prog_char strSDInitFail[] = {"Failed to init the SD card"};
PROGMEM const prog_char strLogOpenFail[] = {"Failed to open serial log file"};

/* The global logger instance */
Logger logger;

Logger::Logger()
{
}

void Logger::begin(int sdPin, int rtcPin)
{
  /* Setup the RTC */
  //clock.begin(rtcPin);
  /* Setup the SD card */
  pinMode(sdPin, OUTPUT);
  if (! SD.begin(sdPin) ) {
    print_prog_str(&Serial, strSDInitFail);
    //logMessage(LOG_MESG, strSDInitFail);
  }
}

void Logger::logMessage(int level, const prog_char *msg)
{
  logMessage(level, msg, NULL, NULL);
}

void Logger::logMessage(int level, const prog_char *msg, const char *serial, CardReader *reader)
{
  /* Base the name of the log file on the current date */
  //clock.update();
  int year = 2011;
  int month = 10;
  int day = 04;
  int hours = 12;
  int minutes = 1;
  int seconds = 15;

  /* We need a buffer to hold the file name (8+1+3=12 chars+null) and later the 
   * timestamp string (20 chars+null) */
  char buf[22];
  /* Extract the last two digits of the year */
  int year2d = year - 100*(year/100);
  sprintf(buf, "hp-%d-%02d.log", year2d, month);

  File file = SD.open(buf, FILE_WRITE);
  if (!file) {
    print_prog_str(&Serial, strLogOpenFail);
  }
  
  // Write out the timestamp
  sprintf(buf, "%d/%02d/%02d %02d:%02d:%02d ", year, month, day, hours, minutes, seconds);
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
  print_prog_str(&Serial, strType);
  print_prog_str(&Serial, msg);

  if (file) {
    print_prog_str(&file, strType);
    print_prog_str(&file, msg);
  }

  if (serial != NULL) {
    print_prog_str(&Serial, strSerialPart);
    Serial.print(serial);
    
    if (file) {
      print_prog_str(&file, strSerialPart);
      file.print(serial);
    }
  }

  if (reader != NULL) {
    int n, numBits = reader->getBitsRead();
    print_prog_str(&Serial, strBufferPart);
    for (n = 0; n < numBits; n++) {
      Serial.print('0'+reader->getData(n), BYTE);
    }
    Serial.print('\n');
    if (file) {
      print_prog_str(&file, strBufferPart);
      for (n = 0; n < numBits; n++) {
        file.print('0'+reader->getData(n), BYTE);
      }
      file.print('\n');
    }
  }

  Serial.print('\n', BYTE);
  if (file) file.print('\n', BYTE);

  if (file) file.close();
}

