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

#ifndef __CARD_READER__
#define __CARD_READER__

#include <avr/pgmspace.h>
#include "WProgram.h"

#define CARD_SUCCESS              0
#define CARD_PREMATURE_END       -1
#define CARD_PARITY_FAILURE      -2
#define CARD_INVALID_START       -3
#define CARD_LRC_PARITY_FAILURE  -4
#define CARD_LRC_FAILURE         -5
#define CARD_TRAILING_ZEROS      -6
#define CARD_PAD_FAILURE         -7
#define CARD_LEADING_ZEROS       -8

#define CARD_BUFFER_LEN         255
#define CARD_NUM_BITS           255

class CardReader
{
  private:
    int dataPin;
    int clockPin;
    int presentPin;
    /* Number of bits read after calling readCard */
    int bitsRead;

    unsigned char data[CARD_BUFFER_LEN];
    int bufferPos;
    int bytePos;
    int bitPos;

    boolean appendData(int bit);
    
  public:
    CardReader(int data, int clock, int present);

    static const prog_char *getErrorStr(int code);

    /* Reads the card present pin to determine if a card is being swipped */
    //boolean isCardPresent();

    /* Reads card data and returns the facility ID and card ID. On success
     * this function returns 0, otherwise it returns the error code. */
    int readCard(unsigned int &facility, unsigned int &card);

    /* Returns the number of bits read after a call to readCard. This is useful 
     * for identifying phantom card reads caused by spurious noise on the card 
     * present line. */
    //int getBitsRead() { return bitsRead; }
    
    void receiveCardData();

    void clearCardData();

    boolean hasCardData();

    int getData(int pos);
    int getBitsRead() { return bitsRead; }
};

#endif

