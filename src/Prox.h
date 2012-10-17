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
#ifndef __PROX_H__
#define __PROX_H__

/* Prox.h */

#include "Arduino.h"
#include "CardReader.h"
#include "CardDatabase.h"
#include "Logger.h"
#include "utils.h"
#include "Door.h"
#include "Clock.h"

/* The maximum admin password length */
#define PASSWORD_BUF_LEN      16

class HausProx
{
  public:
    CardReader     reader;
    CardDatabase   database;
    Door           door;
    
    /* Whether we are in "open house" mode, where the door is kept open
     * for a pre-determined amount of time */
    boolean        openHouseMode;
    
    /* The open house duration in minutes */
    long           openHouseDuration;
    
    /* How long the door remains open (seconds) when somebody swipes a card */
    long           doorEntryDuration;
    
    /* The admin password */
    char           password[PASSWORD_BUF_LEN];
    
    DebouncedInput openHouseButton;
    
    /* Whether the door was locked the last time we checked */
    boolean        lastDoorLocked;
    
    /* Whether the SD card is enabled for use */
    boolean        sdEnabled;
    
    /* Whether scanning a card is able to currently open the door */
    boolean        readerOpensDoor;

    HausProx();

    void begin();
    void initSDCard();

    void lockDoor();
    void unlockDoor(long duration);
  
    void handleEvents();
    void handleCardScanned();
    void handleOpenHouse();

    // Called once every second to update the internal state
    void tick();

    /* Loads the program config from the SD card (eg password, door open duration, etc) */
    boolean loadConfig();
    /* Compares the given text against the admin password */
    boolean checkPassword(const char *input);
    /* Inserts a card into the database and logs the insertion */
    int insertCard(CardInfo &info);
    /* Removes a card from the database and logs the deletion */
    int deleteCard(CardInfo &info);
    /* Updates a card in the database and logs the update */
    int updateCard(CardInfo &info);

};

#endif

