/*

  HC-SR04

  Note: this sketch uses the 3.3V "CS100" version

*/

#include <avr/sleep.h>
#include <RFM69.h>      // https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>  // https://github.com/lowpowerlab/rfm69
#include <SPI.h>
#include <util/delay.h>

/* For the ATTiny1614 the following methods are unavailable:
 *
 * ping_timer(), check_timer(), timer_us(), timer_ms() & timer_stop()
 *
 */

#include <NewPing.h>
#include "config.h"

// setup for the RTC
void RTC_init(void) {
  // initialise RTC:
  RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;         // 1.024kHz Internal Ultra-Low-Power Oscillator (from OSCULP32K)
  while (RTC.STATUS > 0 || RTC.PITSTATUS);  // Wait for all registers to be synchronised
  RTC.PITINTCTRL = RTC_PI_bm;               // PIT Interrupt: enabled
  RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc     // Num cycles between interrupt. RTC Clock Cycles 1024, resulting in 32768/1024Hz = every 32 seconds
  | RTC_PITEN_bm;                           // enable PIT counter
}

// interrupt subroutine, runs when PIT wakes up
ISR(RTC_PIT_vect) {
  RTC.PITINTFLAGS = RTC_PI_bm;          // clear interrupt flag by writing '1' (required)
  current_sleeps += 1;                  // increment sleep counter
}

void setup() {
  // switch off unneeded pins for power saving
  pinMode(PIN_PA0, INPUT_PULLUP);
  // PA1 = MOSI
  // PA2 = MISO
  // PA3 = SCK
  // PA4 = NSS
  // PA5 = trigger pin
  // PA6 = DIO0
  // PA7 = echo pin
  pinMode(PIN_PB0, INPUT_PULLUP);
  pinMode(PIN_PB1, INPUT_PULLUP);
  pinMode(PIN_PB2, INPUT_PULLUP);
  // PB3 = power pin

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  #ifdef IS_RFM69HW_HCW
    radio.setHighPower(); //must include this only for RFM69HW/HCW!
  #endif
  #ifdef ENABLE_ATC
    radio.enableAutoPower(ATC_RSSI); // set the target RSSI
  #endif
  #ifdef ENCRYPTKEY
    radio.encrypt(ENCRYPTKEY);
  #endif

  // This pin controls whether the sensor is on or off
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH); // ensure off - inverted

  // The Trigger pin will tell the sensor to range find
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  //Set Echo pin as input to measure the duration of
  //pulses coming back from the distance sensor
  pinMode(ECHO_PIN, INPUT);

  RTC_init();                           // initialise the RTC timer
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // set sleep mode to POWER DOWN mode
  sleep_enable();                       // enable sleep mode, but not going to sleep yet
  sei();                                // enable interrupts
}

void loop() {
  if (current_sleeps == 0 || current_sleeps == target_sleeps ) { // just switched on, or we've hit our target
    // switch on sensor - inverted
    digitalWrite(POWER_PIN, LOW);
    _delay_us(10); // allow chip time to power on

    echoTime = sonar.ping_median(5);

    // switch off sensor - inverted
    digitalWrite(POWER_PIN, HIGH);

    // distance = speed * time
    // distance in cm, speed in m/s, t in us
    // divide by 2 as out and back echo
    theData.cm = speed_sound * (1.0e-4/2.0) * echoTime;

    if (radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData), numRetries, timeout)) {
      ;
    }

    // end main code
    current_sleeps = 0;                           // reset sleep counter to 0
    radio.sleep();  // sleep radio
  }
  sleep_cpu();                                    // sleep the device and wait for an interrupt to continue
}
