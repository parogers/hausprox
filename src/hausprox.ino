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
PROGMEM const prog_char strMainMenu[] = {"\n**haus|prox**\n\n[1] Status\n[2] Manage cards\n[3] Manage log files\n[4] Change date/time\n[5] Test SD card\n\n> "};
PROGMEM const prog_char strSDCardStatus[] = {"SD enabled:  "};
PROGMEM const prog_char strDoorStatus[] = {"Door locked: "};
PROGMEM const prog_char strOpenHouseStatus[] = {"Open house:  "};
PROGMEM const prog_char strDateStatus[] = {"Date/time:   "};

// Strings for date/time
PROGMEM const prog_char strDateTimePrompt[] = {"Enter YY-MM-DD HH:MM:SS? "};
PROGMEM const prog_char strDateTimeOkay[] = {"Date/time changed\n"};
PROGMEM const prog_char strDateTimeIs[] = {"The time is "};

// Generall-purpose strings
PROGMEM const prog_char strYes[] = {"yes"};
PROGMEM const prog_char strNo[] = {"no"};
PROGMEM const prog_char strInvalidEntry[] = {"Invalid entry"};
PROGMEM const prog_char strAborted[] = {"Aborted"};
PROGMEM const prog_char strSuccess[] = {"Success"};

// Strings for card management screen
PROGMEM const prog_char strCardMenu[] = {"\n**Card Management**\n\n[1] List cards\n[2] Add\n[3] Delete\n[4] Edit\n[9] Back to main\n\n> "};
PROGMEM const prog_char strAddCardTitle[] = {"\n**Add card**\n\n"};
PROGMEM const prog_char strEditCardTitle[] = {"\n**Edit card**\n\n"};
PROGMEM const prog_char strDeleteCardTitle[] = {"\n**Delete card**\n\n"};
PROGMEM const prog_char strSlotPrompt[] = {"Slot number? "};
PROGMEM const prog_char strEditingCard[] = {"Editing card "};
PROGMEM const prog_char strDeletingCard[] = {"Deleting card "};
PROGMEM const prog_char strCardPrompt[] = {"Serial? (FFF-CCCCC) "};
PROGMEM const prog_char strActivePrompt[] = {"Card active? "};
PROGMEM const prog_char strConfirmPrompt[] = {"Confirm? "};
PROGMEM const prog_char strSerialExists[] = {"Serial number is taken"};

// Strings for printing the card database
PROGMEM const prog_char strActive[] = {" - active"};
PROGMEM const prog_char strDisabled[] = {" - disabled"};
PROGMEM const prog_char strBlank[] = {" - blank"};

// Strings for log management
PROGMEM const prog_char strLogMenu[] = {"\n**Log Management**\n\n[1] Review log\n[2] Dump log contents\n[3] Monitor log\n[9] Back to main\n\n> "};
PROGMEM const prog_char strReviewLogTitle[] = {"\n**Review log**\n"};
PROGMEM const prog_char strDumpLogTitle[] = {"\n**Dump log**\n"};
PROGMEM const prog_char strMonitorLogTitle[] = {"\n**Monitor log**\n\nPress enter to stop"};
PROGMEM const prog_char strLogInstructions[] = {"\n\nEnter a blank to use current year, month or day.\n\n"};
PROGMEM const prog_char strEnterYear[] = {"Year? (2 digits) "};
PROGMEM const prog_char strEnterMonth[] = {"Month? "};
PROGMEM const prog_char strEnterDay[] = {"Day? (0=all) "};
PROGMEM const prog_char strLogNotFound[] = {"Log file not found: "};
PROGMEM const prog_char strNoLogEntries[] = {"No entries found"};
PROGMEM const prog_char strSearchingLog[] = {"Scanning log file: "};
PROGMEM const prog_char strPressEnter[] = {"\n<<Press enter to continue>>\n"};

// Convenience macro for printing yes/no
#define YESNO(b)              ((b) ? strYes : strNo)
// When reviewing log files, the number of lines to print before prompting the user to hit enter
#define LOG_LINES_PER_PAGE    30

/***********/
/* Globals */
/***********/

HausProx hausProx;

/* The global buffer for storing user input */
char input[25];

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
  print_cards();
  while(1) 
  {
    /* Prompt the user for the slot number */
    read_input(strSlotPrompt);
    int slot = atoi(input);
    if (slot == 0) {
      println_prog_str(strAborted);
      return false;
    }
    // Lookup the card
    int ret = hausProx.database.getCard(slot, info);
    if (ret == DATABASE_SUCCESS) {
      return true;
    }
    else if (ret == DATABASE_EOF) {
      println_prog_str(strInvalidEntry);
    } else {
      println_prog_str(hausProx.database.getErrorStr(ret));
    }
  }
}

/* Asks a question and waits for the user to enter yes/no. Returns 1=true, 0=false, -1=empty input (eg cancelled) */
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

