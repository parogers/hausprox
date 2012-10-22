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
#include "Const.h"

/* The length of a line in the card database (serial+comma+enabled+newline) */
#define RECORD_LEN     (SERIAL_LEN+1+1+1)

/* The name of the card database */
#define DB_FILE        "cards.txt"

/* The character to use when blanking out card records */
#define BLANK_CHAR     'Z'

/***********/
/* Globals */
/***********/

/* A buffer for storing record data temporarily */
char recordBuf[RECORD_LEN+1];

CardInfo cardInfo;

/*************/
/* Functions */
/*************/

CardDatabase::CardDatabase()
{
}

int CardDatabase::readCard(File *file, CardInfo &info)
{
  /* Read in the next card entry */
  int n = read_line(file, recordBuf, sizeof(recordBuf));

  if (n == 0) {
    // Reached end of file
    return DATABASE_EOF;
  }
  else if (recordBuf[0] == '\n') {
    // Blank line of text
    return DATABASE_EOF;
  }

  if (n < RECORD_LEN) {
    // Bad database entry - record is not long enough
    // TODO - log an error
    //print_prog_str(&Serial, strRecordTooShort);
    return DATABASE_RECORD_TOO_SHORT;
  }

  if (recordBuf[n-1] != '\n') {
    // Record is too long
    //print_prog_str(&Serial, strRecordTooLong);
    return DATABASE_RECORD_TOO_LONG;
  }

  // Remove the newline from the string
  recordBuf[n-1] = 0;

  if (! parseCard(recordBuf, info) )
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
      return false;
    }

    char *enabledStr = strtok(NULL, delims);
    if (enabledStr == NULL) {
      /* Expected enabled flag */
      return false;
    }

    // Trim the spaces off of the card serial number
    //while (*otherStr == ' ') otherStr++;

    // Trim the end of the string as well
//    char *ptr = otherStr + strlen(otherStr)-1;
//    while (ptr > otherStr && *ptr == ' ') *ptr = 0;

    strcpy(info.serial, otherStr); // TODO - strncpy
    info.enabled = (enabledStr[0] == '1');
    return true;
}

int CardDatabase::lookupCard(char *serial, CardInfo &info)
{
  if (!SD.exists(DB_FILE)) {
    return DATABASE_DOES_NOT_EXIST;
  }
  
  /* Load the database */
  File file = SD.open(DB_FILE, FILE_READ);
  if (!file) {
    return DATABASE_OPEN_FAILURE;
  }
  
  /* Database rows are fixed width, with data encoded in readable ascii. Note
   * every record must end in a newline character.
   *
   * serial,enabled\n       (serial=9 chars, enabled=1 char, plus newline)
   * ...
   */
  int count = 0;

  int ret = DATABASE_EOF;
  while(1)
  {
    // Read the next card entry
    ret = readCard(&file, cardInfo);
    
    // If we reach the end of file, the record wasn't found
    if (ret == DATABASE_EOF) {
      ret = DATABASE_RECORD_NOT_FOUND;
      break;
    }
    // If we hit an error reading a record, pass that error back on return
    if (ret != DATABASE_SUCCESS) break;
    cardInfo.slot = count++;

    if (strcmp(cardInfo.serial, serial) == 0)
    {
      /* Found the card in the database */
      ret = DATABASE_SUCCESS;
      info = cardInfo;
      break;
    }
  }
  file.close();
  return ret;
}

int CardDatabase::getCard(unsigned int slot, CardInfo &info)
{
  File file = SD.open(DB_FILE, FILE_READ);
  if (!file) {
    return DATABASE_OPEN_FAILURE;
  }
  
  unsigned long size = file.size();
  unsigned long off = slot*RECORD_LEN;
  
  /* Make sure we don't jump past the end of the file - since seek doesn't seem to check for that */
  if (off >= size || !file.seek(off)) {
    file.close();
    return DATABASE_EOF;
  }
  
  // Read the card record
  int ret = readCard(&file, info);
  file.close();

  if (ret == DATABASE_SUCCESS) {
    info.slot = slot;
  }
  return ret;
}

