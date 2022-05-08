/*

  HC-SR04

  Note: this sketch uses the 3.3V "CS100" version

*/

#include <avr/sleep.h>
#include <RFM69.h>      // https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>  // https://github.com/lowpowerlab/rfm69
#include <SPI.h>

#define debug false

// Constants
const float speed_sound = 337.31; // @10degC see https://en.wikipedia.org/wiki/Speed_of_sound#Tables

// Pins
const int TRIG_PIN = 1; // yellow aka PA5
const int ECHO_PIN = 3; // blue aka PA7

// Anything over 400 cm (23200 us pulse) is "out of range"
const unsigned int MAX_DIST = 23200;

#define NODEID      3   // id of this device
#define NETWORKID   1   // your network id
#define GATEWAYID   1   // id of your gateway
#define FREQUENCY     RF69_433MHZ
//#define ENCRYPTKEY    "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW! See https://lowpowerlab.com/guide/moteino/transceivers/
// #define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
// #define ATC_RSSI      -80 // target RSSI (dBm)
#define SERIAL_BAUD 9600

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

int numRetries = 3; // default is 2
int timeout = 50; // ms to wait for ACK, default is 30

volatile int current_sleeps = 0;        // counter for how many times PIT has woken up
int target_sleeps = 19;                  // target for total sleep time. Increment of the RTC_PERIOD setting below
                                        // e.g. RTC_PERIOD_CYC32768_gc = 32s, target_sleeps = 19 -> ~10 mins sleep

typedef struct {
  float cm;
} Payload;
Payload theData;

// setup for the RTC
void RTC_init(void) {
  // initialise RTC:
  RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;         // 1.024kHz Internal Ultra-Low-Power Oscillator (from OSCULP32K)
  while (RTC.STATUS > 0 || RTC.PITSTATUS);  // Wait for all registers to be synchronised
  RTC.PITINTCTRL = RTC_PI_bm;               // PIT Interrupt: enabled
  RTC.PITCTRLA = RTC_PERIOD_CYC1024_gc      // Num cycles between interrupt. RTC Clock Cycles 1024, resulting in 1024/1024Hz = every 1 seconds
  | RTC_PITEN_bm;                           // enable PIT counter
}

// interrupt subroutine, runs when PIT wakes up
ISR(RTC_PIT_vect) {
  current_sleeps += 1;                  // increment sleep counter
  RTC.PITINTFLAGS = RTC_PI_bm;          // clear interrupt flag by writing '1' (required)
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
  pinMode(PIN_PB3, INPUT_PULLUP);

  if (debug) Serial.begin(SERIAL_BAUD);
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
  if (debug) Serial.print("Transmitting at ");
  if (debug) Serial.print(FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  if (debug) Serial.println(" MHz");

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
    // main code to run here
    unsigned long t1;
    unsigned long t2;
    unsigned long pulse_width;
    float cm;

    // Hold the trigger pin high for at least 10 us
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Wait for pulse on echo pin
    while ( digitalRead(ECHO_PIN) == 0 );

    // Measure how long the echo pin was held high (pulse width)
    // Note: the micros() counter will overflow after ~70 min
    t1 = micros();
    while ( digitalRead(ECHO_PIN) == 1);
    t2 = micros();
    pulse_width = t2 - t1;

    // distance = speed * time
    // distance in cm, speed in m/s, t in us
    // divide by 2 as out and back echo
    cm = speed_sound * (1.0e-4/2.0) * pulse_width;

    // Print out results
    if ( pulse_width > MAX_DIST ) {
      if (debug) Serial.println("Out of range");
      theData.cm = 400.444; // this will be a known out of range value
    } else {
      theData.cm = cm;
    }
    if (debug) Serial.print("Sending struct (");
    if (debug) Serial.print(sizeof(theData));
    if (debug) Serial.print(" bytes): ");
    if (debug) Serial.print(theData.cm);
    if (debug) Serial.print(" ... ");

    if (radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData), numRetries, timeout)) {
      if (debug) Serial.print(" ok!");
    } else {
      if (debug) Serial.print(" nothing...");
    }

    // end main code
    current_sleeps = 0;                           // reset sleep counter to 0
  }
  radio.sleep();  // sleep radio
  sleep_cpu();                                    // sleep the device and wait for an interrupt to continue
}
