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

/* CardReader.cpp */
/* 
 * The card data is encoded as follows:
 *
 * 25 zeros  |                 | 168 zeros
 * 000...000 | SEG SEG ... SEG | 000...000
 *
 * The segments are 5 bits long each, 4 bits of data with one bit for (odd) parity.
 *
 *   SEG = d0 d1 d2 d3 P
 *
 * The segments encode a block stream:
 *
 *   BLOCK = d3 d2 d1 d0 (MSB first)
 * 
 * The stream of BLOCKs looks like:
 *
 *   START DATA DATA ... DATA END LRC
 *
 *   START = 1011 = 0xB (starting block)
 *   END   = 1111 = 0xF (finishing block)
 *   LRC   = LRC checksum on entire block stream
 *
 * The stream of DATA blocks further encode a PAYLOAD stream:
 *
 *   DATA    = 0 PAYLOAD (the MSB of DATA is always zero)
 *   PAYLOAD = b2 b1 b0
 * 
 * The entire PAYLOAD sequence contains the facility code, card number and two parity bits:
 *
 *           | 8 bits   | 16 bits     |
 *  JUNK | P | FACILITY | CARD NUMBER | P
 *
 * Only the final 26 bits of the sequence are considered, the rest are ignored (JUNK).
 *
 */

#include "Arduino.h"
#include "CardReader.h"

// Macro to verify odd parity
#define ODD_PARITY(d0,d1,d2,d3,parity)   (((d0)+(d1)+(d2)+(d3)+(parity)) % 2 == 1)

PROGMEM const prog_char strSuccess[] = {"OK"};
PROGMEM const prog_char strPrematureEnd[] = {"Premature end of data"};
PROGMEM const prog_char strParityFail[] = {"Parity failure"};
PROGMEM const prog_char strInvalidBegin[] = {"Invalid start segment"};
PROGMEM const prog_char strLRCParity[] = {"LRC parity failure"};
PROGMEM const prog_char strLRCFail[] = {"LRC failure"};
PROGMEM const prog_char strTrailingZeros[] = {"Expected trailing zeros"};
PROGMEM const prog_char strPaddingFail[] = {"Data pad fail"};
PROGMEM const prog_char strLeadingZeros[] = {"Leading zeros expected"};

/**************/
/* CardReader */
/**************/

CardReader::CardReader()
{
  bitsRead = 0;
}

void CardReader::begin(int data, int clock, int present, int beep)
{
  dataPin = data;
  clockPin = clock;
  presentPin = present;
  beepPin = beep;
  // Set the pins to input mode
  pinMode(clockPin, INPUT);
  pinMode(dataPin, INPUT);
  pinMode(presentPin, INPUT);
  pinMode(beepPin, OUTPUT);
  // Turn off beep by default
  setBeep(false);
  clearCardData();
}

/*
boolean CardReader::isCardPresent()
{
  return (digitalRead(presentPin) == LOW);
}*/

int CardReader::readCard(unsigned int &facility, unsigned int &card)
{
    int data = 0, d0, d1, d2, d3, parity, n;
    int pos = 0;

    /* Skip the first 25 bits of data (make sure they are zeros) */
    for (n = 0; n < 25; n++)
    {
        data = getData(pos++);
        if (data != 0) {
            /* Error */
            return CARD_LEADING_ZEROS;
            //Serial.println("leading zeros expected\n");
        }
    }

    /* Read in groups of 5-bits */
    boolean start = true;
    int startpos = -1;
    /* The variable which will hold the facility code (8 bits) and 
     * the card number (16 bits) */
    unsigned long result = 0;
    while (1) 
    {
        /* The bits are sent LSB first */
        d0 = getData(pos++);
        d1 = getData(pos++);
        d2 = getData(pos++);
        d3 = getData(pos++);
        parity = getData(pos++);

        if (d0 == -1 || d1 == -1 || d2 == -1 || d3 == -1 || parity == -1) {
          /* The data ended without seeing a 0xF segment */
          return CARD_PREMATURE_END;
        }

        /* Verify the parity bit (odd parity) */
        if (!ODD_PARITY(d0,d1,d2,d3,parity)) {
          //Serial.println("parity failure on 5-bit chunk\n");
          return CARD_PARITY_FAILURE;
        }

        if (start) {
            /* The first segment should be 0xB == 1011B */
            if (! (d0 & d1 & !d2 & d3) ) {
              /* Invalid starting segment */
              return CARD_INVALID_START;
            }
            startpos = pos;
            start = false;
            continue;
        } 

        /* Check for the ending segment 0xF == 1111B */
        if (d0 & d1 & d2 & d3)
        {
            /* End of data sequence, the LRC follows */
            d0 = getData(pos++);
            d1 = getData(pos++);
            d2 = getData(pos++);
            d3 = getData(pos++);
            parity = getData(pos++);
            if (!ODD_PARITY(d0,d1,d2,d3,parity)) {
                //Serial.println("LRC parity failure\n");
                return CARD_LRC_PARITY_FAILURE;
            }

            /* The parity for LRC checks out, verify the actual LRC calculation */

            /* Consume the remaining zeros */
            while (1) {
                data = getData(pos++);
                if (data == -1) {
                    break;
                }
                if (data != 0) {
                  //Serial.println("trailing data must be zeros\n");
                  return CARD_TRAILING_ZEROS;
                }
            }
            /* That is the end of the card data stream */
            break;
        }

        if (d3 != 0) {
            //Serial.println("data segments must have zero padding\n");
            return CARD_PAD_FAILURE;
        }

        /* Shift another 3 bits into the result buffer */
        result = (result << 1) | d2;
        result = (result << 1) | d1;
        result = (result << 1) | d0;
    }

    /* Mask out everything except the final 26 bits (LSB) */
    //result = result & 0x3FFFFFFL;

    /* Check the data parity (first and last bits) */

    // Shift out the end parity bit. We can ignore the upper parity bit
    result >>= 1;
    // Mask out the upper parity bit (not really necessary)
    //result = result & 0xFFFFFF;
    
    /* Extract the facility ID */
    facility = (result >> 16) & 0xFF;
    /* Extract the card ID */
    card = result & 0xFFFF;
    return CARD_SUCCESS;
}

