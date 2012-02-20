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

/* Prox.cpp */

#include <Wire.h>
#include <SD.h>

#include "Prox.h"

/*************/
/* Constants */
/*************/

#define CLOCK             3
#define DATA              4
#define PRESENT           5
#define BEEP              6

#define DOOR_LATCH        2

#define OPEN_HOUSE_BTN    7

/* Chip select for the SD card */
#define SD_CHIPSEL        10

/***********/
/* Strings */
/***********/

/*PROGMEM const prog_char strWelcomeMessage[] = {
  "* * * * * * * * * * * * * * * * * * * * * *\n"\
  "Welcome to the haus|prox door access system\n"\
  "* * * * * * * * * * * * * * * * * * * * * *\n\n"\
  "Please login to continue\n\n"};*/
PROGMEM const prog_char strCardError[] = {"Error reading card"};
PROGMEM const prog_char strValidOpenHouse[] = {"Admit entry (open house active)"};
PROGMEM const prog_char strAdmitEntry[] = {"Admit entry"};
PROGMEM const prog_char strDenyDisabledCard[] = {"Deny disabled card"};
PROGMEM const prog_char strDenyUnregCard[] = {"Deny unregistered card"};
PROGMEM const prog_char strAdminDenied[] = {"Admin access denied"};
PROGMEM const prog_char strBootupMessage[] = {"haus|prox bootup"};
//PROGMEM const prog_char strLoginMessage[] = {"Admin login"};
//PROGMEM const prog_char strLoginPrompt[] = {"\nLogin: "};
PROGMEM const prog_char strOpenHouseOn[] = {"Turn on open house"};
PROGMEM const prog_char strOpenHouseOff[] = {"Turn off open house"};
PROGMEM const prog_char strOpenHouseExpired[] = {"Open house expired"};
PROGMEM const prog_char strDoorLocked[] = {"Door is locked"};
PROGMEM const prog_char strDoorUnlocked[] = {"Door is unlocked"};
PROGMEM const prog_char strDoorAlreadyUnlocked[] = {"Door is already unlocked"};
PROGMEM const prog_char strSDInitFail[] = {"Failed to init SD card"};
PROGMEM const prog_char strCardBuffer[] = {"Card buffer contents"};

/*********/
/* Class */
/*********/

HausProx::HausProx()
{
  /* Initalize globals to default values */
  openHouseMode = false;
  openHouseDuration = 3*60*60;
  doorEntryDuration = 10;
  lastDoorLocked = true;
//  strcpy(password, "123");
}

void HausProx::begin()
{
  /* Set the open house button pin and internal pull-up resistor */
  pinMode(OPEN_HOUSE_BTN, INPUT);
  digitalWrite(OPEN_HOUSE_BTN, HIGH);

  /* Initialize the I2C bus for interfacing with the clock (arduino is bus master) */
  Wire.begin();

  pinMode(SD_CHIPSEL, OUTPUT);

  /* Setup the card reader */
  reader.begin(DATA, CLOCK, PRESENT, BEEP);

  /* Setup the door control */
  door.begin(DOOR_LATCH);

  initSDCard();
  if (! sdEnabled ) {
    /* Log that the SD is not enabled. Of course, this won't log to the SD card but
     * it will write the message to the serial port. */
    logger.logMessage(LOG_ERROR, strSDInitFail);
  }
  
  // Log the bootup message
  logger.logMessage(LOG_MESG, strBootupMessage);

  /* The door starts locked */
  lock_door();

  // Have the reader make a short beep
  reader.setBeep(true);
  delay(500);
  reader.setBeep(false);
}

void HausProx::initSDCard()
{
  if (! SD.begin(SD_CHIPSEL) ) {
    sdEnabled = false;
  } else {
    sdEnabled = true;
  }
  /* Let the logger know if it's using the SD card or not */
  logger.sdEnabled = sdEnabled;
}

/* Locks the door and logs a message */
void HausProx::lock_door()
{
  logger.logMessage(LOG_DOOR, strDoorLocked);
  door.lock();
}

