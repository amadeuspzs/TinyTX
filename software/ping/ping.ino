/*

  Test sketch to get your TinyTX up and running.

  You will need a receiver (gateway) set up to receive packets.

  e.g. https://github.com/amadeuspzs/rfm69-pi-avr/tree/master/hub-pi
  
  Packets will be toggled between 0 and 1 every 5 seconds, providing a ping

  LED will blink once for transmit, and flash for received ACK

*/

#include <RFM69.h>      // https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>  // https://github.com/lowpowerlab/rfm69
#include <SPI.h>

#define debug false     // enable Serial output  
#define led 7           // positive lead of LED
#define NODEID      2   // id of this device
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

int TRANSMITPERIOD = 5000; //transmit a packet to gateway so often (in ms)

typedef struct {
  bool state;
} Payload;
Payload theData;

void setup() {
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

  pinMode(led, OUTPUT);   // blinker
  digitalWrite(led, LOW); // turn off
}

void blink(int times = 1, int interval = 100) {
  for (int n=0; n < times; n++) {
    digitalWrite(led, HIGH);
    delay(interval);
    digitalWrite(led, LOW);
    delay(interval);
  }
}


void loop() {
  // toggle state
  theData.state = (theData.state == 0) ? 1 : 0;
  
  if (debug) Serial.print("Sending struct (");
  if (debug) Serial.print(sizeof(theData));
  if (debug) Serial.print(" bytes): ");
  if (debug) Serial.print(theData.state);
  if (debug) Serial.print(" ... ");

  blink(); // blink to trasmit
  
  if (radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData), numRetries, timeout)) {
    if (debug) Serial.print(" ok!");
    delay(100);
    blink(5, 30); // flash to show ACK
  } else { 
    if (debug) Serial.print(" nothing...");
  }
  delay(TRANSMITPERIOD);
  if (debug) Serial.println();
}
