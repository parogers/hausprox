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

/* Door.cpp */

#include "WProgram.h"
#include "Door.h"

Door::Door(int pin)
{
  latchPin = pin;
  pinMode(latchPin, OUTPUT);
  lockDoorCountdown = 0;
}

void Door::lock()
{
  // Disable interrupts
  cli();
  lockDoorCountdown = 0;
  digitalWrite(latchPin, LOW);
  // Re-enable interrupts
  sei();
}

void Door::unlock(long duration)
{
  // Disable interrupts 
  cli();
  if (duration > 0) {
    lockDoorCountdown = duration;
    digitalWrite(latchPin, HIGH);
  }
  // Re-enable interrupts
  sei();
}

void Door::tick()
{
  /* This function is called from within an interrupt handler, so there's no need to
   * lock out the variables. */
  if (lockDoorCountdown > 0) {
    lockDoorCountdown--;
    if (lockDoorCountdown == 0) {
      /* Lock the door again */
      digitalWrite(latchPin, LOW);
    }
  }
}

