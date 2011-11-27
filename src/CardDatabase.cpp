/*
haus|prox - Electronic door access control system
Copyright (C) 2011  Peter Rogers @thinkhaus

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* CardDatabase.cpp */

#include <SD.h>
#include "CardDatabase.h"
#include "utils.h"

PROGMEM const prog_char strDatabaseOpenFail[] = {"Failed to open card database\n"};
PROGMEM const prog_char strExpectingCardNum[] = {"Expecting card number\n"};
PROGMEM const prog_char strExpectingEnabledFlag[] = {"Expecting enabled flag\n"};
PROGMEM const prog_char strInvalidFacilityCode[] = {"Invalid facility code\n"};
PROGMEM const prog_char strInvalidCardNum[] = {"Invalid card number\n"};

CardDatabase::CardDatabase()
{
}

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

