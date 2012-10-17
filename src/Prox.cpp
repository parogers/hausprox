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
#include "Const.h"

/*************/
/* Constants */
/*************/

#define PIN_CLOCK             3
#define PIN_DATA              4
#define PIN_PRESENT           5
#define PIN_BEEP              6

#define PIN_DOOR_LATCH        2

#define PIN_OPEN_HOUSE_BTN    7

/* Chip select for the SD card */
#define PIN_SD_CHIPSEL        10

/* The hausprox config file */
#define CONFIG_FILE       "hausprox.cfg"

#define DEFAULT_PASSWORD        "123"
#define DEFAULT_OPEN_DOOR_LEN   30
#define DEFAULT_OPEN_HOUSE_LEN  (3*60*60)

/*********/
/* Class */
/*********/

HausProx::HausProx()
{
  /* Initalize globals to default values */
  readerOpensDoor = true;
  openHouseMode = false;
  openHouseDuration = DEFAULT_OPEN_HOUSE_LEN;
  doorEntryDuration = DEFAULT_OPEN_DOOR_LEN;
  lastDoorLocked = true;
  strcpy(password, DEFAULT_PASSWORD);
}

void HausProx::begin()
{
  /* Set the open house button pin and internal pull-up resistor */
  pinMode(PIN_OPEN_HOUSE_BTN, INPUT);
  digitalWrite(PIN_OPEN_HOUSE_BTN, HIGH);

  /* Initialize the I2C bus for interfacing with the clock (arduino is bus master) */
  Wire.begin();

  pinMode(PIN_SD_CHIPSEL, OUTPUT);

  /* Setup the card reader */
  reader.begin(PIN_DATA, PIN_CLOCK, PIN_PRESENT, PIN_BEEP);

  /* Setup the door control */
  door.begin(PIN_DOOR_LATCH);

  initSDCard();
  if (! sdEnabled ) {
    /* Log that the SD is not enabled. Of course, this won't log to the SD card but
     * it will write the message to the serial port. */
    logger.logMessage(LOG_ERROR, strSDInitFail);
  }

  loadConfig();
  
  // Log the bootup message
  logger.logMessage(LOG_MESG, strBootupMessage);

  /* The door starts locked */
  lockDoor();

  // Have the reader make a short beep
  reader.beep(500);
}

void HausProx::initSDCard()
{
  sdEnabled =  SD.begin(PIN_SD_CHIPSEL);
  /* Let the logger know if it's using the SD card or not */
  logger.sdEnabled = sdEnabled;
}

/* Locks the door and logs a message */
void HausProx::lockDoor()
{
  logger.logMessage(LOG_DOOR, strDoorLocked);
  door.lock();
}

/* Unlocks the door for a period of time (in seconds) and logs a message */
void HausProx::unlockDoor(long duration)
{
  if (! door.isLocked() ) {
    logger.logMessage(LOG_DOOR, strDoorAlreadyUnlocked);
  } else {
    logger.logMessage(LOG_DOOR, strDoorUnlocked);
  }
  /* Unlock the door for a period of time */
  door.unlock(duration);
}

void HausProx::handleEvents()
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
  handleCardScanned();

  /* Check if the open|haus button has been pressed */
  handleOpenHouse();
}

/* Called to handle a card being scanned. The data is actually buffered up by the interrupt handler
 * attached to the clock pin. When sufficient data has been captured (255 bytes) this function 
 * will process the data, scan the database, etc. */
