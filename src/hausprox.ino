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
PROGMEM const prog_char strDateTimePrompt[] = {"Enter YY-MM-DD HH:MM:SS "};
PROGMEM const prog_char strDateTimeOkay[] = {"Date/time changed\n"};

PROGMEM const prog_char strYes[] = {"yes"};
PROGMEM const prog_char strNo[] = {"no"};
PROGMEM const prog_char strInvalidEntry[] = {"Invalid entry"};
PROGMEM const prog_char strAborted[] = {"Aborted"};
PROGMEM const prog_char strSuccess[] = {"Success"};

// Strings for card management screen
PROGMEM const prog_char strCardMenu[] = {"\n**Card Management**\n\n[1] List cards\n[2] Add\n[3] Delete\n[4] Edit\n[9] Back to main\n\n"};
PROGMEM const prog_char strAddCardTitle[] = {"\n**Add card**\n\n"};
PROGMEM const prog_char strEditCardTitle[] = {"\n**Edit card**\n\n"};
PROGMEM const prog_char strDeleteCardTitle[] = {"\n**Delete card**\n\n"};
PROGMEM const prog_char strSlotPrompt[] = {"Slot number? "};
PROGMEM const prog_char strEditingCard[] = {"Editing card "};
PROGMEM const prog_char strDeletingCard[] = {"Deleting card "};
PROGMEM const prog_char strCardPrompt[] = {"Serial (format FFF-CCCCC)? "};
PROGMEM const prog_char strActivePrompt[] = {"Card active? "};
PROGMEM const prog_char strConfirmPrompt[] = {"Confirm? "};
PROGMEM const prog_char strSerialExists[] = {"Serial number is already taken"};

#define YESNO(b)      ((b) ? strYes : strNo)

/***********/
/* Globals */
/***********/

HausProx hausProx;
char input[20];
CardInfo info;

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

/* Displays the list of cards in the database, prompts the user for a slot number and
 * loads the card data. */
boolean read_card(CardInfo &info)
{
  hausProx.database.printRecords();
  while(1) 
  {
    /* Prompt the user for the slot number */
    read_input(strSlotPrompt);
    int slot = atoi(input);
    if (slot == 0) {
      println_prog_str(strAborted);
      return false;
    }
    slot--;
    // Lookup the card
    int ret = hausProx.database.getCard(slot, info);
    if (ret == DATABASE_SUCCESS) {
      return true;
    }
    print_prog_str(hausProx.database.getErrorStr(ret));
  }
}

/* Asks a question and waits for the user to enter yes/no. Returns 1=true, 0=false, -1=empty input */
int read_yesno(const prog_char *msg)
{
  while(1)
  {
    // Display the prompt, wait for input
    read_input(msg);
    // We only check the first character typed
    char first = input[0];
    if (first == 0) return -1;
    if (first == 'y' || first == 'Y') return 1;
    if (first == 'n' || first == 'N') return 0;
    print_prog_str(strInvalidEntry);
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
      println_prog_str(strAborted);
      break;
    }
    if (logger.clock.setDateTime(input)) {
      // Time was set successfully
      logger.logMessage(LOG_ADMIN, strDateTimeOkay);
      break;
    }
    // There was a problem setting the time
    println_prog_str(strInvalidEntry);
  }
}

void command_test_sd()
{
  /* Check if the card is physically plugged in */
  
  /* Try to initialize the SD library */
//  if (!SD.begin()
  
  /* Try to access the card database */

}

void card_management_add()
{
  print_prog_str(strAddCardTitle);

  while(1) 
  {
    // Let them enter a new serial (or blank to keep existing number if editing)
    read_input(strCardPrompt);
    if (input[0] == 0) {
      // Aborted adding
      println_prog_str(strAborted);
      return;
    }
    // Trim the newline
    trim(input);
    if (strlen(input) == SERIAL_LEN)
    {
      // Make sure the serial doesn't already exist
      // Change the serial
      strcpy(info.serial, input);
      break;
    }
    // Try again
    println_prog_str(strInvalidEntry);
  }

  // Prompt the user for the enabled flag
  int ret = read_yesno(strActivePrompt);
  if (ret == -1) {
      println_prog_str(strAborted);
      return;
  }
  info.enabled = ret;
  
  // Now attempt to insert the card into the database
  ret = hausProx.database.insertCard(info);
  if (ret == DATABASE_SUCCESS) {
    // Successful
    println_prog_str(strSuccess);
  } else {
    // Failure
    println_prog_str(hausProx.database.getErrorStr(ret));
  }
}

void card_management_edit()
{
  int ret;

  // Lookup a card in the database
  print_prog_str(strEditCardTitle);
  if (!read_card(info)) {
    return;
  }

  // Tell the user what card is being edited
  print_prog_str(strEditingCard);
  Serial.println(info.serial);
  
  boolean changed = false;
  while(1) 
  {
    // Let them enter a new serial (or blank to keep existing number if editing)
    read_input(strCardPrompt);
    if (input[0] == 0) {
      break;
    }
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
    println_prog_str(strInvalidEntry);
  }

  // Prompt the user for the enabled flag
  ret = read_yesno(strActivePrompt);
  if (ret != -1) {
    info.enabled = ret;
    changed = true;
  }

  if (changed) {
    // Replace an existing card
    ret = hausProx.database.putCard(info.slot, info);
    
    if (ret == DATABASE_SUCCESS) {
      println_prog_str(strSuccess);
    } else {
      println_prog_str(hausProx.database.getErrorStr(ret));
    }
  } else {
    println_prog_str(strAborted);
  }
}

void card_management_delete()
{
  int ret;

  print_prog_str(strDeleteCardTitle);
  if (!read_card(info)) {
    return;
  }

  // Ask the user for confirmation
  print_prog_str(strDeletingCard);
  Serial.println(info.serial);

  read_input(strConfirmPrompt);
  if (input[0] == 0 && input[0] != 'y' && input[0] == 'Y') {
    println_prog_str(strAborted);
    return;
  }
  // Blank out the card entry
  info.setBlank();
  ret = hausProx.database.putCard(info.slot, info);
  if (ret == DATABASE_SUCCESS) {
    println_prog_str(strSuccess);
  } else {
    println_prog_str(hausProx.database.getErrorStr(ret));
  }
}

void card_management_menu()
{
  int slot, n;
  
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
        card_management_add();
        break;
      case '3':
        // Remove entry
        card_management_delete();
        break;
      case '4':
        // Editing an entry
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

