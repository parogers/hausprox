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

#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <string.h>
#include <TimerOne.h>

#include "Prox.h"

/* TODO:
 * -autoinit SD card if a previous attempt to access it failed
 * -use the SD card present pin
 */

/***********/
/* Strings */
/***********/

// Strings for main screen
PROGMEM const prog_char strMainMenu[] = {"\n**haus|prox**\n\n[1] Status\n[2] Manage cards\n[3] Change date/time\n[4] Test SD card\n\n"};
PROGMEM const prog_char strSDCardStatus[] = {"SD enabled:  "};
PROGMEM const prog_char strDoorStatus[] = {"Door locked: "};
PROGMEM const prog_char strOpenHouseStatus[] = {"Open house:  "};
PROGMEM const prog_char strDateTimePrompt[] = {"Enter YY-MM-DD HH:MM:SS\n"};
PROGMEM const prog_char strDateTimeOkay[] = {"Date/time changed\n"};

PROGMEM const prog_char strYes[] = {"yes"};
PROGMEM const prog_char strNo[] = {"no"};
PROGMEM const prog_char strInvalidEntry[] = {"Invalid entry\n"};
PROGMEM const prog_char strAborted[] = {"Aborted\n"};

// Strings for card management screen
PROGMEM const prog_char strCardMenu[] = {"\n**Card Management**\n\n[1] List cards\n[2] Add\n[3] Delete\n[4] Edit\n[9] Back to main\n\n"};
PROGMEM const prog_char strAddCardTitle[] = {"\n**Add card**\n\n"};
PROGMEM const prog_char strEditCardTitle[] = {"\n**Edit card**\n\n"};
PROGMEM const prog_char strDeleteCardTitle[] = {"\n**Delete card**\n\n"};
PROGMEM const prog_char strSlotPrompt[] = {"Slot number?\n"};
PROGMEM const prog_char strEditingCard[] = {"Editing card "};
PROGMEM const prog_char strCardPrompt[] = {"New serial (format FFF-CCCCC)?"};
PROGMEM const prog_char strActivePrompt[] = {"Card active?\n"};
PROGMEM const prog_char strConfirmPrompt[] = {"Confirm?\n"};
PROGMEM const prog_char strCardRecordEdited[] = {"Card record edited"};

#define YESNO(b)      ((b) ? strYes : strNo)

/***********/
/* Globals */
/***********/

HausProx hausProx;
char input[20];

/*************/
/* Functions */
/*************/

/* Reads a line of input from Serial, up to 'maxlen-1' chars. The returned string is always
 * null-terminated. This function calls 'handle_events' while it waits for input, so the 
 * program will be responsive to events while inputting data. */
void read_input(const prog_char *msg)
{
  int pos = 0, maxlen=sizeof(input);

  if (msg != NULL) {
    // Display the prompt
    print_prog_str(msg);
  }
  
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
      input[pos++] = ch;
    }
    /* Possibly handle other events */
    hausProx.handle_events();
  }
  /* Be sure to null terminate the string */
  input[pos] = 0;
  Serial.println(input);
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
  print_prog_str(strWelcomeMessage);
  while (1) {
    print_prog_str(strLoginPrompt);
    read_input(buffer, sizeof(buffer));
    //Serial.println(buffer);
    if (strcmp(buffer, password) == 0) break;
    delay(1000);
    logger.logMessage(LOG_ADMIN, strAdminDenied);
  }
  logger.logMessage(LOG_ADMIN, strLoginMessage);
}
#endif

void command_status()
{
  /* Display the open house status */
  print_prog_str(strOpenHouseStatus);
  print_prog_str(YESNO(hausProx.openHouseMode));
  Serial.print('\n');
  /* Display SD card status */
  print_prog_str(strSDCardStatus);
  print_prog_str(YESNO(hausProx.sdEnabled));
  Serial.print('\n');
  /* Display the door lock status */
  print_prog_str(strDoorStatus);
  print_prog_str(YESNO(hausProx.door.isLocked()));
  Serial.print('\n');
}

