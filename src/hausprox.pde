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

#include <string.h>
#include <SD.h>
#include <TimerOne.h>
#include "CardReader.h"
#include "CardDatabase.h"
#include "Logger.h"
#include "utils.h"
#include "Door.h"

/* TODO:
 * -What if the SD card fails to init?
 * -What if the SD card is corrupted/fails during a write/read?
 */

#define CLOCK             3
#define DATA              4
#define PRESENT           5

#define DOOR_LATCH        2

#define OPEN_HOUSE_BTN    7
#define OPEN_HOUSE_LIGHT  8

/* Chip select for the SD card */
#define SD_CHIPSEL        10

/* The maximum admin password length */
#define PASSWORD_LEN      16

CardReader     reader(DATA, CLOCK, PRESENT);
CardDatabase   database;

Door           door(DOOR_LATCH);

/* Whether we are in "open house" mode, where the door is kept open
 * for a pre-determined amount of time */
boolean        openHouseMode;

/* The open house duration in minutes */
long           openHouseDuration;

/* How long the door remains open (seconds) when somebody swipes a card */
long           doorEntryDuration;

/* The admin password */
char           password[PASSWORD_LEN];

DebouncedInput openHouseButton;

/* Whether the door was locked the last time we checked */
boolean        lastDoorLocked;

/***********/
/* Strings */
/***********/

PROGMEM const prog_char strWelcomeMessage[] = {
  "* * * * * * * * * * * * * * * * * * * * * *\n"\
  "Welcome to the haus|prox door access system\n"\
  "* * * * * * * * * * * * * * * * * * * * * *\n\n"\
  "Please login to continue\n\n"};
PROGMEM const prog_char strCardError[] = {"Error reading card"};
PROGMEM const prog_char strValidOpenHouse[] = {"Admit entry (open house mode active)"};
PROGMEM const prog_char strAdmitEntry[] = {"Admit entry"};
PROGMEM const prog_char strDenyDisabledCard[] = {"Deny disabled card"};
PROGMEM const prog_char strDenyUnregCard[] = {"Deny unregistered card"};
PROGMEM const prog_char strAdminDenied[] = {"Admin access denied"};
PROGMEM const prog_char strSDInitFail[] = {"Failed to init the SD card"};
PROGMEM const prog_char strBootupMessage[] = {"haus|prox bootup"};
PROGMEM const prog_char strLoginMessage[] = {"Admin login"};
PROGMEM const prog_char strLoginPrompt[] = {"\nLogin: "};
PROGMEM const prog_char strStatusTitle[] = {"haux|prox status\n"};
PROGMEM const prog_char strDoorStatus[] = {"Door locked: "};
PROGMEM const prog_char strOpenHouseStatus[] = {"Open house: "};
PROGMEM const prog_char strYes[] = {"yes"};
PROGMEM const prog_char strNo[] = {"no"};
PROGMEM const prog_char strOpenButtonStatus[] = {"Open button: "};
PROGMEM const prog_char strOpenHouseOn[] = {"Turn on open house mode"};
PROGMEM const prog_char strOpenHouseOff[] = {"Turn off open house mode"};
PROGMEM const prog_char strOpenHouseExpired[] = {"Open house mode expired"};
PROGMEM const prog_char strDoorLocked[] = {"Door is now locked"};
PROGMEM const prog_char strDoorUnlocked[] = {"Door is now unlocked"};
PROGMEM const prog_char strDoorAlreadyUnlocked[] = {"Door is already unlocked"};

/*************/
/* Functions */
/*************/

/* Load the general config options */
void load_config()
{
  /* Initalize globals to default values */
  openHouseMode = false;
  openHouseDuration = 3*60*60;
  doorEntryDuration = 10;
  lastDoorLocked = true;

  /* The door starts locked */
  lock_door();

  strcpy(password, "123");

//  File file = SD.open("hausprox.cfg", FILE_READ);
//  file.close();
}

/* Interrupt handler to receive card data */
void receive_card_data()
{
  reader.receiveCardData();
}

/* Reads a line of input from Serial, up to 'maxlen-1' chars. The returned string is always
 * null-terminated. This function calls 'handle_events' while it waits for input, so the 
 * program will be responsive to events while inputting data. */
void read_input(char *buffer, int maxlen)
{
  int pos = 0;
  /* We need to leave a space at the end of the string for the null 
   * character, in case the input goes too long */
  while (pos < maxlen-1)
  {
    if (Serial.available() > 0) 
    {
      // Grab another character from the serial port
      char ch = Serial.read();
      if (ch == '\n' || ch == '\r') break;
      // Add another character to our buffer
      buffer[pos++] = ch;
    }
    /* Possibly handle other events */
    handle_events();
  }
  /* Be sure to null terminate the string */
  buffer[pos] = 0;
}

/* Called once every second by Timer1 */
void timer_tick()
{
  door.tick();
}

void lock_door()
{
  logger.logMessage(LOG_DOOR, strDoorLocked);
  door.lock();
}

void unlock_door(long duration)
{
  if (! door.isLocked() ) {
    logger.logMessage(LOG_DOOR, strDoorAlreadyUnlocked);
  } else {
    logger.logMessage(LOG_DOOR, strDoorUnlocked);
  }
  /* Unlock the door for a period of time */
  door.unlock(duration);
}

/**********/
/* Events */
/**********/

