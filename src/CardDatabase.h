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

#define DATABASE_SUCCESS             0
#define DATABASE_OPEN_FAILURE       -1
#define DATABASE_RECORD_TOO_SHORT   -2
#define DATABASE_RECORD_TOO_LONG    -3
#define DATABASE_INVALID_RECORD     -4
#define DATABASE_RECORD_NOT_FOUND   -5
#define DATABASE_EOF                -6
#define DATABASE_DOES_NOT_EXIST     -7
#define DATABASE_ALREADY_EXISTS     -8

#define SERIAL_LEN                  (3+1+5)

// Card serial number type
typedef char serial_t[SERIAL_LEN+1];

// Represents a single card in the database
class CardInfo
{
  public:
    serial_t serial;
    // The 'slot' the card occupies in the database (starts at 0)
    unsigned int slot;
    boolean enabled;
    
    /* Set the card info to represent a blank record (ie deleted) */
    void setBlank();
    
    /* Returns whether this card info represents a blank record */
    boolean isBlank();
};

typedef void (*CardCallback)(CardInfo&);

/* Interface to the card number database. Card records are stored as fixed-length ascii strings
 * for random access and ease of debugging. See 'docs/Database.txt' for more information. */
class CardDatabase
{
  private:
    /* The error code (zero = no error) */
    int error;

    /* Reads a card record from the file stream. On success, this function returns DATABASE_SUCCESS and
     * fills in 'info'. Otherwise it returns one of the DATABASE_* error codes */
    int readCard(File *file, CardInfo &info);
    boolean parseCard(char *line, CardInfo &info);

  public:
    CardDatabase();

    static const prog_char *getErrorStr(int code);

    /* Lookup a card in the database. Fills information in 'info'
     * and returns DATABASE_SUCCESS if the card is found, otherwise 
     * leaves info unchanged and returns the error code. */
    int lookupCard(char *serial, CardInfo &info);

    /* Retreive a card given the slot number (index starts at 0) */
    int getCard(unsigned int slot, CardInfo &info);

    /* Saves a card in the database (index from 0) */
    int putCard(unsigned int slot, CardInfo &info);

    /* Inserts card data into the database at the first empty slot, or appended if
     * there are no slots available. Returns the slot number occuped by the record,
     * or the error code upon failure. */
    int insertCard(CardInfo &info);

    /* Enumerates the records in the card database, calling 'func' for each record */
    int enumerateRecords(CardCallback func);

};

#endif