void HausProx::handleCardScanned()
{
  if (!readerOpensDoor) {
    // The reader is not currently opening the door
    return;
  }

  // Scan the card if present
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

  // Clear the card buffer
  reader.clearCardData();

  /* Scan the database */
  CardInfo info;
  int ret = database.lookupCard(serial, info);

  if (ret == DATABASE_RECORD_NOT_FOUND) {
    /* The card isn't in the database */
    reader.playFailBeep();
    logger.logMessage(LOG_CARD, strDenyUnregCard, serial);
    return;

  } else if (ret != DATABASE_SUCCESS) {
    /* Log the error */
    logger.logMessage(LOG_ERROR, CardDatabase::getErrorStr(ret), serial);
    return;
  }

  if (info.enabled) 
  {
    /* Card holder is granted access */
    if (openHouseMode) {
      /* Already in open house mode, so whatever */
      logger.logMessage(LOG_CARD, strValidOpenHouse, info.serial);
    } else {
      /* Log the entry message */
      logger.logMessage(LOG_CARD, strAdmitEntry, info.serial);
      unlockDoor(doorEntryDuration);
    }
  } else {
    /* The card is disabled */
    reader.playFailBeep();
    logger.logMessage(LOG_CARD, strDenyDisabledCard, info.serial);
  }
}

/* Handles the open house button being pressed */
void HausProx::handleOpenHouse()
{
  /* Update the open house toggle button (handles debouncing) */
  boolean n = (digitalRead(PIN_OPEN_HOUSE_BTN) == LOW);
  openHouseButton.update(n);
  /* Check if somebody has pressed the button (goes low to high) */
  if (openHouseButton.changed && openHouseButton.state) 
  {
    /* Toggle open house mode */
    openHouseMode = !openHouseMode;
    if (!openHouseMode) {
      // Turn off open house mode
      lockDoor();
      logger.logMessage(LOG_DOOR, strOpenHouseOff);
    } else {
      // Turn on open house mode
      unlockDoor(openHouseDuration);
      logger.logMessage(LOG_DOOR, strOpenHouseOn);
    }
  }
}

boolean HausProx::checkPassword(const char *input)
{
  return (strcmp(input, password) == 0);
}

void HausProx::tick()
{
  door.tick();
}

boolean HausProx::loadConfig()
{
  File file = SD.open(CONFIG_FILE, FILE_READ);
  if (!file) {
    // Log the error
    println_prog_str(strErrorLoadingConfig);
    return false;
  }
  
  const char *delims = "=";
  char line[32];
  while(1) 
  {
    // Read another line
    if (!read_line(&file, line, sizeof(line))) {
      break;
    }
    trim(line);
    if (line[0] == 0 || line[0] == '#') {
      // Skip blank lines and comments
      continue;
    }
    // Extract the name
    char *name = strtok(line, delims);
    // Extract the value
    char *value = strtok(NULL, delims);

    if (!name || !value) {
      // Badly formed line
      println_prog_str(strConfigBadLine);
      continue;
    }

    // Trim whitespace from the name and value
    trim(name);
    trim(value);
    
    if (prog_str_equals(strConfigPass, name) && value) {
      // Password
      strcpy(password, value);
    } else if (prog_str_equals(strConfigOpenDoor, name) && value) {
      // Open door length
      doorEntryDuration = atol(value);
    } else if (prog_str_equals(strConfigOpenHouse, name) && value) {
      // Open house length
      openHouseDuration = atol(value);
    } else {
      print_prog_str(strConfigInvalid);
      Serial.println(name);
    }
  }
  file.close();
  return true;
}

int HausProx::insertCard(CardInfo &info)
{
  int ret = database.insertCard(info);
  
  if (ret == DATABASE_SUCCESS) {
    // Successfully added the card
    logger.logMessage(LOG_ADMIN, strInsertedCard, info.serial, NULL);
  }
  return ret;
}

int HausProx::deleteCard(CardInfo &info)
{
  serial_t serial;
  strcpy(serial, info.serial);
  /* Note that cards are never deleted from the database. Instead the information is blankked-out
   * (tombstoned) and reused later when a card is inserted. */
  info.setBlank();
  int ret = database.putCard(info.slot, info);
  
  if (ret == DATABASE_SUCCESS) {
    // Successfully added the card
    logger.logMessage(LOG_ADMIN, strDeletedCard, serial, NULL);
  }
  return ret;
}

int HausProx::updateCard(CardInfo &info)
{
  int ret = database.putCard(info.slot, info);
  if (ret == DATABASE_SUCCESS) {
    // Successfully updated the card
    logger.logMessage(LOG_ADMIN, strUpdatedCard, info.serial, NULL);
  }
  return ret;
}

