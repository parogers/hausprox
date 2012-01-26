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

PROGMEM const prog_char strStatusTitle[] = {"haux|prox status\n----------------\n"};
PROGMEM const prog_char strSDCardStatus[] = {"SD enabled:  "};
PROGMEM const prog_char strDoorStatus[] = {"Door locked: "};
PROGMEM const prog_char strOpenHouseStatus[] = {"Open house:  "};
PROGMEM const prog_char strOpenButtonStatus[] = {"Open button: "};
PROGMEM const prog_char strYes[] = {"yes"};
PROGMEM const prog_char strNo[] = {"no"};

#define YESNO(b)      ((b) ? strYes : strNo)

/***********/
/* Globals */
/***********/

HausProx hausProx;

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
  char buf[30];
  while(1) 
  {
    read_input(buf, sizeof(buf));
    if (strcmp(buf, "status") == 0) 
    {
      /* Display some debug information */
      print_prog_str(&Serial, strStatusTitle);
      /* Display the open house status */
      print_prog_str(&Serial, strOpenHouseStatus);
      print_prog_str(&Serial, YESNO(hausProx.openHouseMode));
      Serial.print('\n');
      /* Display the open house button status */
      print_prog_str(&Serial, strOpenButtonStatus);
      print_prog_str(&Serial, YESNO(hausProx.openHouseButton.state));
      Serial.print('\n');
      /* Display SD card status */
      print_prog_str(&Serial, strSDCardStatus);
      print_prog_str(&Serial, YESNO(hausProx.sdEnabled));
      Serial.print('\n');
      /* Display the door lock status */
      print_prog_str(&Serial, strDoorStatus);
      print_prog_str(&Serial, YESNO(hausProx.door.isLocked()));
      Serial.print('\n');
      Serial.print('\n');
    } 
    else if (strncmp(buf, "settime", 7) == 0)
    {
      /* Setting the time. Format: YY-MM-DD HH:MM:SS (24 hour clock) */
      hausProx.logger.clock.setDateTime("12-01-20 19:38:00");
      //hausProx.logger.clock.setDateTime(12, 01, 20, 19, 38, 00);
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
  debug_screen();
}

