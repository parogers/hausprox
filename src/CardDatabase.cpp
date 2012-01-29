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

/* CardDatabase.cpp */

#include <SD.h>
#include "CardDatabase.h"
#include "utils.h"

/* Maximum record length, including the newline character */
#define RECORD_LINE_LEN     (MAX_SERIAL_LEN+1+1+1)

PROGMEM const prog_char strDatabaseSuccess[] = {"Success"};
PROGMEM const prog_char strDatabaseNotFound[] = {"Record not found"};
PROGMEM const prog_char strDatabaseOpenFail[] = {"Failed to open card DB"};
PROGMEM const prog_char strInvalidRecord[] = {"Invalid record in card DB"};
PROGMEM const prog_char strRecordTooLong[] = {"Record too long"};
PROGMEM const prog_char strRecordTooShort[] = {"Record too short"};
PROGMEM const prog_char strActive[] = {" - active"};
PROGMEM const prog_char strDisabled[] = {" - disabled"};

/* The length of a line in the card database (serial+comma+enabled+newline) */
#define RECORD_LEN     (SERIAL_LEN+1+1+1)

CardDatabase::CardDatabase()
{
}

int CardDatabase::readCard(File *file, CardInfo &info)
{
  /* Read in the next card entry */
  char buf[RECORD_LEN+1];
  int n = read_line(file, buf, sizeof(buf));

  if (n == 0) {
    // Reached end of file
    return DATABASE_EOF;
  }
  else if (buf[0] == '\n') {
    // Blank line of text
    return DATABASE_EOF;
  }

  if (n < RECORD_LEN) {
    // Bad database entry - record is not long enough
    // TODO - log an error
    //print_prog_str(&Serial, strRecordTooShort);
    return DATABASE_RECORD_TOO_SHORT;
  }

  if (buf[n-1] != '\n') {
    // Record is too long
    //print_prog_str(&Serial, strRecordTooLong);
    return DATABASE_RECORD_TOO_LONG;
  }

  // Remove the newline from the string
  buf[n-1] = 0;

  if (! parseCard(buf, info) )
  {
      // Invalid record
      //print_prog_str(&Serial, strInvalidRecord);
      return DATABASE_INVALID_RECORD;
  }
  return DATABASE_SUCCESS;
}

boolean CardDatabase::parseCard(char *line, CardInfo &info)
{
    /* Tokenize the string */
    const char *delims = ",";
    
    /* Note that strtok either returns NULL, or returns a non-empty string */
    char *otherStr = strtok(line, delims);
    if (otherStr == NULL) {
      /* Expected card number */
//      print_prog_str(&Serial, strExpectingCardNum);
      return false;
    }

    char *enabledStr = strtok(NULL, delims);
    if (enabledStr == NULL) {
      /* Expected enabled flag */
//      print_prog_str(&Serial, strExpectingEnabledFlag);
      return false;
    }

    // Trim the spaces off of the card serial number
    while (*otherStr == ' ') otherStr++;

    // Trim the end of the string as well
    char *ptr = otherStr + strlen(otherStr)-1;
    while (ptr > otherStr && *ptr == ' ') *ptr = 0;

    strcpy(info.serial, otherStr); // TODO - strncpy
    info.enabled = (enabledStr[0] == '1');
    return true;
}

int CardDatabase::lookupCard(char *serial, CardInfo &info)
{
  /* Load the database */
  File file = SD.open("cards.txt", FILE_READ);
  if (!file) {
    return DATABASE_OPEN_FAILURE;
  }
  
//  file.close();
//  return false;

  /* Database rows are fixed width, with data encoded in readable ascii. Note
   * every record must end in a newline character.
   *
   * serial,enabled\n       (serial=9 chars, enabled=1 char, plus newline)
   * ...
   */
  int count = 1;

  int ret;
  while(1)
  {
    // Read the next card entry
    ret = readCard(&file, info);
    
    // If we reach the end of file, the record wasn't found
    if (ret == DATABASE_EOF) {
      ret = DATABASE_DOES_NOT_EXIST;
      break;
    }
    // If we hit an error reading a record, pass that error back on return
    if (ret != DATABASE_SUCCESS) break;
    info.slot = count++;

    if (strcmp(info.serial, serial) == 0)
    {
      /* Found the card in the database */
      ret = DATABASE_SUCCESS;
      break;
    }
  }
  file.close();
  return ret;
}

boolean CardDatabase::getCard(unsigned int slot, CardInfo &info)
{
  File file = SD.open("cards.txt", FILE_READ);
  if (!file) {
    return false;
  }
  
  if (!file.seek((slot-1)*RECORD_LEN)) {
    file.close();
    return false;
  }
  
  int ret = readCard(&file, info);
  file.close();

  if (ret == DATABASE_SUCCESS) {
    info.slot = slot;
    return true;
  }
  return false;
}

int CardDatabase::putCard(unsigned int slot, CardInfo &info)
{
  return DATABASE_SUCCESS;
}

const prog_char *CardDatabase::getErrorStr(int code)
{
  switch(code) {
    case DATABASE_SUCCESS:
      return strDatabaseSuccess;
    case  DATABASE_OPEN_FAILURE:
      return strDatabaseOpenFail;
    case  DATABASE_RECORD_TOO_SHORT:
      return strRecordTooShort;
    case  DATABASE_RECORD_TOO_LONG:
      return strRecordTooLong;
    case  DATABASE_INVALID_RECORD:
      return strInvalidRecord;
    case  DATABASE_DOES_NOT_EXIST:
      return strDatabaseNotFound;
  };
  /* All other unknown errors (16-bit code, at most 5 digits, plus a sign,
   * plus '#', plus null) */
  char err[8];
  sprintf(err, "#%d", code);
  return err;
}

void CardDatabase::printRecords()
{
  /* Load the database */
  File file = SD.open("cards.txt", FILE_READ);
  if (!file) {
    println_prog_str(getErrorStr(DATABASE_OPEN_FAILURE));
    return;
  }

  CardInfo info;
  int count=1;
  while(1)
  {
    CardInfo info;
    int ret = readCard(&file, info);

    if (ret == DATABASE_EOF) break;
    if (ret != DATABASE_SUCCESS) {
      println_prog_str(getErrorStr(ret));
      break;
    }

    info.slot = count++;

    Serial.print('[');
    Serial.print((int)info.slot);
    Serial.print(']');
    Serial.print(' ');
    Serial.print(info.serial);
    if (info.enabled) {
      println_prog_str(strActive);
    } else {
      println_prog_str(strDisabled);
    }
  }
  file.close();
}