/* Reads an integer value from the user, or a default value if the user enters a blank line */
int read_int(const prog_char *msg, int def)
{
  int ret;
  while(1) {
    read_input(msg);
    if (input[0] == 0) {
      // Go with the default
      Serial.println(def);
      return def;
    }
    boolean isInt = true;
    char *ptr = input;
    while(*ptr != 0) {
      if (*ptr < '0' || *ptr > '9') {
        isInt = false;
        break;
      }
      ptr++;
    }
    if (isInt) {
      return atoi(input);
    }
    println_prog_str(strInvalidEntry);
  }
}

void print_card_cb(CardInfo &info)
{
  Serial.print('[');
  Serial.print((int)info.slot);
  Serial.print(']');
  Serial.print(' ');
  Serial.print(info.serial);
  if (info.isBlank()) {
    println_prog_str(strBlank);
  } else {
    if (info.enabled) {
      println_prog_str(strActive);
    } else {
      println_prog_str(strDisabled);
    }
  }
}

/* Prints the contents of the card database */
void print_cards()
{
  hausProx.database.enumerateRecords(print_card_cb);
}

/* Prints the date/time to Serial */
void print_datetime()
{
  clock.update();
  clock.formatDateTime(input, sizeof(input));
  print_prog_str(strDateTimeIs);
  Serial.println(input);
}

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

/***************/
/* Status menu */
/***************/

void command_status()
{
  /* Display the open house status */
  print_prog_str(strOpenHouseStatus);
  print_prog_str(YESNO(hausProx.openHouseMode));
  Serial.println();
  /* Display SD card status */
  print_prog_str(strSDCardStatus);
  print_prog_str(YESNO(hausProx.sdEnabled));
  Serial.println();
  /* Display the door lock status */
  print_prog_str(strDoorStatus);
  print_prog_str(YESNO(hausProx.door.isLocked()));
  Serial.println();
  /* Display the date/time */
  clock.update();
  clock.formatDateTime(input, sizeof(input));
  print_prog_str(strDateStatus);
  Serial.println(input);
}

/******************/
/* Date/time menu */
/******************/

void command_setdate()
{
  /* Display the current date/time. Note the input buffer is long enough to do this. */
  print_datetime();
  while(1) 
  {
    /* Format: YY-MM-DD HH:MM:SS (24 hour clock) */
    read_input(strDateTimePrompt);
    if (input[0] == 0) {
      // User cancelled
      println_prog_str(strAborted);
      break;
    }
    if (clock.setDateTime(input)) {
      // Time was set successfully
      logger.logMessage(LOG_ADMIN, strDateTimeOkay);
      break;
    }
    // There was a problem setting the time
    println_prog_str(strInvalidEntry);
  }
}

/***********/
/* SD menu */
/***********/

void command_test_sd()
{
  /* Check if the card is physically plugged in */
  
  /* Try to initialize the SD library */
//  if (!SD.begin()
  
  /* Try to access the card database */

}

/************************/
/* Card management menu */
/************************/

void card_management_add()
{
  CardInfo info;
  int ret;
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
    if (strlen(input) != SERIAL_LEN) {
      // Bad serial number
      println_prog_str(strInvalidEntry);
      continue;
    }

    // Make sure the serial doesn't already exist
    ret = hausProx.database.lookupCard(input, info);
    if (ret == DATABASE_DOES_NOT_EXIST) {
      // The serial is not taken
      break;
    } else if (ret == DATABASE_SUCCESS) {
      // Serial is already taken
      println_prog_str(strSerialExists);
    } else {
      println_prog_str(hausProx.database.getErrorStr(ret));
      return;
    }
  }
  strcpy(info.serial, input);

  // Prompt the user for the enabled flag
  ret = read_yesno(strActivePrompt);
  if (ret == -1) {
      println_prog_str(strAborted);
      return;
  }
  info.enabled = ret;
  
  // Now attempt to insert the card into the database
  ret = hausProx.database.insertCard(info);
  // Display the result (will also print 'Success' on success)
  println_prog_str(hausProx.database.getErrorStr(ret));
}

void card_management_edit()
{
  // Lookup a card in the database
  CardInfo info;
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
  int ret = read_yesno(strActivePrompt);
  if (ret != -1) {
    info.enabled = ret;
    changed = true;
  }

  if (changed) {
    // Replace an existing card
    ret = hausProx.database.putCard(info.slot, info);
    // Display the result (will also print 'Success' on success)
    println_prog_str(hausProx.database.getErrorStr(ret));
  } else {
    println_prog_str(strAborted);
  }
}

void card_management_delete()
{
  CardInfo info;
  print_prog_str(strDeleteCardTitle);
  if (!read_card(info)) {
    return;
  }

  // Ask the user for confirmation
  print_prog_str(strDeletingCard);
  Serial.println(info.serial);

  if (read_yesno(strConfirmPrompt) != 1) {
    // The user entered a blank line, or entered 'no'
    println_prog_str(strAborted);
    return;
  }
  /* Note that cards are never deleted from the database. Instead the information is blankked-out
   * (tombstoned) and reused later when a card is inserted. */
  info.setBlank();
  int ret = hausProx.database.putCard(info.slot, info);
  // Display the result (will also print 'Success' on success)
  println_prog_str(hausProx.database.getErrorStr(ret));
}

