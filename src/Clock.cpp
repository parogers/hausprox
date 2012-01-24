/* Clock.cpp */
#include <Wire.h>
#include "Arduino.h"

#include "utils.h"
#include "Clock.h"

/* The real-time clock address on the I2C bus */
#define RTC_ADDRESS       B1101000

Clock::Clock()
{
  seconds = 0;
  minutes = 0;
  hours = 0;
  day = 0;
  month = 0;
  year = 0;
}

void Clock::update()
{
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(RTC_ADDRESS, 7);
  if (Wire.available()) 
  {
    seconds = decodeBCD(Wire.read());
    minutes = decodeBCD(Wire.read());
    hours = decodeBCD(Wire.read());
    weekday = decodeBCD(Wire.read());
    day = decodeBCD(Wire.read());
    month = decodeBCD(Wire.read());
    year = decodeBCD(Wire.read());
    working = true;
  } else {
    working = false;
  }

  /* Make sure the time data makes sense */
  /* TODO - log inconsistent data */
  if (year > 99) year = 0;
  if (month > 12) month = 0;
  if (day > 31) day = 0;
  if (hours > 24) hours = 0;
  if (minutes > 60) minutes = 0;
  if (seconds > 60) seconds = 0;
}

void Clock::setDateTime(byte year, byte month, byte day, byte hour, byte mins, byte secs)
{
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write((byte)0);
  Wire.write(encodeBCD(secs));
  Wire.write(encodeBCD(mins));
  Wire.write(encodeBCD(hour));
  Wire.write((byte)0); // TODO - fix this
  Wire.write(encodeBCD(day));
  Wire.write(encodeBCD(month));
  Wire.write(encodeBCD(year));
  Wire.endTransmission();
}

