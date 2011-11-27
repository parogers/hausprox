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

#define OPEN_HOUSE_BTN    6
#define OPEN_HOUSE_LIGHT  7

/* Chip select for the SD card */
#define SD_CHIPSEL        10

/* The maximum admin password length */
#define PASSWORD_LEN      16

CardReader     reader(DATA, CLOCK, PRESENT);
CardDatabase   database;
Logger         logger;

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

/***********/
/* Strings */
/***********/

PROGMEM const prog_char strWelcomeMessage[] = {
  "* * * * * * * * * * * * * * * * * * * * * *\n"\
  "Welcome to the haus|prox door access system\n"\
  "* * * * * * * * * * * * * * * * * * * * * *\n\n"\
  "Please login to continue\n\n"};
PROGMEM const prog_char strCardError[] = {"Error reading card"};
PROGMEM const prog_char strValidOpenHouse[] = {"Valid card, but open house mode active"};
PROGMEM const prog_char strAdmitEntry[] = {"Admit entry"};
PROGMEM const prog_char strDenyDisabledCard[] = {"Deny disabled card"};
PROGMEM const prog_char strDenyUnregCard[] = {"Deny unregistered card"};
PROGMEM const prog_char strAdminDenied[] = {"Admin access denied"};
PROGMEM const prog_char strSDInitFail[] = {"Failed to init the SD card"};
PROGMEM const prog_char strBootupMessage[] = {"haus|prox bootup"};
PROGMEM const prog_char strLoginMessage[] = {"Admin login"};
PROGMEM const prog_char strLoginPrompt[] = {"\nLogin: "};

/*************/
/* Functions */
/*************/

/* Load the general config options */
void load_config()
{
  /* Initalize globals to default values */
  openHouseMode = false;
  openHouseDuration = 3*60*60;
  doorEntryDuration = 5;

  /* The door starts locked */
  door.lock();

  strcpy(password, "123");

//  File file = SD.open("hausprox.cfg", FILE_READ);
//  file.close();
}

/* Interrupt handler to receive card data */
void receive_card_data()
{
  reader.receiveCardData();
}

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
    // Possibly handle other events
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

/**********/
/* Events */
/**********/

void handle_events()
{
  /* Check if a card has been scanned */
  handle_card_scanned();

  /* Check if the open|haus button has been pressed */
//  handle_open_house();

  /* Set the door latch state */
//  if (lockDoorCountdown == 0) {
//    digitalWrite(DOOR_LATCH, HIGH);
//  }
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
    /* Log the error */
    logger.logMessage(LOG_ERROR, CardReader::getErrorStr(err));
    /* Log the contents of the card buffer */
    
    // Clear the card buffer
    reader.clearCardData();
    return;
  }

  // Clear the card buffer
  reader.clearCardData();

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

  CardInfo info;
  if (database.lookupCard(facility, card, info)) {
    if (info.enabled) {
      if (openHouseMode) {
        logger.logMessage(LOG_CARD, strValidOpenHouse, facility, card);
      } else {
        /* Unlock the door for a period of time */
        door.unlock(doorEntryDuration);
        /* Log the entry message */
        logger.logMessage(LOG_CARD, strAdmitEntry, facility, card);
      }
    } else {
      logger.logMessage(LOG_CARD, strDenyDisabledCard, facility, card);
    }
  } else {
    logger.logMessage(LOG_CARD, strDenyUnregCard, facility, card);
  }
}

/* Handles the open house button being pressed */
#if 0
void handle_open_house()
{
  boolean newHouse = digitalRead(OPEN_HOUSE_BTN) == HIGH;

  if (newHouse != openHouseMode) 
  {
    openHouseMode = newHouse;
    if (!openHouseMode) {
      /* Lock the door and turn off the light*/
      //lock_door();
      digitalWrite(OPEN_HOUSE_LIGHT, LOW);
    } else {
      /* Unlock the door for open house mode and turn on the light */
      unlock_door(openHouseDuration);
      digitalWrite(OPEN_HOUSE_LIGHT, HIGH);
    }
  }
}
#endif

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
    //break;
  }
  logger.logMessage(LOG_ADMIN, strLoginMessage);
}

/********/
/* Main */
/********/

void setup()
{
  Serial.begin(9600);
  delay(200);

//  pinMode(OPEN_HOUSE_BTN, INPUT);
//  pinMode(OPEN_HOUSE_LIGHT, OUTPUT);

  // Initialize the SD card
  pinMode(SD_CHIPSEL, OUTPUT);
  if (!SD.begin(SD_CHIPSEL)) {
    print_prog_str(&Serial, strSDInitFail);
    logger.enabled = false;
  } else {
    logger.enabled = true;
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
  handle_events();
//  show_login_screen();
}