void command_setdate()
{
  /* Format: YY-MM-DD HH:MM:SS (24 hour clock) */
  while(1) 
  {
    read_input(strDateTimePrompt);
    if (input[0] == 0) {
      // User cancelled
      print_prog_str(strAborted);
      break;
    }
    if (logger.clock.setDateTime(input)) {
      // Time was set successfully
      logger.logMessage(LOG_ADMIN, strDateTimeOkay);
      break;
    }
    // There was a problem setting the time
    print_prog_str(strInvalidEntry);
  }
}

void command_test_sd()
{
  /* Check if the card is physically plugged in */
  
  /* Try to initialize the SD library */
//  if (!SD.begin()
  
  /* Try to access the card database */

}

void card_management_edit()
{
  // Edit entry
  print_prog_str(strEditCardTitle);
  hausProx.database.printRecords();
  read_input(strSlotPrompt);
  int slot = atoi(input);
  if (slot == 0) {
    print_prog_str(strAborted);
    return;
  }
  
  // Lookup the card
  CardInfo info;
  if (!hausProx.database.getCard(slot, info)) {
    print_prog_str(strInvalidEntry);
    return;
  }

  boolean changed = false;
  while(1) 
  {
    // Tell the user what card is being edited
    print_prog_str(strEditingCard);
    Serial.println(info.serial);
    // Let them enter a new serial (or blank to keep existing)
    read_input(strCardPrompt);
    if (input[0] == 0) break;
    // Trim the newline
    trim(input);
    if (strlen(input) == SERIAL_LEN)
    {
      // Change the serial
      strcpy(info.serial, input);
      changed = true;
      break;
    }
    // Try again
    print_prog_str(strInvalidEntry);
  }
  while(1) {
    // Prompt the user to change the active/inactive status
    read_input(strActivePrompt);
    if (input[0] == 0) break;
    // Change the enabled status
    if (input[0] == 'y' || input[1] == 'Y') {
      info.enabled = 1;
      changed = true;
      break;
    } else if (input[0] == 'n' || input[1] == 'N') {
      info.enabled = 0;
      changed = true;
      break;
    }
    print_prog_str(strInvalidEntry);
  }
  if (changed) {
    // Store the changes back in the database
    int ret = hausProx.database.putCard(slot, info);
    if (ret == DATABASE_SUCCESS) {
      logger.logMessage(LOG_ADMIN, strCardRecordEdited, info.serial, NULL);
    } else {
      println_prog_str(hausProx.database.getErrorStr(ret));
    }
  }
}

void card_management_menu()
{
  int slot, n;
  CardInfo info;
  
  while(1)
  {
    read_input(strCardMenu);
    switch(input[0]) {
      case '1':
        // List cards
        hausProx.database.printRecords();
        break;
      case '2':
        // Add card entry
        print_prog_str(strAddCardTitle);
        read_input(strCardPrompt);
        read_input(strActivePrompt);
        break;
      case '3':
        // Remove entry
        print_prog_str(strDeleteCardTitle);
        hausProx.database.printRecords();
        read_input(strSlotPrompt);
        slot = atoi(input);
        if (slot == 0) continue;
        read_input(strConfirmPrompt);
        if (input[0] == 0) continue;
        break;
      case '4':
        card_management_edit();
        break;
      case '9':
        // Back to main menu
        return;
    }
  }
}

void main_menu()
{
  while(1) 
  {
    /* Display the main menu */
    read_input(strMainMenu);
    switch(input[0]) {
      case '1':
        command_status();
        break;
      case '2':
        card_management_menu();
        break;
      case '3':
        command_setdate();
        break;
      case '4':
        command_test_sd();
        break;
    }
  }
}

/************/
/* Handlers */
/************/

/* Called once every second by Timer1 */
void timer_tick()
{
  hausProx.door.tick();
}

/* Interrupt handler to receive card data */
void receive_card_data()
{
  hausProx.reader.receiveCardData();
}

/********/
/* Main */
/********/

void setup()
{
  Serial.begin(9600);
  delay(200);

  /* Attach an interrupt to the card reader clock pin */
  attachInterrupt(1, receive_card_data, FALLING);

  /* Start the 1-second interrupt timer. We use that for keeping track of how long the door has been open */
  Timer1.attachInterrupt(timer_tick);
  Timer1.initialize(1000000);
  
  hausProx.begin();
}

void loop()
{
  main_menu();
}