int CardReader::readCard(char *serial, int maxlen)
{
  unsigned int facility;
  unsigned int card;

  /* Make sure the serial buffer is large enough */
  if (maxlen < READER_SERIAL_BUF_LEN) {
    return CARD_BUFFER_TOO_SMALL;
  }

  /* Read the facility and card numbers */
  int ret = readCard(facility, card);
  if (ret != CARD_SUCCESS) {
    return ret;
  }

  /* Translate the card data into a serial number of the form "facility-card". */
  sprintf(serial, "%03u-%05u", facility, card);
  return CARD_SUCCESS;
}

void CardReader::setBeep(boolean b) 
{
  digitalWrite(beepPin, b ? LOW : HIGH);
}

void CardReader::playFailBeep()
{
  int n;
  for(n = 0; n < 3; n++)
  {
    setBeep(true);
    delay(200);
    setBeep(false);
    delay(100);
  }
}

const prog_char *CardReader::getErrorStr(int code)
{
  switch(code) {
    case CARD_SUCCESS:
      return strSuccess;
    case CARD_PREMATURE_END:
      return strPrematureEnd;
    case CARD_PARITY_FAILURE:
      return strParityFail;
    case CARD_INVALID_START:
      return strInvalidBegin;
    case CARD_LRC_PARITY_FAILURE:
      return strLRCParity;
    case CARD_LRC_FAILURE:
      return strLRCFail;
    case CARD_TRAILING_ZEROS:
      return strTrailingZeros;
    case CARD_PAD_FAILURE:
      return strPaddingFail;
    case CARD_LEADING_ZEROS:
      return strLeadingZeros;
  }
  /* All other unknown errors (16-bit code, at most 5 digits, plus a sign,
   * plus '#', plus null) */
  char err[8];
  sprintf(err, "#%d", code);
  return err;
}

void CardReader::receiveCardData()
{
  if (bitsRead == CARD_BUFFER_LEN)
  {
    /* The card buffer is full, signal an error */
    /* ... */
  }
  else if (digitalRead(presentPin) == LOW)
  {
    appendData(digitalRead(dataPin));
  }
}

void CardReader::clearCardData() 
{
  bitsRead = 0;
  bufferPos = 0;
  bytePos = 0;
  bitPos = 0;
  // Zero out the card buffer
  memset(data, 0, CARD_BUFFER_LEN);
}

boolean CardReader::hasCardData() 
{
  return bitsRead == CARD_NUM_BITS;
}

boolean CardReader::appendData(int bit)
{
  if (bufferPos >= CARD_BUFFER_LEN) {
    return false;
  }
  data[bufferPos++] = bit;
  bitsRead++;
/*    
  data[bytePos] |= bit << bitPos;
  bitPos++;
  if (bitPos > 7) {
    bitPos = 0;
    bytePos++;
  }*/
  return true;
}

int CardReader::getData(int pos)
{
  if (pos < CARD_BUFFER_LEN)
    return 1-data[pos];
  return -1;
  /* 
  byte = pos/8;
  bit = pos%8;
  if (byte < CARD_BUFFER_LEN) {
    return 1-((data[byte] >> bit)&0x1);
  }
  return -1;
  */
}

