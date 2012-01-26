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

/* CardDatabase.h */

#ifndef __CARD_DATABASE_H__
#define __CARD_DATABASE_H__

#include "Arduino.h"
#include <SD.h>

#define DATABASE_FOUND               0
#define DATABASE_OPEN_FAILURE       -1
#define DATABASE_RECORD_TOO_SHORT   -2
#define DATABASE_RECORD_TOO_LONG    -3
#define DATABASE_INVALID_RECORD     -4
#define DATABASE_DOES_NOT_EXIST     -5

/* Maximum card serial number length in chars */
#define MAX_SERIAL_LEN       16

class CardInfo
{
  public:
    char serial[MAX_SERIAL_LEN+1];
    unsigned int slot;
    boolean enabled;
};

class CardDatabase
{
  private:
    /* The length of a record in the database (including the newline character) */
    int recordLength;
    /* The error code (zero = no error) */
    int error;

    boolean parseCard(char *line, CardInfo &info);

  public:
    CardDatabase();

    static const prog_char *getErrorStr(int code);

    boolean begin();

    /* Lookup a card in the database. Fills information in 'info'
     * and returns DATABASE_SUCCESS if the card is found, otherwise 
     * leaves info unchanged and returns the error code. */
    int lookupCard(char *serial, CardInfo &info);

    /* Retreive a card given the slot number */
    boolean getCard(unsigned int slot, CardInfo &info);

    /* Saves a card in the database */
    boolean putCard(unsigned int slot, CardInfo &info);

    /* Inserts card data into the database at the first empty slot, or appended if
     * there are no slots available. */
    boolean insertCard(CardInfo &info);

    /* Lookup a card in the database. Fills information in 'info'
     * and returns true if the card is found, otherwise leaves
     * info unchanged and returns false. */    
//    boolean lookupCard(unsigned int facilty, unsigned int card, CardInfo &info);
};

#endif