void handle_events()
{
  /* Check if the door has recently locked */
  boolean locked = door.isLocked();
  if (locked && !lastDoorLocked) {
    if (openHouseMode) {
      /* Open house mode has expired */
      openHouseMode = false;
      logger.logMessage(LOG_DOOR, strOpenHouseExpired);
    }
    /* Log that the door has been locked again */
    logger.logMessage(LOG_DOOR, strDoorLocked);
  }
  lastDoorLocked = locked;
  
  /* Check if a card has been scanned */
  handle_card_scanned();

  /* Check if the open|haus button has been pressed */
  handle_open_house();
}

void handle_card_scanned()
{
  unsigned int facility, card;

  if (!reader.hasCardData()) {
    return;
  }

  // Read the card data
  int err = reader.readCard(facility, card);

  // Interpret the results
  if (err != 0) {
    /* Log the error and the contents of the card buffer */
    logger.logMessage(LOG_ERROR, CardReader::getErrorStr(err), 0, 0, &reader);
    // Clear the card buffer
    reader.clearCardData();
    return;
  }

/*
  int n;
  for (n = 0; n < reader.getLength(); n++)
  {
    int bit = reader.getData(n);
    Serial.print('0'+bit, BYTE);
  }
  Serial.println(reader.getLength());
  Serial.println("done");
  
  return;
*/

  // Clear the card buffer
  reader.clearCardData();

  CardInfo info;
  if (! database.lookupCard(facility, card, info) ) 
  {
    /* The card isn't in the database */
    logger.logMessage(LOG_CARD, strDenyUnregCard, facility, card);
    return;
  }
  if (info.enabled) 
  {
    /* Card holder is granted access */
    if (openHouseMode) {
      /* Already in open house mode, so whatever */
      logger.logMessage(LOG_CARD, strValidOpenHouse, facility, card);
    } else {
      /* Log the entry message */
      logger.logMessage(LOG_CARD, strAdmitEntry, facility, card);
      unlock_door(doorEntryDuration);
    }
  } else {
    logger.logMessage(LOG_CARD, strDenyDisabledCard, facility, card);
  }
}

/* Handles the open house button being pressed */
void handle_open_house()
{
  /* Update the open house toggle button (handles debouncing) */
  boolean n = (digitalRead(OPEN_HOUSE_BTN) == LOW);
  openHouseButton.update(n);
  /* Check if somebody has pressed the button (goes low to high) */
  if (openHouseButton.hasChanged() && openHouseButton.getState()) 
  {
    if (openHouseMode) {
      // Turn off open house mode
      openHouseMode = false;
      digitalWrite(OPEN_HOUSE_LIGHT, LOW);
      logger.logMessage(LOG_DOOR, strOpenHouseOff);
      lock_door();
    } else {
      // Turn on open house mode
      openHouseMode = true;
      logger.logMessage(LOG_DOOR, strOpenHouseOn);
      unlock_door(openHouseDuration);
    }
    digitalWrite(OPEN_HOUSE_LIGHT, HIGH);
  }
}

/********/
/* Menu */
/********/

/*
    Welcome + login screen
    Debug mode
    Card management
        Add card
        Delete card
        List cards
        De/activate card
*/
#if 0
void show_login_screen()
{
  char buffer[10];
  print_prog_str(&Serial, strWelcomeMessage);
  while (1) {
    print_prog_str(&Serial, strLoginPrompt);
    read_input(buffer, sizeof(buffer));
    //Serial.println(buffer);
    if (strcmp(buffer, password) == 0) break;
    delay(1000);
    logger.logMessage(LOG_ADMIN, strAdminDenied);
  }
  logger.logMessage(LOG_ADMIN, strLoginMessage);
}
#endif

void debug_screen()
{
  char buf[10];
  while(1) {
    read_input(buf, sizeof(buf));
    if (strcmp(buf, "status") == 0) {
      /* Display some debug information */
      print_prog_str(&Serial, strStatusTitle);
      /* Display the door lock status */
      print_prog_str(&Serial, strDoorStatus);
      if (door.isLocked()) {
        print_prog_str(&Serial, strYes);
      } else {
        print_prog_str(&Serial, strNo);
      }
      Serial.print('\n', BYTE);
      /* Display the open house status */
      print_prog_str(&Serial, strOpenHouseStatus);
      if (openHouseMode) {
        print_prog_str(&Serial, strYes);
      } else {
        print_prog_str(&Serial, strNo);
      }
      Serial.print('\n', BYTE);
      /* Display the open house button status */
      print_prog_str(&Serial, strOpenButtonStatus);
      if (digitalRead(OPEN_HOUSE_BTN) == LOW) {
        print_prog_str(&Serial, strYes);
      } else {
        print_prog_str(&Serial, strNo);
      }
      Serial.print('\n', BYTE);
      Serial.print('\n', BYTE);
    }
  }
}

/********/
/* Main */
/********/

void setup()
{
  Serial.begin(9600);
  delay(200);

  /* Set the open house button pin and internal pull-up resistor */
  pinMode(OPEN_HOUSE_BTN, INPUT);
  digitalWrite(OPEN_HOUSE_BTN, HIGH);

  // Initialize the SD card
  pinMode(SD_CHIPSEL, OUTPUT);
  if (!SD.begin(SD_CHIPSEL)) {
    logger.logMessage(LOG_MESG, strSDInitFail);
  }
  logger.logMessage(LOG_MESG, strBootupMessage);

  /* Load the general program configuration */  
  load_config();

  /* Attach an interrupt to the card reader clock pin */
  attachInterrupt(1, receive_card_data, FALLING);

  /* Start the 1-second interrupt timer */
  Timer1.attachInterrupt(timer_tick);
  Timer1.initialize(1000000);
}

void loop()
{
  debug_screen();
//  handle_events();
//  show_login_screen();
}

