#ifndef CONFIG_H
#define CONFIG_H

#include <RFM69.h>
#include <RFM69_ATC.h>

// ---------------------------------------------------------------------------
// Debug output via Hardware Serial (TX on PB2, ATtiny1614 default)
// Comment out #define DEBUG to disable all debug output for production
// (saves flash, RAM, and avoids Serial init overhead)
// ---------------------------------------------------------------------------
// #define DEBUG

#ifdef DEBUG
#define DBG_BEGIN(baud) Serial.begin(baud)
#define DBG_PRINT(x) Serial.print(x)
#define DBG_PRINTLN(x) Serial.println(x)
#define DBG_PRINT2(x, f) Serial.print(x, f)
#else
#define DBG_BEGIN(baud)
#define DBG_PRINT(x)
#define DBG_PRINTLN(x)
#define DBG_PRINT2(x, f)
#endif

#define SERIAL_BAUD 9600

// ---------------------------------------------------------------------------
// Radio Configuration
// ---------------------------------------------------------------------------
#define NODEID 3    // ID of this device
#define NETWORKID 1 // Your network ID
#define GATEWAYID 1 // ID of your gateway
#define FREQUENCY RF69_433MHZ
#define IS_RFM69HW_HCW // Uncomment only for RFM69HW/HCW!
#define ENABLE_ATC     // Comment out to disable Auto Transmission Control
#define ATC_RSSI -75   // Target RSSI (dBm)

// ---------------------------------------------------------------------------
// Pin Configuration
// ---------------------------------------------------------------------------
#define TRIG_PIN PIN_PA5     // Pin 1 (PA5) trigger sensor measurement (active low pulse)
#define RX_PIN PIN_PA7       // Pin 3 (PA7) SoftwareSerial RX from sensor
#define DUMMY_TX_PIN PIN_PB1 // Pin 6 (PB1) unused/dummy SoftwareSerial TX
#define POWER_PIN PIN_PB3    // Pin 4 (PB3) MOSFET gate to power sensor on/off
// Note: PB2 is Hardware Serial TX (debug output when DEBUG is defined)

// ---------------------------------------------------------------------------
// Sleep / Timing Configuration
//
// The RTC PIT fires every ~1 second (RTC_PERIOD_CYC1024_gc @ 1.024 kHz).
// Total sleep time ≈ TARGET_SLEEPS seconds.
// Set to 5 for bench testing; change to e.g. 300 for ~5 min in production.
// ---------------------------------------------------------------------------
#define TARGET_SLEEPS 1800 // 30 mins

// ---------------------------------------------------------------------------
// Radio Class Selection
// ---------------------------------------------------------------------------
#ifdef ENABLE_ATC
extern RFM69_ATC radio;
#else
extern RFM69 radio;
#endif

// ---------------------------------------------------------------------------
// Payload structure transmitted over RFM69
// ---------------------------------------------------------------------------
typedef struct
{
  float distance_cm;
  float temperature_c;
  bool heartbeat;
} Payload;

#endif // CONFIG_H