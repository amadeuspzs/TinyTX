#line 1 "/home/amadeus/dev/TinyTX/software/ping/ping.ino"
/*

  Test sketch to get your TinyTX up and running.

  You will need a receiver (gateway) set up to receive packets.

  e.g. https://github.com/amadeuspzs/rfm69-pi-avr/tree/master/hub-pi

  Packets will be toggled between 0 and 1 every 5 seconds, providing a ping

  LED will blink once for transmit, and flash for received ACK

  NOTE: RFM69 library pulled directly from GitHub (last tested 2026-06-09)
        This reflects the latest API and may differ from Arduino Library Manager versions

*/
#include <Arduino.h>
#include <RFM69.h>     // https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h> // https://github.com/lowpowerlab/rfm69
#include <SPI.h>

#define debug true  // enable Serial output
#define led 7       // positive lead of LED
#define NODEID 2    // id of this device
#define NETWORKID 1 // your network id
#define GATEWAYID 1 // id of your gateway
#define FREQUENCY RF69_433MHZ
// #define ENCRYPTKEY    "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW // uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW! See https://lowpowerlab.com/guide/moteino/transceivers/
#define ENABLE_ATC     // comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI -80   // target RSSI (dBm)
#define SERIAL_BAUD 9600

#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif

int numRetries = 3; // default is 2
int timeout = 50;   // ms to wait for ACK, default is 30

int TRANSMITPERIOD = 5000; // transmit a packet to gateway so often (in ms)

typedef struct
{
  bool state;
} Payload;
Payload theData;

#line 51 "/home/amadeus/dev/TinyTX/software/ping/ping.ino"
void setup();
#line 110 "/home/amadeus/dev/TinyTX/software/ping/ping.ino"
void loop();
#line 51 "/home/amadeus/dev/TinyTX/software/ping/ping.ino"
void setup()
{
  delay(2000);
  if (debug)
  {
    Serial.begin(SERIAL_BAUD);
    delay(100); // give Serial time to initialize
    // Wait for serial connection with 5 second timeout
    // This allows you to connect the monitor and see messages
    uint32_t startTime = millis();
    while (!Serial && (millis() - startTime < 5000))
    {
      delay(100);
    }
    delay(100); // extra delay to ensure serial is ready
    // Print startup messages
    Serial.println("\n\n=== TinyTX Ping Starting ===");
  }

  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  // v1.6.0 changed default to 433.92MHz - revert to 433.0MHz for compatibility with older hubs
  radio.setFrequency(433000000);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); // must include this only for RFM69HW/HCW!
#endif
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI); // set the target RSSI
#endif
#ifdef ENCRYPTKEY
  radio.encrypt(ENCRYPTKEY);
#endif

  // Verify actual frequency register (catches library version mismatches)
  if (debug)
  {
    uint32_t frf = ((uint32_t)radio.readReg(0x07) << 16) |
                   ((uint32_t)radio.readReg(0x08) << 8) |
                   radio.readReg(0x09);
    float freq_mhz = (frf * 61.03515625) / 1000000.0;
    Serial.print("Radio actual frequency: ");
    Serial.print(freq_mhz, 2);
    Serial.println(" MHz");
  }

  pinMode(led, OUTPUT);   // blinker
  digitalWrite(led, LOW); // turn off
}

void blink(int times = 1, int interval = 100)
{
  for (int n = 0; n < times; n++)
  {
    digitalWrite(led, HIGH);
    delay(interval);
    digitalWrite(led, LOW);
    delay(interval);
  }
}

void loop()
{
  // toggle state
  theData.state = (theData.state == 0) ? 1 : 0;

  if (debug)
    Serial.print("Sending struct (");
  if (debug)
    Serial.print(sizeof(theData));
  if (debug)
    Serial.print(" bytes): ");
  if (debug)
    Serial.print(theData.state);
  if (debug)
    Serial.print(" ... ");

  blink(); // blink to transmit

  if (radio.sendWithRetry(GATEWAYID, (const void *)(&theData), sizeof(theData), numRetries, timeout))
  {
    if (debug)
      Serial.print(" ok!");

    // Show current power level after successful ACK
    if (debug)
    {
      uint8_t pa = radio.readReg(0x11); // PA level register
      Serial.print(" [PA=0x");
      Serial.print(pa, HEX);
      Serial.print("]");
    }

    delay(100);
    blink(5, 30); // flash to show ACK
  }
  else
  {
    if (debug)
      Serial.print(" nothing...");
  }
  radio.sleep(); // send radio to sleep to save power
  delay(TRANSMITPERIOD);
  if (debug)
    Serial.println();
}

