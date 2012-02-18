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

/* Logger.h */

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <SD.h>

#define LOG_CARD     1
#define LOG_ERROR    2
#define LOG_ADMIN    3  
#define LOG_MESG     4
#define LOG_DOOR     5

class CardReader;

class Logger
{
  private:

  public:
    Logger();

    boolean sdEnabled;

    /* Message format:
     *
     * YYYY/MM/DD hh:mm:ss [TYPE] msg 
     */
    void logMessage(int level, const prog_char *msg);

    /* Message format:
     *
     * YYYY/MM/DD hh:mm:ss [CARDS] msg: facility=nnn, card=nnn, buffer=nnn
     */
    void logMessage(int level, const prog_char *msg, const char *serial, CardReader *reader=NULL);

};

/* The global logger */
extern Logger logger;

#endif

