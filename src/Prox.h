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

    void lock_door();
    void unlock_door(long duration);
  
    void handle_events();
    void handle_card_scanned();
    void handle_open_house();

    /* Loads the program config from the SD card (eg password, door open duration, etc) */
    boolean load_config();
    /* Compares the given text against the admin password */
    boolean check_password(const char *input);

};

#endif

