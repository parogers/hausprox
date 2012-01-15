#ifndef __CLOCK_H__
#define __CLOCK_H__

/* The interface to the realtime clock chip (DS3234). It uses the SPI library to interface with the bus. The
 * bus is shared with the SD card reader so we have to be careful when using it. */
class Clock
{
  private:
    // The chip select pin for the time chip
    int chipsel;
  
  public:
    Clock();
    
    void begin(int chipsel);
    
    // The hours, minutes, seconds on the last call to 'update'
    int seconds;
    int minutes;
    int hours;
    
    // The date on the last call to 'update'. Note the year ranges 0-99.
    int day;
    int month;
    int year;
  
    // Update the stored time to the current time on the RTC chip
    void update();
    
    // Stores the date and time specified by this object in the RTC
    void storeDateTime();
};

#endif

