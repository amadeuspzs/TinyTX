#ifndef CONFIG_H
#define CONFIG_H

#include <RFM69.h>
#include <RFM69_ATC.h>

// Debug serial (Hardware Serial) uses the default PB2 pin on ATtiny1614
#define SERIAL_BAUD 9600

// Radio Configuration
#define NODEID      3         // ID of this device
#define NETWORKID   1         // Your network ID
#define GATEWAYID   1         // ID of your gateway
#define FREQUENCY   RF69_433MHZ
#define IS_RFM69HW_HCW        // Uncomment only for RFM69HW/HCW!
#define ENABLE_ATC        // Uncomment to enable Auto Transmission Control
#define ATC_RSSI    -75       // Target RSSI (dBm)

// Pin Configuration
#define TRIG_PIN    PIN_PA5   // Pin 1 (PA5) to trigger sensor measurement (active low pulse)
#define RX_PIN      PIN_PA7   // Pin 3 (PA7) as SoftwareSerial RX to receive 6-byte frames
#define DUMMY_TX_PIN PIN_PB1  // Pin 6 (PB1) as unused/dummy SoftwareSerial TX pin
#define POWER_PIN   PIN_PB3   // Pin 4 (PB3) used to toggle the mosfet on/off

// Timing
#define TRANSMITPERIOD 5000   // Send data every 5 seconds (5000 ms)

// Radio Class Selection
#ifdef ENABLE_ATC
extern RFM69_ATC radio;
#else
extern RFM69 radio;
#endif

// Payload structure transmitted over RFM69
typedef struct {
  float distance_cm;
  float temperature_c;
} Payload;

#endif // CONFIG_H
