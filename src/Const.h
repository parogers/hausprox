#ifndef __CONST_H__
#define __CONST_H__

/***********/
/* Strings */
/***********/

PROGMEM const prog_char strVersion[] = {"v1.01"};

// Login screen
PROGMEM const prog_char strLoginPrompt[] = {"\nWelcome to HAUS|PROX\n====================\n\nPassword: "};
PROGMEM const prog_char strLoginDenied[] = {"Invalid password\n"};
PROGMEM const prog_char strLoginDeniedLog[] = {"Login failed"};
PROGMEM const prog_char strLoginMessage[] = {"Admin login"};
PROGMEM const prog_char strLogoutMessage[] = {"Logout"};

// Strings for main screen
PROGMEM const prog_char strMainMenu[] = {"\n**haus|prox**\n\n[1] Status\n[2] Manage cards\n[3] Manage log files\n[4] Change date/time\n[5] Diagnostics\n[9] Logout\n\n> "};

// Strings for status menu
PROGMEM const prog_char strVersionStatus[] = {"Version:        "};
PROGMEM const prog_char strSDCardStatus[] = {"SD enabled:     "};
PROGMEM const prog_char strDoorStatus[] = {"Door locked:    "};
PROGMEM const prog_char strOpenHouseStatus[] = {"Open house:     "};
PROGMEM const prog_char strDateStatus[] = {"Date/time:      "};
PROGMEM const prog_char strDoorLenStatus[] = {"Door entry len: "};
PROGMEM const prog_char strOpenLenStatus[] = {"Open house len: "};

// Strings for date/time
PROGMEM const prog_char strDateTimePrompt[] = {"Enter YY-MM-DD HH:MM:SS? "};
PROGMEM const prog_char strDateTimeOkay[] = {"Date/time changed"};
PROGMEM const prog_char strDateTimeIs[] = {"Time is "};

// Strings for card management screen
PROGMEM const prog_char strCardMenu[] = {"\n**Card Management**\n\n[1] List cards\n[2] Add\n[3] Delete\n[4] Edit\n[5] Scan to add\n[9] Back to main\n\n> "};
PROGMEM const prog_char strAddCardTitle[] = {"\n**Add cards**\n\n"};
PROGMEM const prog_char strEditCardTitle[] = {"\n**Edit cards**\n\n"};
PROGMEM const prog_char strDeleteCardTitle[] = {"\n**Delete cards**\n\n"};
PROGMEM const prog_char strSlotPrompt[] = {"Slot number? "};
PROGMEM const prog_char strEditingCard[] = {"Editing card "};
PROGMEM const prog_char strDeletingCard[] = {"Deleting card "};
PROGMEM const prog_char strCardPrompt[] = {"Serial? (FFF-CCCCC) "};
PROGMEM const prog_char strActivePrompt[] = {"Card active? "};
PROGMEM const prog_char strConfirmPrompt[] = {"Confirm? "};
PROGMEM const prog_char strSerialExists[] = {"Serial number taken"};

// Strings for printing the card database
PROGMEM const prog_char strActive[] = {" - active"};
PROGMEM const prog_char strDisabled[] = {" - disabled"};
PROGMEM const prog_char strBlank[] = {" - blank"};

// Strings for log management
PROGMEM const prog_char strLogMenu[] = {"\n**Log Management**\n\n[1] Review log\n[2] Dump\n[3] Monitor\n[9] Back to main\n\n> "};
PROGMEM const prog_char strReviewLogTitle[] = {"\n**Review log**\n"};
PROGMEM const prog_char strDumpLogTitle[] = {"\n**Dump log**\n"};
PROGMEM const prog_char strMonitorLogTitle[] = {"\n**Monitoring log**\n\nPress enter to stop"};
PROGMEM const prog_char strLogInstructions[] = {"\n\nEnter blank to use current year, month or day\n\n"};
PROGMEM const prog_char strEnterYear[] = {"Year? (2 digits) "};
PROGMEM const prog_char strEnterMonth[] = {"Month? "};
PROGMEM const prog_char strEnterDay[] = {"Day? (0=all) "};
PROGMEM const prog_char strLogNotFound[] = {"Log not found: "};
PROGMEM const prog_char strNoLogEntries[] = {"No entries found"};
PROGMEM const prog_char strSearchingLog[] = {"Scanning log: "};
PROGMEM const prog_char strPressEnter[] = {"\nEnter to continue, 'q' to stop.\n"};

