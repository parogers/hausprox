#ifndef __CLOCK_H__
#define __CLOCK_H__

/* The interface to the realtime clock chip (DS3234) */
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
};

#endif

