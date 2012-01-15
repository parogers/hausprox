/*
 * haus|prox - Electronic door access control system
 * Copyright (C) 2011  Peter Rogers @thinkhaus
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

#ifndef __DOOR_H__
#define __DOOR_H__

/* Door interface, featuring locking and unlocking */
class Door
{
  private:
    /* The number of seconds left until the door should be locked again */
    long  lockDoorCountdown;
    /* The latch output pin (HIGH=locked) */
    int latchPin;

  public:
    Door();

    void begin(int pin);
    void lock();
    void unlock(long duration);
    void tick();
    boolean isLocked() { return lockDoorCountdown == 0; }
};

#endif

