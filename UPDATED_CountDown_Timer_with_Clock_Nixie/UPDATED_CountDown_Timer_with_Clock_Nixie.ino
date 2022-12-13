// Countdown Timer and clock
// After turning on, use the UP and DOWN buttons to set the time in minutes and seconds
// Push START to initiate coutdown timer
// Press the RESET button if you want to reset before the time has completed
// At the end of the timer, display will show "0" and alarms will begin
// Press RESET button to turn off alarms and reset timer to previous start time 
// Clock comes from the DS3231 RTC
// To view the clock, swith over to "Clock Mode"
// The timer will continue to run even when in clock mode 

// Include the library driver for display:
#include <Wire.h>

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"

RTC_DS3231 rtc;  //I2C address is 0x68

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


// Define pin connections
#define UP_BUTTON 8
#define DOWN_BUTTON 6
#define START_BUTTON 4
#define RESET_BUTTON 5
#define BUZZER 7
#define LIGHT 9 
#define CLOCK_SWITCH 10 


int addr1 = 0x20;  // PCF8574 device 1
int addr2 = 0x21;  // PCF8574 device 2
int timeToDisplay; // the time that should be displayed on the timer
int timerDuration; // used to keep track of the user's desired timer length
int blank = 2;
bool isTimerRunning; //used to track whether or not the timer is counting down

unsigned long milsWhenTimerStarted; //used to keep track of when a timer started