void card_management_menu()
{
  while(1)
  {
    read_input(strCardMenu);
    switch(input[0]) {
      case '1':
        // List cards
        print_cards();
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

/***********************/
/* Log management menu */
/***********************/

void log_management_menu()
{
  while(1) {
    read_input(strLogMenu);
    switch(input[0]) {
      case '1':
        // Dump the log interactively
        log_management_dump(true);
        break;
      case '2':
        // Dump the log
        log_management_dump(false);
        break;
      case '3':
        // Monitor new additions to the log file
        log_management_monitor();
        break;
      case '9':
        return;
    }
  }
}

void log_management_monitor()
{
  println_prog_str(strMonitorLogTitle);
  logger.serialLogging = true;
  while(1) {
    if (Serial.available() > 0) {
      /* Consume the input so it doesn't get picked up elsewhere and stop monitoring */
      while(Serial.available() > 0) Serial.read();
      break;
    }
  }
  logger.serialLogging = false;
  println_prog_str(strAborted);
}

/* Dump the contents of a log file specified by the user. If 'interactive' is true, the user will
 * be prompted to hit enter to scroll through pages of log entries. Otherwise the log file is 
 * printed without pause. */
void log_management_dump(boolean interactive)
{
  int year, month, day;
  
  clock.update();

  if (interactive) {
    println_prog_str(strReviewLogTitle);
  } else {
    println_prog_str(strDumpLogTitle);
  }

  print_datetime();

  // Enter the year (default is current year)
  while(1) {  
    year = read_int(strEnterYear, clock.year);
    if (year >= 0 && year <= 99) break;
    println_prog_str(strInvalidEntry);
  }

  // Enter the month (default is current month)
  while(1) {
    month = read_int(strEnterMonth, clock.month);
    if (month >= 1 && month <= 12) break;
    println_prog_str(strInvalidEntry);
  }
  
  // Enter the day (default is today)
  while(1) {
    day = read_int(strEnterDay, clock.day);
    if (day >= 0 && day <= 31) break;
    println_prog_str(strInvalidEntry);
  }

  sprintf(input, "HP-%02d-%02d.LOG", year, month);
  File file = SD.open(input, FILE_READ);
  if (!file) {
    print_prog_str(strLogNotFound);
    Serial.println(input);
    return;
  }
  
  // For large log files there can be a bit of a delay, so let the user know what's going on
  print_prog_str(strSearchingLog);
  Serial.println(input);
  Serial.println();
  
  boolean done = false, found=false;
  // The number of lines processed in the log file
  int count = 1;
  // The number of lines printed from the log file
  int linesPrinted = 0;
  while(!done)
  {
    /* Since log lines may be arbitrarily long, we read them in small chunks. We want to handle the first
     * chunk specially, since we will use that in conjunction with our date filtering. */
    int len = read_line(&file, input, sizeof(input));
    if (len == 0) {
      break;
    }

    // Whether to output the line or skip over it (ie does it match our filter)
    boolean outputLine = true;
    // Make sure the line is long enough to include a timestamp
    if (day != 0 && len >= 20) {
      // TODO - is there a better way than this?
      char daystr[3];
      daystr[0] = input[8];
      daystr[1] = input[9];
      daystr[2] = 0;
      if (day != atoi(daystr)) {
        // Ignore this line. Note we still need to read in the rest of the line below.
        outputLine = false;
      }
    }
    if (outputLine) {
      found = true;
      linesPrinted++;
      Serial.print('[');
      Serial.print(count);
      Serial.print(']');
      Serial.print(' ');
      Serial.print(input);
    }
    count++;
    
    while(1) {
      /* Read in the next chunk, up to the end of the line */
      int len = read_line(&file, input, sizeof(input));
      if (len == 0) {
        done = true;
        break;
      }

      if (outputLine) {
        /* Dump it to the serial */
        Serial.print(input);
      }
      if (input[len-1] == '\n') {
        /* Found the end of line */
        break;
      }
    }
    /* Let the user break out at any time */
    if (Serial.available() > 0) {
      /* Consume the input so it doesn't get picked up elsewhere */
      while(Serial.available() > 0) Serial.read();
      println_prog_str(strAborted);
      break;
    }
    if (interactive && linesPrinted % LOG_LINES_PER_PAGE == 0) {
      // Wait for user input
      read_input(strPressEnter);
    }
  } 
  file.close();
  
  if (!found) {
    println_prog_str(strNoLogEntries);
  }
}

/*************/
/* Main menu */
/*************/

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
        log_management_menu();
        break;
      case '4':
        command_setdate();
        break;
      case '5':
        command_test_sd();
        break;
      default:
        println_prog_str(strInvalidEntry);
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
  // Turn off serial logging
  logger.serialLogging = false;
}

void loop()
{
  main_menu();
}

