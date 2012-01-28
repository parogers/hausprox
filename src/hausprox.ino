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

PROGMEM const prog_char strMainMenu[] = {"\nhaus|prox\n---------\n\n[1] Status\n[2] Manage cards\n[3] Change date/time\n[4] Test SD card\n\n"};
PROGMEM const prog_char strCardMenu[] = {"\nCard Management\n---------------\n\n[1] Add\n[2] Delete\n[3] Activate/deactive\n[4] Previous page\n[5] Next page\n[9] Back to main\n\n"};
PROGMEM const prog_char strSDCardStatus[] = {"SD enabled:  "};
PROGMEM const prog_char strDoorStatus[] = {"Door locked: "};
PROGMEM const prog_char strOpenHouseStatus[] = {"Open house:  "};
PROGMEM const prog_char strYes[] = {"yes"};
PROGMEM const prog_char strNo[] = {"no"};
PROGMEM const prog_char strDateTimePrompt[] = {"Enter YY-MM-DD HH:MM:SS\n"};
PROGMEM const prog_char strDateTimeInvalid[] = {"Invalid entry\n"};
PROGMEM const prog_char strDateTimeOkay[] = {"Date/time changed\n"};
PROGMEM const prog_char strAborted[] = {"Aborted\n"};
PROGMEM const prog_char strSlotPrompt[] = {"Slot number?"};
PROGMEM const prog_char strCardPrompt[] = {"Card number?"};
PROGMEM const prog_char strActivePrompt[] = {"Active?"};

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
    hausProx.handle_events();
  }
  /* Be sure to null terminate the string */
  buffer[pos] = 0;
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
    print_prog_str(strDateTimePrompt);
    read_input(input, sizeof(input));
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
    print_prog_str(strDateTimeInvalid);
  }
}

void command_test_sd()
{
  /* Check if the card is physically plugged in */
  
  /* Try to initialize the SD library */
//  if (!SD.begin()
  
  /* Try to access the card database */

}

void card_management_menu()
{
  while(1)
  {
    print_prog_str(strCardMenu);
    read_input(input, sizeof(input));
    switch(input[0]) {
      case '1':
        // Add card entry
        break;
      case '2':
        // Remove entry
        break;
      case '3':
        // Edit entry
        print_prog_str(strSlotPrompt);
        read_input(input, sizeof(input));
        break;
      case '4':
        // Previous page
        break;
      case '5':
        // Next page
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
    print_prog_str(strMainMenu);
    read_input(input, sizeof(input));
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