void setup() {
  
  Serial.begin(57600);
  
  rtc.begin();
  
  #ifndef ESP8266
    while (!Serial); // wait for serial port to connect. Needed for native USB
  #endif
  
    if (! rtc.begin()) {
      Serial.println("Couldn't find RTC");
      Serial.flush();
      while (1) delay(10);
    }
  
    if (rtc.lostPower()) {
      Serial.println("RTC lost power, let's set the time!");
      // When time needs to be set on a new device, or after a power loss, the
      // following line sets the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      // This line sets the RTC with an explicit date & time, for example to set
      // January 21, 2014 at 3am you would call:
      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  //rtc.adjust(DateTime(2022, 11, 21, 15, 38, 0));

 
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(LIGHT, OUTPUT); 
  pinMode(CLOCK_SWITCH, INPUT_PULLUP); 

  timeToDisplay = 5;
  milsWhenTimerStarted = millis();
  isTimerRunning = false;
  timerDuration = 5;
    //900 = 15 mins
    //480 = 8 mins
    //180 = 3 mins
   
  // I2C init
  Wire.begin();

  // Turn off all Nixie displays by writing "1111" to all driver BCD inputs
  Wire.beginTransmission(addr1);
  Wire.write(0xFF);
  Wire.endTransmission();

  Wire.beginTransmission(addr2);
  Wire.write(0xFF);
  Wire.endTransmission();
  
}

void loop() {

  DateTime now = rtc.now();
 
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("d");

  // calculate a date which is 7 days, 12 hours, 30 minutes, 6 seconds into the future
  DateTime future (now + TimeSpan(7,12,30,6));

  Serial.print(" now + 7d + 12h + 30m + 6s: ");
  Serial.print(future.year(), DEC);
  Serial.print('/');
  Serial.print(future.month(), DEC);
  Serial.print('/');
  Serial.print(future.day(), DEC);
  Serial.print(' ');
  Serial.print(future.hour(), DEC);
  Serial.print(':');
  Serial.print(future.minute(), DEC);
  Serial.print(':');
  Serial.print(future.second(), DEC);
  Serial.println();

  Serial.print("Temperature: ");
  Serial.print(rtc.getTemperature());
  Serial.println(" C");


  if(digitalRead(CLOCK_SWITCH) == HIGH)
  {
    NixieClock(); 
  }
  else{
    if(isTimerRunning == false){
      ShowTime(timerDuration);
      HandleUserInputTimerStopped();
    }
    else{
      ShowTime(timeToDisplay);
    }
    //user pressed reset
    if(digitalRead(RESET_BUTTON) == LOW)
    {
      Reset(); 
    }
  }
 
  HandleTimerCountdown();

}


void HandleTimerCountdown()
{
  if(timeToDisplay > 0)
  {
    UpdateTimeToDisplay();
    if(timeToDisplay <= 0)
    {
      timeToDisplay = 0;
      TriggerAlarm();
    }
  }
}

void HandleUserInputTimerStopped(){
  //user pressed up button
    if (digitalRead(UP_BUTTON) ==  LOW){
      IncreaseTimerDuration();
    }
    //user pressed down button
    if (digitalRead(DOWN_BUTTON) == LOW){
      DecreaseTimerDuration();
    }
    //user pressed start button
    if (digitalRead (START_BUTTON) == LOW){
      StartTimer();
    }
}

void UpdateTimeToDisplay(){
  int secondsPassed = (millis() - milsWhenTimerStarted) / 1000;
  timeToDisplay = timerDuration - secondsPassed;
}

void StartTimer(){
    timeToDisplay = timerDuration;
    isTimerRunning = true;
    milsWhenTimerStarted = millis();
    delay(250); //delay for 1/4 second to allow time for button to be pressed and release? maybe unecessary?
}

void NixieClock(){
  //this just shows the time on the nixie display until the clock switch is moved to "Timer"

 DateTime now = rtc.now();
  
  if (now.hour() > 12){
    nixiePrint(((now.hour()-12) * 100) + (now.minute()),!digitalRead(blank)); //show time PM in 12 hour format
  }
  else{
  nixiePrint((now.hour() * 100) + (now.minute()),!digitalRead(blank)); //show time
  }

  Serial.println();
  delay(500);
 }

void IncreaseTimerDuration()
{
    if (timerDuration < 180){
    timerDuration += 10;
    }
    else{
    timerDuration += 60;
    }
    delay(250);
}

void DecreaseTimerDuration()
{
    if (timerDuration > 180){
      timerDuration -= 60;
    }
    else{
    timerDuration -= 10;
    }
    delay(250);
}

void TriggerAlarm(){
    if (isTimerRunning == true){
    digitalWrite(BUZZER, HIGH);
    digitalWrite(LIGHT, HIGH);
     }
}

void Reset(){
// if the RESET button is pressed, stop alarms and go back to last time set on timer
    digitalWrite(BUZZER, LOW);
    digitalWrite(LIGHT, LOW); 
    isTimerRunning = false;
}  
  

void ShowTime(int value){
 //Displays the time on the nixie tubes
  int seconds = value % 60; //remainder of dividing the vlaue (total seconds) by 60, which gives seconds
  int minutes = value / 60; //the whole minutes 
  
//show on 4 tube Nixie Display
  nixiePrint((minutes * 100) + seconds,!digitalRead(blank)); 
    //minutes *100 forces the minutes to the first two nixie tubes by adding zeros
    //adding seconds filles the zeros of the last two nixie tubes
    //now you have the looks of a timer with minutes and seconds with leading zeros blanked
  }




//-------------------------------------------------------------------------
// nixiePrint - takes a 4 digit integer and separates out the digits to send to Nixie tubes
//-------------------------------------------------------------------------
void nixiePrint(int number, bool blanking) {
  byte ones,         // separated out digits from input integer
       tens,
       hund,
       thou;
  byte lsb,          // output BCD data for four Nixie drivers
       msb;

  ones = number % 10;         // the result of (a number mod 10) is the least significant digit of the number.
  tens = (number / 10) % 10;  // then dividing by 10 will shift the decimal so the next
  hund = (number / 100) % 10; // digit can be found, etc.
  thou = (number / 1000) % 10;

  // blank leading zeroes for numbers by setting Nixie driver DCBA INPUT_PULLUPs to "1111"
  // e.g. instead of displaying "0049", just display "49" with the left digits turned off
  if (number < 10) {        // if we only have one digit, blank the tens
    tens = 0xFF;
  }
  if (number < 100) {       // if we only have two digits or less, blank the hundreds
    hund = 0xFF;
  }
  if (number < 1000) {      // if we only have three digits or less, blank the thousands
    thou = 0xFF;
  }

  // combine the separate 4 bit digits into the two 8 bit LSB/MSB bytes to go to the GPIO expander
  // LSB:  7..4 = ones   3..0 = tens
  // MSB:  7..4 = hundreds   3..0 = thousands
  // to get the ones into the upper 4 bits of the byte, a left shift occurs 4 times to move the bits
  // then that is "or"ed with the tens, which is already in the lower 4 bit position so the final byte
  // has everything in the right place.  
  // the tens must be masked with 0x0F, where the "F" contains the valid 4 bit "tens" data and the "0" 
  // is forced into any bits that don't contain valid data. A "0" is required for the "or"ing to work   

  lsb = (ones << 4) | (0x0F & tens); // place "tens" in lower 4 bits and "ones" in upper 4 bits of LSB data
  msb = (hund << 4) | (0x0F & thou); // place "thou" in lower 4 bits and "hund" in upper 4 bits of MSB data

  // if the display is blanked, force all 1's to the Nixie control INPUT_PULLUPs
  // to disable all outputs on compatible BCD drivers.  If the driver does
  // not support blanking, the datasheet will detail the display behaviour
  // for an input of DCBA = 1111
  if (blanking == true) {
    lsb = 0xFF;
    msb = 0xFF;
  }

  
// nixiePrint - takes a 4 digit integer and prints the
//              individual digits on 4 Nixie tubes with
//              a PCF8574 GPIO Expander 



  // Send the LSB 8 bits out to "ones" and "tens" Nixie tubes on GPIO expander
  Wire.beginTransmission(addr1);
  Wire.write(lsb);
  Wire.endTransmission();

  // Send the MSB 8 bits out to "hund" and "thou" Nixie tubes on GPIO expander
  Wire.beginTransmission(addr2);
  Wire.write(msb);
  Wire.endTransmission();


}