// Strings for the diagnostics menu
PROGMEM const prog_char strDiagnosticsMenu[] = {"\n**Diagnostics**\n\n[1] Beep test\n[2] Strike test\n[3] Card swipe test\n[9] Back to main\n\n> "};
PROGMEM const prog_char strSwipeNow[] = {"\nSwipe your cards now (enter to stop)"};

// Generall-purpose strings
PROGMEM const prog_char strYes[] = {"yes"};
PROGMEM const prog_char strNo[] = {"no"};
PROGMEM const prog_char strInvalidEntry[] = {"Invalid entry"};
PROGMEM const prog_char strAborted[] = {"Aborted"};
PROGMEM const prog_char strSuccess[] = {"Success"};
PROGMEM const prog_char strCardAdded[] = {"Card added"};

PROGMEM const prog_char strCardError[] = {"Error reading card"};
PROGMEM const prog_char strValidOpenHouse[] = {"Admit entry (open house)"};
PROGMEM const prog_char strAdmitEntry[] = {"Admit entry"};
PROGMEM const prog_char strDenyDisabledCard[] = {"Deny disabled card"};
PROGMEM const prog_char strDenyUnregCard[] = {"Deny unregistered card"};
PROGMEM const prog_char strAdminDenied[] = {"Admin access denied"};
PROGMEM const prog_char strBootupMessage[] = {"haus|prox bootup"};
PROGMEM const prog_char strOpenHouseOn[] = {"Turn on open house"};
PROGMEM const prog_char strOpenHouseOff[] = {"Turn off open house"};
PROGMEM const prog_char strOpenHouseExpired[] = {"Open house expired"};
PROGMEM const prog_char strDoorLocked[] = {"Door is locked"};
PROGMEM const prog_char strDoorUnlocked[] = {"Door is unlocked"};
PROGMEM const prog_char strDoorAlreadyUnlocked[] = {"Door already unlocked"};
PROGMEM const prog_char strSDInitFail[] = {"Failed to init SD"};

PROGMEM const prog_char strConfigPass[] = {"password"};
PROGMEM const prog_char strConfigOpenDoor[] = {"open-door-len"};
PROGMEM const prog_char strConfigOpenHouse[] = {"open-house-len"};
PROGMEM const prog_char strConfigInvalid[] = {"Invalid config: "};
PROGMEM const prog_char strConfigBadLine[] = {"Invalid config"};
PROGMEM const prog_char strErrorLoadingConfig[] = {"Error loading config"};

PROGMEM const prog_char strInsertedCard[] = {"Add card"};
PROGMEM const prog_char strDeletedCard[] = {"Remove card"};
PROGMEM const prog_char strUpdatedCard[] = {"Update card"};

PROGMEM const prog_char strDatabaseFailure[] = {"Failure"};
PROGMEM const prog_char strDatabaseNotFound[] = {"Record not found"};
PROGMEM const prog_char strDatabaseOpenFail[] = {"Failed to open card DB"};
PROGMEM const prog_char strInvalidRecord[] = {"Invalid record in card DB"};
PROGMEM const prog_char strRecordTooLong[] = {"Record too long"};
PROGMEM const prog_char strRecordTooShort[] = {"Record too short"};
PROGMEM const prog_char strDatabaseEOF[] = {"Database EOF"};
PROGMEM const prog_char strDatabaseDoesNotExist[] = {"Database does not exist"};

PROGMEM const prog_char strPrematureEnd[] = {"Premature end of data"};
PROGMEM const prog_char strParityFail[] = {"Parity fail"};
PROGMEM const prog_char strInvalidBegin[] = {"Invalid start segment"};
PROGMEM const prog_char strLRCFail[] = {"LRC fail"};
PROGMEM const prog_char strTrailingZeros[] = {"Expected trailing 0s"};
PROGMEM const prog_char strPaddingFail[] = {"Pad fail"};
PROGMEM const prog_char strLeadingZeros[] = {"Expected leading 0s"};
PROGMEM const prog_char strUnknown[] = {"Unknown error"};

PROGMEM const prog_char strCardType[] = {"[CARD] "};
PROGMEM const prog_char strAdminType[] = {"[ADMN] "};
PROGMEM const prog_char strMessageType[] = {"[MESG] "};
PROGMEM const prog_char strErrorType[] = {"[ERRR] "};
PROGMEM const prog_char strDoorType[] = {"[DOOR] "};
PROGMEM const prog_char strSerialPart[]  = {", serial="};
PROGMEM const prog_char strBufferPart[] = {", buffer="};
PROGMEM const prog_char strLogOpenFail[] = {"Failed to open log file"};

#endif

