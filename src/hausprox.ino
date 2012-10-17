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
#include "Const.h"

// Convenience macro for printing yes/no
#define YESNO(b)              ((b) ? strYes : strNo)
// When reviewing log files, the number of lines to print before prompting the user to hit enter
#define LOG_LINES_PER_PAGE    30

#define INPUT_NEWLINE         '\r'

// Length of the user input buffer
#define MAX_INPUT_LEN         25

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
void read_input(const prog_char *msg, boolean echo=true)
{
  int pos = 0, maxlen=MAX_INPUT_LEN;

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
      if (ch == INPUT_NEWLINE) break;
      // Add another character to our buffer
      input[pos++] = ch;
    }
    /* Possibly handle other events */
    hausProx.handleEvents();
  }
  /* Be sure to null terminate the string */
  input[pos] = 0;
  trim(input);
//  if (echo) {
    // Echo the input
  Serial.println(input);
//  }
}

/* Displays the list of cards in the database, prompts the user for a slot number and
 * loads the card data. */
boolean read_card(CardInfo &info)
{
  // Print the card database
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
    int ret = hausProx.database.getCard(slot-1, info);
    if (ret == DATABASE_SUCCESS) {
      return true;
    }
    else if (ret == DATABASE_EOF) {
      println_prog_str(strInvalidEntry);
    } else {
      println_prog_str(CardDatabase::getErrorStr(ret));
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

/* Waits for the user to scan a card at the reader, or press enter to cancel. Returns false if they cancel,
 * true if a card was scanned. */
boolean wait_for_card()
{
  while(!hausProx.reader.hasCardData())
  {
    // Wait for the user to hit enter
    if (Serial.available() > 0 && Serial.read() == INPUT_NEWLINE) {
      return false;
    }
  }
  return true;
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
  int err = hausProx.database.enumerateRecords(print_card_cb);
  if (err != DATABASE_SUCCESS) {
    println_prog_str(CardDatabase::getErrorStr(err));
  }
}

/* Prints the date/time to Serial */
void print_datetime()
{
  clock.update();
  clock.formatDateTime(input, MAX_INPUT_LEN);
  print_prog_str(strDateTimeIs);
  Serial.println(input);
}

/****************/
/* Login screen */
/****************/

void login_screen()
{
  while (1) {
    // Prompt for the password, but don't echo the input
    read_input(strLoginPrompt, false);
    if (hausProx.checkPassword(input)) {
      break;
    }
    delay(1000);
    logger.logMessage(LOG_ADMIN, strLoginDeniedLog);
    print_prog_str(strLoginDenied);
  }
  logger.logMessage(LOG_ADMIN, strLoginMessage);
}

/***************/
/* Status menu */
/***************/

void command_status()
{
  /* Show the version number */
  print_prog_str(strVersionStatus);
  println_prog_str(strVersion);
  /* Display the open house status */
  print_prog_str(strOpenHouseStatus);
  println_prog_str(YESNO(hausProx.openHouseMode));
  /* Display SD card status */
  print_prog_str(strSDCardStatus);
  println_prog_str(YESNO(hausProx.sdEnabled));
  /* Display the door lock status */
  print_prog_str(strDoorStatus);
  println_prog_str(YESNO(hausProx.door.isLocked()));
  /* Display the date/time */
  clock.update();
  clock.formatDateTime(input, MAX_INPUT_LEN);
  print_prog_str(strDateStatus);
  Serial.println(input);
  /* Display the door entry duration */
  print_prog_str(strDoorLenStatus);
  Serial.print(hausProx.doorEntryDuration);
  Serial.println(" s");
  /* Display the open house duration */
  print_prog_str(strOpenLenStatus);
  Serial.print(hausProx.openHouseDuration);
  Serial.println(" s");
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

/********************/
/* Diagnostics menu */
/********************/

void diagnostics_menu()
{
  while(1) {
    read_input(strDiagnosticsMenu);
    switch(input[0]) {
      case '1':
        hausProx.reader.playTestBeep();
        break;
      case '2':
        // Strike test
        hausProx.door.unlock(5);
        break;
      case '3':
        // Card swipe test
        card_management_scan(false);
        break;
      case '9':
        return;
    }
  }

  /* Check if the card is physically plugged in */
  
  /* Try to initialize the SD library */
//  if (!SD.begin()
  
  /* Try to access the card database */

}

/************************/
/* Card management menu */
/************************/

/* Prompts the user to manually add a card to the database */
boolean card_management_add()
{
  CardInfo info;
  int ret;

  // Read the serial number
  while(1) 
  {
    read_input(strCardPrompt);
    if (input[0] == 0) {
      // Aborted adding
      println_prog_str(strAborted);
      return false;
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
    if (ret == DATABASE_RECORD_NOT_FOUND || ret == DATABASE_DOES_NOT_EXIST) {
      // The serial is not taken or the database doesn't exist
      break;
    } else if (ret == DATABASE_SUCCESS) {
      // Serial is already taken
      println_prog_str(strSerialExists);
    } else {
      println_prog_str(CardDatabase::getErrorStr(ret));
      return true;
    }
  }
  strcpy(info.serial, input);

  // Prompt the user for the enabled flag
  ret = read_yesno(strActivePrompt);
  if (ret == -1) {
      println_prog_str(strAborted);
      return false;
  }
  info.enabled = ret;
  
  // Now attempt to insert the card into the database
  ret = hausProx.insertCard(info);
  // Display the result (will also print 'Success' on success)
  println_prog_str(CardDatabase::getErrorStr(ret));
  return true;
}

/* Lets the user scan cards and automatically adds them to the database disabled */
void card_management_scan(boolean add)
{
  println_prog_str(strSwipeNow);
  while(1)
  {
    // Wait for the user to scan a card or cancel
    if (!wait_for_card()) break;
  
    // Read the card data
    CardInfo info;
    int err = hausProx.reader.readCard(info.serial, sizeof(info.serial));

    // Check for errors
    if (err != 0)
    {
      /* Log the error and the contents of the card buffer */
      println_prog_str(CardReader::getErrorStr(err));
      hausProx.reader.printBuffer(Serial);
      Serial.println("");
      continue;
    }

    // Clear the card buffer
    hausProx.reader.clearCardData();

    // Display the serial number
    Serial.println(info.serial);

    if (add) {
      // Add the card but disable it
      info.enabled = false;
      err = hausProx.insertCard(info);
      if (err == DATABASE_SUCCESS) {
        println_prog_str(strCardAdded);
      } else {
        // Display the error
        println_prog_str(CardDatabase::getErrorStr(err));
      }
    }
  }
}

/* User edits a card entry in the database */
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
    // The user either entered yes or no (note blank is default)
    info.enabled = ret;
    changed = true;
  }

  if (changed) {
    // Update the database
    ret = hausProx.updateCard(info);
    // Display the result (will also print 'Success' on success)
    println_prog_str(CardDatabase::getErrorStr(ret));
  } else {
    println_prog_str(strAborted);
  }
}

boolean card_management_delete()
{
  CardInfo info;
  
  // Prompt the user to enter a card number
  if (!read_card(info)) {
    return false;
  }

  // Ask the user for confirmation
  print_prog_str(strDeletingCard);
  Serial.println(info.serial);

  if (read_yesno(strConfirmPrompt) != 1)
  {
    // The user entered a blank line, or entered 'no'
    println_prog_str(strAborted);
    return true;
  }
  /* Delete the card */
  int ret = hausProx.deleteCard(info);
  // Display the result (will also print 'Success' on success)
  println_prog_str(CardDatabase::getErrorStr(ret));
  return true;
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
        print_prog_str(strAddCardTitle);
        while(card_management_add()) ;
        break;
      case '3':
        // Remove entry
        print_prog_str(strDeleteCardTitle);
        while(card_management_delete()) ;
        break;
      case '4':
        // Editing an entry
        card_management_edit();
        break;
      case '5':
        // Scan to add cards
        card_management_scan(true);
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

/* Logs all activity to the serial interface until the user presses enter */
void log_management_monitor()
{
  logger.serialLogging = true;
  read_input(strMonitorLogTitle);
  logger.serialLogging = false;
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
  // The number of lines printed from the log file on the current "screen"
  int linesPrinted = 0;
  while(!done)
  {
    /* Since log lines may be arbitrarily long, we read them in small chunks. We want to handle the first
     * chunk specially, since we will use that in conjunction with our date filtering. */
    int len = read_line(&file, input, MAX_INPUT_LEN);
    if (len == 0) {
      break;
    }

    // Whether to output the line or skip over it (ie does it match our filter)
    boolean outputLine = true;
    // Make sure the line is long enough to include a timestamp
    if (day != 0 && len >= 20) 
    {
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
      int len = read_line(&file, input, MAX_INPUT_LEN);
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
    if (interactive && linesPrinted == LOG_LINES_PER_PAGE) {
      // Wait for user input before continuing, or 'q' to quit
      read_input(strPressEnter);
      if (input[0] == 'q') break;
      linesPrinted = 0;
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
        diagnostics_menu();
        break;
      case '9':
        println_prog_str(strLogoutMessage);
        return;
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
  hausProx.tick();
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
  login_screen();
  main_menu();
}

