/*
  S88 occupancy sensor interface 64 pins to Base Station V2.0 with S88 bus interface

  Original design by Ruud Boer, November 2014 for 16 pins (RB-S88-V03).
  This sketch is only for Arduino Mega and allows sensors up to 64 pins ( pin 6-53 and A0-A15(54-69) )
  Freely distributable for private, non commercial, use.

  Sketch uses 2578 bytes (1%) of program storage space. Maximum is 253952 bytes.
  Global variables use 155 bytes (1%) of dynamic memory, leaving 8037 bytes for local variables. Maximum is 8192 bytes.

  Connections for S88 bus:
  s88 pin 1 data  - MEGA pin 5 (DataOut)
  s88 pin 2 GND   - MEGA pin GND
  s88 pin 3 CLOCK - MEGA pin 2 (interrupt 0)
  s88 pin 4 PS    - MEGA pin 3 (interrupt 1)
  s88 pin 5 data  - MEGA pin 4 (DataIn)
  s88 pin 6 VSS   - MEGA pin Vin (5V)
  
  REMARK: inputs have the internal pullup resistor active, the sensor must pull the input to GND.
  In RocRail you must set sensor to active low

  Boolean Array definition for sensors bit vs pin number
  First 8 pins has 2+6 Dupont connector, every next 8 pins are a dupont 8 pin connector for UTP cable or flat cable.
  please note that blocks 3,4,5,6 pin numbers will be odd/even when using 8 pin flat connector (see pin layout)!

Pin Layout
 _________     _________     _________     _________
/ bit| pin\   / bit| pin\   / bit| pin\   / bit| pin\
\_________/   \_________/   \_________/   \_________/
  block1        block 3       block 5       block 7
 ---------     ---------     ---------     ---------
/  0 |  6 \   / 16 | 22 \   / 32 | 38 \   / 48 | 54 \
\  1 |  7 /   | 18 | 24 |   | 34 | 40 |   | 49 | 55 |
/  2 |  8 \   | 20 | 26 |   | 36 | 42 |   | 50 | 56 |
|  3 |  9 |   | 22 | 28 |   | 38 | 44 |   | 51 | 57 |
|  4 | 10 |   | 24 | 30 |   | 40 | 46 |   | 52 | 58 |
|  5 | 11 |   | 26 | 32 |   | 42 | 48 |   | 53 | 59 |
|  6 | 12 |   | 28 | 34 |   | 44 | 50 |   | 54 | 60 |
\  7 | 13 /   \ 30 | 36 /   \ 46 | 52 /   \ 55 | 61 /
 ----=----     ---------     ----=----     ----=---- 
  block 2       block 4       block 6       block 8
 ---------     ---------     ---------     ---------
/  8 | 14 \   / 17 | 23 \   / 33 | 39 \   / 56 | 62 \
|  9 | 15 |   | 19 | 25 |   | 35 | 41 |   | 57 | 63 |
| 10 | 16 |   | 21 | 27 |   | 37 | 43 |   | 58 | 64 |
| 11 | 17 |   | 23 | 29 |   | 39 | 45 |   | 59 | 65 |
| 12 | 18 |   | 25 | 31 |   | 41 | 47 |   | 60 | 66 |
| 13 | 19 |   | 27 | 33 |   | 43 | 49 |   | 61 | 67 |
| 14 | 20 |   | 29 | 35 |   | 45 | 51 |   | 62 | 68 |
\ 15 | 21 /   \ 31 | 37 /   \ 47 | 53 /   \ 63 | 69 /
 ---------     ---------     ---------     ---------
 */
/////////////////////////////////////////////////////////////////////////////////////
// RELEASE VERSION
/////////////////////////////////////////////////////////////////////////////////////

#define VERSION "S88-X64-V01"

/////////////////////////////////////////////////////////////////////////////////////
// IDENTIFY ARDUINO BOARD
/////////////////////////////////////////////////////////////////////////////////////

#ifdef ARDUINO_AVR_MEGA                   // is using Mega 1280, define as Mega 2560 (pinouts and functionality are identical)
  #define ARDUINO_AVR_MEGA2560
#endif

#if defined  ARDUINO_AVR_MEGA2560

  #define ARDUINO_TYPE    "MEGA"

#else

  #error CANNOT COMPILE - S88-X64 ONLY WORKS WITH AN ARDUINO MEGA 1280/2560

#endif

 
int SensorPins   = 64;
int ClockCounter = 0;

// As MEGA does not have 64 bit Long unsigned integers, a boolean array is used to hold 64 bits of sensor data
boolean SensorArray[64];
boolean DataArray[64];

const byte DataClock = 2; // clock output
const byte DataLoad  = 3; // load trigger port
const byte DataIn    = 4; // data input pin from S88 occupancy device in chain
const byte DataOut   = 5; // data output pin to management station or to previous S88 occupancy in chain


void setup() {
 
  //
  // pin 4 = DataIn from next S88 occupancy unit in chain
  pinMode(DataIn, INPUT_PULLUP); 
  // pin 5 = DataOut to management station or to previous S88 occupancy device in chain
  pinMode(DataOut, OUTPUT); 

  // Arduino Mega pin 6-53, A0-A15(54-69) set for input with pullup
  for (int pincount = 6; pincount < 70;) pinMode(pincount++, INPUT_PULLUP);
  //
  // clear boolean arrays for predefined state
  // clear sensor array 
  for (int pincount = 0; pincount < SensorPins;) SensorArray[pincount++] = false;
  // clear data array
  for (int pincount = 0; pincount < SensorPins;) DataArray[pincount++] = false;

  // define input pins for CLOCK and PS
  pinMode(DataClock, INPUT_PULLUP);
  pinMode(DataLoad,  INPUT_PULLUP);
  //
  //attach interrupts to pins
  attachInterrupt(digitalPinToInterrupt(DataClock), CLOCK, RISING); // pin DataClock = CLOCK interrupt
  attachInterrupt(digitalPinToInterrupt(DataLoad),  PS,    RISING); // pin DataLoad  = PS interrupt

// Serial.begin(115200);  // Used for test purposes only (enabled will cost 1026 bytes of memory)
}

void loop() {
  // Main loop
  // read 64 pins and update the SensorArray accordingly (false/tree)
  for (int pincount = 6; pincount < 70; pincount++) {
    if (!digitalRead(pincount)) bitSet(SensorArray[pincount - 6], true);
  }

 /*
  // Testing only
  Serial.print("sensor - ");
  for (int pincount = 0; pincount < sensorpins;) Serial.print(sensorarray[pincount++], BIN);
  Serial.println(":");
  delay(1000);
*/
}

void PS() {
  // interrupt pin 3 for data load action
  ClockCounter = 0;
  // copy sensor array on trigger to data buffer
  for (int pincount = 0; pincount < SensorPins; pincount++) {
    DataArray[pincount] = SensorArray[pincount];
  }

  // clear sensor array 
  for (int pincount = 0; pincount < SensorPins;) SensorArray[pincount++] = false;
}

void CLOCK() {
  // interrupts pin 2 for shifting data through this device.
  // write output bit to s88 controller
  digitalWrite(DataOut, DataArray[ClockCounter]);
  //Delay makes reading output signal from next Arduino in chain more reliable.
  delayMicroseconds(16); 
  // read bit from S88 sensor in link
  bitWrite(DataArray[ClockCounter], ClockCounter, digitalRead(DataIn));
  //select next bit to be send out
  ClockCounter = (ClockCounter + 1) % SensorPins;
}
