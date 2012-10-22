haus|prox - electronic door access system
-----------------------------------------

Source: https://github.com/parogers/hausprox/

Software by Peter Rogers (peter.rogers@gmail.com)
Hardware by Trevyn Watson

Haus|prox is an inexpensive feature-filled RFID card access system.
The software is released under the GPL v3. The hardware design is 
covered by the Creative Commons, Attribution, Non-Commercial license.
The software is covered by GPLv3. Please read LICENSE.txt for details.

* Arduino Duemilanove with ATMega 328 (or equivalent)
* SD card reader
* Arduino program code
* HID brand RFID card reader
* Electronic door strike
* A custom board to connect everything together

Arduino Pinouts
---------------

HID card reader
    CLOCK           Digital 3
    DATA            Digital 4
    PRESENT         Digital 5
    BEEP            Digital 6
Door latch          Digital 2
Open house button   Digital 7
SD card
	Chip select     Digital 10
	MOSI            Digital 11
	MISO            Digital 12
	SCK             Digital 13
RTC chip
	SDA             Analog 4
	SCL             Analog 5

Compiling
---------

You will need the Arduino IDE version 1.0 (http://arduino.cc) and 
the TimerOne library:

* TimerOne - http://code.google.com/p/arduino-timerone/

The code compiles to around 30k so you need a board that can handle 
that. The program was developed and tested on the Arduino Duemilanove 
with the ATMega 328 chip, but other similar boards should work too.

