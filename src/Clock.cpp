/* Clock.cpp */
#include "WProgram.h"
#include "Clock.h"
#include <SPI.h>

Clock::Clock()
{
  chipsel = -1;
  seconds = 0;
  minutes = 0;
  hours = 0;
  day = 0;
  month = 0;
  year = 0;
}

void Clock::begin(int sel)
{
  chipsel = sel;
  pinMode(chipsel, OUTPUT);
  digitalWrite(chipsel, HIGH);

  // Initialize SPI
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1);
}

void Clock::update()
{
  int buf[7];
  int n;
  
  if (chipsel == -1) {
    // Need to call 'begin' first
    return;
  }
  
  // Enable the rtc chip
  digitalWrite(chipsel, LOW);
  // Start reading data from address 0x00 (start of time registers)
  SPI.transfer(B00000000);

  for (n = 0; n < 7; n++)
  {
    int data = SPI.transfer(0);
    int low = data & B00001111;
    int high = 0;

    switch(n) {
      case 2:
        // Hours
        if (data & B00010000) high = 1;
        else if (data & B00100000) high = 2;
        break;

      case 5:
        // Month
        if (data & B00010000) high = 1;
        break;

      default:
//      case 0:
//      case 1:
//      case 4:
//      case 6:
        // Seconds, minutes, day and year
        high = (data >> 4) & B00001111;
        break;
    }
    buf[n] = 10*high + low;
  }
  // Disable the rtc
  digitalWrite(chipsel, HIGH);

  seconds = buf[0];
  minutes = buf[1];
  hours = buf[2];
  day = buf[4];
  month = buf[5];
  year = buf[6];
}

void Clock::storeDateTime()
{
  // Enable the chip
  digitalWrite(chipsel, LOW);
  SPI.transfer(0x80);
  SPI.transfer(B01010000); // seconds
  SPI.transfer(B01011001); // minutes
  SPI.transfer(B00011001); // hours
  digitalWrite(chipsel, HIGH);
}