int CardDatabase::putCard(unsigned int slot, CardInfo &info)
{
  /* Verify the serial number is okay */
  if (strlen(info.serial) != SERIAL_LEN) {
    return DATABASE_INVALID_RECORD;
  }

  /* Load the database */
  File file = SD.open(DB_FILE, FILE_WRITE);
  if (!file) {
    return DATABASE_OPEN_FAILURE;
  }
  
  unsigned long size = file.size();
  unsigned long off;
 
  if (slot == -1) {
    /* Append the record */
    off = size;
  } else {
    /* Overwrite an existing record */
    off = slot*RECORD_LEN;
  }
  
//  Serial.println(off);
//  Serial.println(size);
//  Serial.println(slot);

  if (off > size || !file.seek(off)) {
    file.close();
    return DATABASE_EOF;
  }
  
  /* Format the fixed-length record and write it out */
  sprintf(recordBuf, "%9s,%c\n", info.serial, info.enabled ? '1' : '0');
  file.write(recordBuf);

  /* Close up the file again */
  file.close();

  return DATABASE_SUCCESS;
}

int CardDatabase::insertCard(CardInfo &info)
{
  CardInfo tmp;

  // Make sure the serial doesn't already exist
  strcpy(tmp.serial, info.serial);
  int ret = lookupCard(tmp.serial, tmp);
  if (ret == DATABASE_SUCCESS) {
    return DATABASE_ALREADY_EXISTS;
  }

  // Now try to insert the new card into a blank slot
  tmp.setBlank();
  
  ret = lookupCard(tmp.serial, tmp);
  if (ret == DATABASE_SUCCESS) {
    // Overwrite the blank record
    return putCard(tmp.slot, info);
  }
  // No blank spots so append the record to the file instead
  return putCard(-1, info);
}

const prog_char *CardDatabase::getErrorStr(int code)
{
  switch(code) {
    case DATABASE_SUCCESS:
      return strSuccess;
    case  DATABASE_OPEN_FAILURE:
      return strDatabaseOpenFail;
    case  DATABASE_RECORD_TOO_SHORT:
      return strRecordTooShort;
    case  DATABASE_RECORD_TOO_LONG:
      return strRecordTooLong;
    case  DATABASE_INVALID_RECORD:
      return strInvalidRecord;
    case  DATABASE_RECORD_NOT_FOUND:
      return strDatabaseNotFound;
    case DATABASE_EOF:
      return strDatabaseEOF;
    case DATABASE_DOES_NOT_EXIST:
      return strDatabaseDoesNotExist;
    case DATABASE_ALREADY_EXISTS:
      return strSerialExists;
  };
  return strDatabaseFailure;
}

int CardDatabase::enumerateRecords(CardCallback func)
{
  CardInfo info;
  /* Open the database */
  File file = SD.open(DB_FILE, FILE_READ);
  if (!file) {
    return DATABASE_OPEN_FAILURE;
  }

  int count=1;
  while(1)
  {
    // Read in the next card info and print it
    int ret = readCard(&file, info);

    if (ret == DATABASE_EOF) break;
    if (ret != DATABASE_SUCCESS) {
      file.close();
      return ret;
    }
    info.slot = count++;
    func(info);
  }
  file.close();
  return DATABASE_SUCCESS;
}

/************/
/* CardInfo */
/************/

void CardInfo::setBlank()
{
  // Fill the serial number with zeros
  for (int c=0; c < SERIAL_LEN; c++) {
    serial[c] = BLANK_CHAR;
  }
  // Terminate with a null character. Note that the serial buffer is one char longer
  // than 'SERIAL_LEN', so doing this is okay.
  serial[SERIAL_LEN] = 0;
  enabled = 0;
}

boolean CardInfo::isBlank()
{
  for (int c=0; c < SERIAL_LEN; c++) {
    if (serial[c] != BLANK_CHAR) return false;
  }
  return true;
}

