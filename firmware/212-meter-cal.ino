#include <avr/sleep.h>
#include <EEPROM.h>
#include <USERSIG.h>

const byte LEDL = 15;
byte previousState = 0;
byte currentState = 0;
byte calFactor = 100;
uint16_t highestADC = 0;
uint16_t currentADC = 0;
uint16_t correctedVal = 0;
uint16_t loopCount = 0;

ISR(RTC_PIT_vect) {
  RTC.PITINTFLAGS = RTC_PI_bm;          // Clear interrupt flag
}

ISR(PORTA_PORT_vect) {
  _PROTECTED_WRITE(RSTCTRL.SWRR,1);     // we have been woken up from long sleep, reset
}

void RTC_arm(void) {                     // Arming the nugget
  while (RTC.STATUS > 0)
  {
    ;                                   // Wait for all registers to synchronize
  }
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;    // 32.768kHz Internal Ultra-Low-Power Oscillator (OSCULP32K)
  RTC.PITINTCTRL = RTC_PI_bm;           // PIT Interrupt: enabled
  RTC.PITCTRLA = RTC_PERIOD_CYC4096_gc  // RTC Clock Cycles 8192, resulting in 32.768kHz/4096 = 8Hz
                 | RTC_PITEN_bm;        // Enable PIT counter: enabled
}

void RTC_disarm() {
  RTC.PITCTRLA = RTC_PERIOD_OFF_gc &~ RTC_PITEN_bm;
}

void prepareSleep() {                   // don't leave any pins floating before going to sleep
  digitalWriteFast(2, LOW);
  pinModeFast(2, OUTPUT);
  digitalWriteFast(3, LOW);
  pinModeFast(3, OUTPUT);
  digitalWriteFast(4, LOW);
  pinModeFast(4, OUTPUT);
}

void pinModeCharlieOne() {              // saves a few bytes
  pinModeFast(3, OUTPUT);
  pinModeFast(4, OUTPUT);
  pinModeFast(2, INPUT);
}

void pinModeCharlieTwo() {
  pinModeFast(3, INPUT);
  pinModeFast(4, OUTPUT);
  pinModeFast(2, OUTPUT);
}

void leftred() {                         // all the LED combinations
  pinModeCharlieOne();
  digitalWriteFast(4, LOW);
  digitalWriteFast(3, HIGH);
  delay(LEDL);
  digitalWriteFast(3, LOW);
}

void leftgreen() {
  pinModeCharlieTwo();
  digitalWriteFast(2, LOW);
  digitalWriteFast(4, HIGH);
  delay(LEDL);
  digitalWriteFast(4, LOW);
}

void rightgreen() {
  pinModeCharlieOne();
  digitalWriteFast(3, LOW);
  digitalWriteFast(4, HIGH);
  delay(LEDL);
  digitalWriteFast(4, LOW);
}

void rightred() {
  pinModeCharlieTwo();
  digitalWriteFast(4, LOW);
  digitalWriteFast(2, HIGH);
  delay(LEDL);
  digitalWriteFast(2, LOW);
}

void leftyellow () {                      // LED helpers
  leftred();
  leftgreen();
}

void rightyellow () {
  rightred();
  rightgreen();
}

void bothgreen () {
  rightgreen();
  leftgreen();
}

byte checkswitch() {                                 // voltage divider cutoffs: 0, 360, 725
  uint16_t switchState = analogRead(1);
  if (switchState < 50) {                            // high switch setting  - WritPw
    return 3;
  }
  if (switchState > 335 && switchState < 385) {      // low switch setting - LrefPw
    return 1;
  }
  if (switchState > 700 && switchState < 750) {      // middle switch setting - HrefPw
    return 2;
  }
  return 0;                                          // something went wrong
}

byte checkswitchDb() {                                // deliberately debounce with delay so we can decide before sleeping for 125ms
  analogReference(VDD);
  analogRead(1);                                      // discard first readout
  byte switchCounter = 0;
  byte currentReading = checkswitch();
  byte lastState = 0;

  for (byte i = 0; i <= 4; i++) {
    lastState = currentReading;
    delay(3);
    currentReading = checkswitch();
    if (lastState == currentReading) switchCounter++;
  }
  if (switchCounter == 5) {                            // if we got 5 identical readings in 15ms then we are good
    return currentReading;
  }
  else {                                               // we try again next time
    return 0;
  }
}