/* Unlocks the door for a period of time (in seconds) and logs a message */
void HausProx::unlock_door(long duration)
{
  if (! door.isLocked() ) {
    logger.logMessage(LOG_DOOR, strDoorAlreadyUnlocked);
  } else {
    logger.logMessage(LOG_DOOR, strDoorUnlocked);
  }
  /* Unlock the door for a period of time */
  door.unlock(duration);
}

void HausProx::handle_events()
{
  /* This is where we catch the door being locked and log a message. Locking happens either inside the 
   * timer interrupt, or by explicitly calling door.lock() when the open house button is pressed. This
   * code handles both of those cases. */
  boolean locked = door.isLocked();
  if (locked && !lastDoorLocked) 
  {
    if (openHouseMode) {
      /* Open house mode has expired */
      openHouseMode = false;
      logger.logMessage(LOG_DOOR, strOpenHouseExpired);
    } else {
      /* Log that the door has been locked again */
      logger.logMessage(LOG_DOOR, strDoorLocked);
    }
  }
  lastDoorLocked = locked;
  
  /* Check if a card has been scanned */
  handle_card_scanned();

  /* Check if the open|haus button has been pressed */
  handle_open_house();
}

/* Called to handle a card being scanned. The data is actually buffered up by the interrupt handler
 * attached to the clock pin. When sufficient data has been captured (255 bytes) this function 
 * will process the data, scan the database, etc. */
void HausProx::handle_card_scanned()
{
  if (!reader.hasCardData()) {
    // No data present
    return;
  }

  // Read the card data
  char serial[READER_SERIAL_BUF_LEN];
  int err = reader.readCard(serial, sizeof(serial));

  // Interpret the results
  if (err != 0) {
    /* Log the error and the contents of the card buffer */
    logger.logMessage(LOG_ERROR, CardReader::getErrorStr(err), NULL, &reader);
    // Clear the card buffer
    reader.clearCardData();
    return;
  }

//  logger.logMessage(LOG_CARD, strCardBuffer, NULL, &reader);

  // Clear the card buffer
  reader.clearCardData();

  /* Scan the database */
  CardInfo info;
  int ret = database.lookupCard(serial, info);

  if (ret == DATABASE_DOES_NOT_EXIST) {
    /* The card isn't in the database */
    reader.playFailBeep();
    logger.logMessage(LOG_CARD, strDenyUnregCard, serial);
    return;

  } else if (ret != DATABASE_SUCCESS) {
    /* Log the error */
    logger.logMessage(LOG_ERROR, database.getErrorStr(ret), serial);
    return;
  }

  if (info.enabled) 
  {
    /* Card holder is granted access */
    if (openHouseMode) {
      /* Already in open house mode, so whatever */
      logger.logMessage(LOG_CARD, strValidOpenHouse, serial);
    } else {
      /* Log the entry message */
      logger.logMessage(LOG_CARD, strAdmitEntry, serial);
      unlock_door(doorEntryDuration);
    }
  } else {
    /* The card is disabled */
    reader.playFailBeep();
    logger.logMessage(LOG_CARD, strDenyDisabledCard, serial);
  }
}

/* Handles the open house button being pressed */
void HausProx::handle_open_house()
{
  /* Update the open house toggle button (handles debouncing) */
  boolean n = (digitalRead(OPEN_HOUSE_BTN) == LOW);
  openHouseButton.update(n);
  /* Check if somebody has pressed the button (goes low to high) */
  if (openHouseButton.changed && openHouseButton.state) 
  {
    /* Toggle open house mode */
    openHouseMode = !openHouseMode;
    if (!openHouseMode) {
      // Turn off open house mode
      lock_door();
      logger.logMessage(LOG_DOOR, strOpenHouseOff);
    } else {
      // Turn on open house mode
      unlock_door(openHouseDuration);
      logger.logMessage(LOG_DOOR, strOpenHouseOn);
    }
  }
}

