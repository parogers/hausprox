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
#define MAX_RECORD_LEN     (MAX_SERIAL_LEN+1+1+1)

PROGMEM const prog_char strDatabaseRecordFound[] = {"Record found"};
PROGMEM const prog_char strDatabaseNotFound[] = {"Record not found"};
PROGMEM const prog_char strDatabaseOpenFail[] = {"Failed to open card DB"};
PROGMEM const prog_char strInvalidRecord[] = {"Invalid record in card DB"};
PROGMEM const prog_char strRecordTooLong[] = {"Record too long"};
PROGMEM const prog_char strRecordTooShort[] = {"Record too short"};

/* The length of a line in the card database (serial+comma+enabled+newline) */
#define DB_LINE_LEN     (MAX_SERIAL_LEN+1+1+1)

CardDatabase::CardDatabase()
{
  recordLength = 12;
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
    /* Register an error */
    /* ... */
    //print_prog_str(&Serial, strDatabaseOpenFail);
    return DATABASE_OPEN_FAILURE;
  }
  int ret = DATABASE_DOES_NOT_EXIST;
  
//  file.close();
//  return false;

  /* Database rows are fixed width, with data encoded in readable ascii. Note
   * every record must end in a newline character.
   *
   * serial,enabled\n       (serial=9 chars, enabled=1 char, plus newline)
   * ...
   */
  int count = 0;
  char buf[MAX_RECORD_LEN+1];

  while(1)
  {
    /* Read in the next card entry */
    int n = read_line(&file, buf, recordLength+1);

    if (n == 0) {
      // Reached end of file
      break;
    }
    else if (buf[0] == '\n') {
      // Blank line of text
      break;
    }

//    Serial.print("read: ");
//    Serial.println(buf);

    if (n < recordLength) {
      // Bad database entry - record is not long enough
      // TODO - log an error
      //print_prog_str(&Serial, strRecordTooShort);
      ret = DATABASE_RECORD_TOO_SHORT;
      break;
    }

    if (buf[n-1] != '\n') {
      // Record is too long
      //print_prog_str(&Serial, strRecordTooLong);
      ret = DATABASE_RECORD_TOO_LONG;
      break;
    }

    // Remove the newline from the string
    buf[n-1] = 0;

    if (! parseCard(buf, info) )
    {
        // Invalid record
        //print_prog_str(&Serial, strInvalidRecord);
        ret = DATABASE_INVALID_RECORD;
        break;
    }
    info.slot = count;

    if (strcmp(info.serial, serial) == 0)
    {
      /* Found this card in the database */
      ret = DATABASE_FOUND;
      break;
    }
    count++;
  }
  file.close();
  return ret;
}

boolean CardDatabase::getCard(unsigned int slot, CardInfo &info)
{
}

boolean CardDatabase::putCard(unsigned int slot, CardInfo &info)
{
}

const prog_char *CardDatabase::getErrorStr(int code)
{
  switch(code) {
    case DATABASE_FOUND:
      return strDatabaseRecordFound;
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