void setup() {

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);                  // prepare sleep mode
  sleep_enable();
  RTC_arm();                                            // prepare PIT/RTC to wake up later

  pinModeFast(0, INPUT);                                // photodiode
  pinModeFast(1, INPUT);                                // switch
  pinModeFast(2, INPUT);                                // charlieplex
  pinModeFast(3, INPUT);                                // charlieplex
  pinModeFast(4, INPUT);                                // charlieplex
  // 5 is UPDI

  analogReference(VDD);                                 // save battery voltage level
  VREF.CTRLA = VREF_ADC0REFSEL_1V5_gc;
  analogSampleDuration(13);                             // no rush
  uint16_t battery = analogRead(ADC_INTREF);            // discard
  battery = analogRead(ADC_INTREF);
  EEPROM.put(48, battery);                              // battery goes to EEPROM offset 48

  analogReference(INTERNAL2V5);                         // reset ADC reference

  calFactor = USERSIG.read(0);                          // read device calibration factor
  // this region of EEPROM is safe from chip erase
  
  leftred();                                            // LED check
  leftgreen();
  rightgreen();
  rightred();

}

void loop() {
  currentState = checkswitchDb();
  if (currentState != previousState) {                 // switch has moved since last time, we have work to do
    switch (previousState) {                           // save the last sensor reading to EEPROM
      case 3:                                          // Writ goes to EEPROM offset 0
        EEPROM.put(0, highestADC);
        break;
      case 2:                                          // Href goes to offset 16
        EEPROM.put(16, highestADC);
        break;
      case 1:                                          // Lref goes to offset 32
        EEPROM.put(32, highestADC);
        break;
    }
    highestADC = 0;                                    // reset global counters
    loopCount = 0;
    switch (currentState) {                            // blink LED to indicate a change was noted
      case 1:
        rightred();
        break;
      case 2:
        rightyellow();
        break;
      case 3:
        rightgreen();
        break;
    }
    previousState = currentState;
  }

  switch (currentState) {                               // use appropriate reference for switch state
    case 1:
      analogReference(INTERNAL1V1);
      break;
    case 2:
      analogReference(INTERNAL1V1);
      break;
    case 3:
      analogReference(VDD);                             // full range of ADC needed to reach 5mW
      break;
  }
  currentADC = analogRead(0);                           // discard
  currentADC = analogRead(0);                           // keep track of highest recorded value on photodiode
  if (currentADC > highestADC) {
    highestADC = currentADC;
  }
  
  loopCount++;
  if (loopCount % 12 == 0) {                             // every 12 measurements (~1.8 second) indicate result
    switch (currentState) {
      case 1:                                            // LRef
      correctedVal = (highestADC * calFactor);
        if (correctedVal < 25900) leftred();
        if (correctedVal >= 25900 && correctedVal < 29500) leftyellow();
        if (correctedVal >= 29500 && correctedVal < 33100) leftgreen();
        if (correctedVal >= 33100 && correctedVal < 40400) bothgreen();
        if (correctedVal >= 40400 && correctedVal < 44000) rightgreen();
        if (correctedVal >= 44000 && correctedVal < 47700) rightyellow();
        if (correctedVal >= 47700) rightred();
        break;
      case 2:                                             // Href
      correctedVal = (highestADC * calFactor);
        if (correctedVal < 30400) leftred();
        if (correctedVal >= 30400 && correctedVal < 35000) leftyellow();
        if (correctedVal >= 35000 && correctedVal < 39500) leftgreen();
        if (correctedVal >= 39500 && correctedVal < 48600) bothgreen();
        if (correctedVal >= 48600 && correctedVal < 53100) rightgreen();
        if (correctedVal >= 53100 && correctedVal < 57700) rightyellow();
        if (correctedVal >= 57700) rightred();
        break;
      case 3:                                              // Writ
      // no need for high accuracy
        if (highestADC < 720) leftred();
        if (highestADC >= 720 && highestADC < 780) leftyellow();
        if (highestADC >= 780 && highestADC < 840) leftgreen();
        if (highestADC >= 840 && highestADC < 960) bothgreen();
        if (highestADC >= 960 && highestADC < 1020) rightgreen();
        // if (highestADC >= 980 && highestADC < 1020) rightyellow(); we run out of range
        if (highestADC >= 1020) rightred();
        break;
    }

    if (currentState == 3 && loopCount % 4500 == 0) {       // if user hasn't done anything for ~10 minutes 
      rightred();                                           // express displeasure
      leftred();
      RTC_disarm();                                         // sleep until woken from switch interrupt
      PORTA.PIN7CTRL  = 0b00000001;      
   }

  }
  prepareSleep();
  sleep_cpu(); // wait for the prince's kiss
}
