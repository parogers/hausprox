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

/* CardDatabase.cpp */

#include <SD.h>
#include "CardDatabase.h"
#include "utils.h"

PROGMEM const prog_char strDatabaseOpenFail[] = {"Failed to open card database\n"};
PROGMEM const prog_char strInvalidRecord[] = {"Invalid record in card database\n"};
PROGMEM const prog_char strRecordTooLong[] = {"Database record exceeds maximum length\n"};
PROGMEM const prog_char strRecordTooShort[] = {"Database record is too short\n"};

PROGMEM const prog_char strExpectingCardNum[] = {"Expecting card number\n"};
PROGMEM const prog_char strExpectingEnabledFlag[] = {"Expecting enabled flag\n"};
//PROGMEM const prog_char strInvalidFacilityCode[] = {"Invalid facility code\n"};
//PROGMEM const prog_char strInvalidCardNum[] = {"Invalid card number\n"};

/* The length of a line in the card database (serial+comma+enabled+newline) */
#define DB_LINE_LEN     (MAX_SERIAL_LEN+1+1+1)

CardDatabase::CardDatabase()
{
  recordLength = 12;
}

boolean CardDatabase::begin()
{
#if 0
  /* Make sure we can access the database file */
  File file = SD.open("cards.txt", FILE_READ);
  if (!file) {
    /* Register an error */
    return;
  }
  file.close();
#endif
}

boolean CardDatabase::parseCard(char *line, CardInfo &info)
{
    /* Tokenize the string */
    const char *delims = ",";
    
    /* Note that strtok either returns NULL, or returns a non-empty string */
    char *otherStr = strtok(line, delims);
    if (otherStr == NULL) {
      /* Expected card number */
      print_prog_str(&Serial, strExpectingCardNum);
      return false;
    }

    char *enabledStr = strtok(NULL, delims);
    if (enabledStr == NULL) {
      /* Expected enabled flag */
      print_prog_str(&Serial, strExpectingEnabledFlag);
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

boolean CardDatabase::lookupCard(char *serial, CardInfo &info)
{
  /* Load the database */
  File file = SD.open("cards.txt", FILE_READ);
  if (!file) {
    /* Register an error */
    /* ... */
    print_prog_str(&Serial, strDatabaseOpenFail);
    return false;
  }
  
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
  boolean found = false;

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
      print_prog_str(&Serial, strRecordTooShort);
      break;
    }

    if (buf[n-1] != '\n') {
      // Record is too long
      print_prog_str(&Serial, strRecordTooLong);
      break;
    }

    // Remove the newline from the string
    buf[n-1] = 0;

    if (! parseCard(buf, info) )
    {
        // Invalid record
        print_prog_str(&Serial, strInvalidRecord);
        break;
    }
    info.slot = count;

    if (strcmp(info.serial, serial) == 0)
    {
      /* Found this card in the database */
      found = true;
      break;
    }
    count++;
  }
  file.close();
  return found;
}

boolean CardDatabase::getCard(unsigned int slot, CardInfo &info)
{
}

boolean CardDatabase::putCard(unsigned int slot, CardInfo &info)
{
}

#if 0
boolean CardDatabase::lookupCard(unsigned int facility, unsigned int card, CardInfo &info)
{
  /* Load the database */
  File file = SD.open("cards.txt", FILE_READ);
  if (!file) {
    /* Register an error */
    /* ... */
    print_prog_str(&Serial, strDatabaseOpenFail);
    return false;
  }

  /* Database entries look like:
   *
   * facility,card,enabled
   */
  char buf[30];
  boolean found = true;

  while(1)
  {
    /* Read in the next card entry */
    int n = read_line(&file, buf, sizeof(buf));
    if (n == 0) {
      // Reached end of file
      found = false;
      break;
    }

    if (buf[n-1] == '\n') {
      if (n == 1) { 
        // Blank line
        continue;
      }
      // Remove the newline from the string
      buf[n-1] = 0;
    }
    
    if (buf[0] == '#') {
      // Comment line
      continue;
    }

    /* Tokenize the string */
    const char *delims = ", \t";

    /* Note that strtok either returns NULL, or returns a non-empty string */
    char *facilityStr = strtok(buf, delims);
    if (facilityStr == NULL) {
      /* Skip over blank lines */
      continue;
    }
    
    char *cardStr = strtok(NULL, delims);
    if (cardStr == NULL) {
      /* Expected card number */
      print_prog_str(&Serial, strExpectingCardNum);
      continue;
    }
    
    char *enabledStr = strtok(NULL, delims);
    if (enabledStr == NULL) {
      /* Expected enabled flag */
      print_prog_str(&Serial, strExpectingEnabledFlag);
      continue;
    }

    int otherFacility = atoi(facilityStr);
    long otherCard = atol(cardStr);
    
    if (otherFacility == 0) {
      /* Failed to parse the facility code */
      print_prog_str(&Serial, strInvalidFacilityCode);
      continue;
    }
    if (otherCard == 0) {
      /* Failed to parse the card number */
      print_prog_str(&Serial, strInvalidCardNum);
      continue;
    }
    
    if (facility == otherFacility && card == otherCard)
    {
      /* Found this card in the database */
      info.enabled = enabledStr[0] == '1';
      found = true;
      break;
    }
  }

  file.close();
  
  /* Card not found in database */
  return found;
}
#endif

