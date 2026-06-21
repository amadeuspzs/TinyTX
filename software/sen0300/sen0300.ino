/*
  SEN0300 Integration

  Reads distance and temperature from DFRobot SEN0300 Waterproof Ultrasonic Sensor
  and transmits data via RFM69.

  Wiring Connections:
  TinyTX Header L-R:
  7 6 5 4 1 3 3V3 GND

  ------------------------------------
  Sensor Pin   | TinyTX Pin (Arduino Pin)
  ------------------------------------
  VCC          | VCC
  GND          | GND
  RX (Trigger) | Pin 1 (PA5)
  TX (Data)    | Pin 3 (PA7)
  ------------------------------------

  Pin 4 (PB3) is the MOSFET gate to power the sensor on/off (active LOW).
  Pin 5 (PB2) is Hardware Serial TX — debug output when DEBUG is defined in config.h.
  Pin 6 (PB1) is the dummy SoftwareSerial TX (sensor serial never transmits).

  Sleep:
    RTC PIT wakes the MCU every ~1 s; TARGET_SLEEPS ticks pass before the
    next measurement, giving a variable transmit interval without busy-delay.
*/

#include <avr/sleep.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include "config.h"
#include <math.h>

// ---------------------------------------------------------------------------
// Radio
// ---------------------------------------------------------------------------
#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif

// ---------------------------------------------------------------------------
// SoftwareSerial for sensor receive (TX pin is dummy, never driven)
// ---------------------------------------------------------------------------
SoftwareSerial sensorSerial(RX_PIN, DUMMY_TX_PIN);

// ---------------------------------------------------------------------------
// Radio retries
// ---------------------------------------------------------------------------
int numRetries = 3;
int timeout = 50;      // ms to wait for ACK
bool heartbeat = true; // binary heartbeat every sleep cycle

// ---------------------------------------------------------------------------
// Payload
// ---------------------------------------------------------------------------
Payload theData;

// ---------------------------------------------------------------------------
// Sleep / RTC state
// ---------------------------------------------------------------------------
volatile int current_sleeps = 0; // incremented by the PIT ISR

// ---------------------------------------------------------------------------
// RTC / PIT initialisation
// One PIT interrupt every ~1 s (CYC1024 @ 1.024 kHz internal oscillator)
// ---------------------------------------------------------------------------
void RTC_init(void)
{
  RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // 1.024 kHz ULP oscillator
  while (RTC.STATUS > 0 || RTC.PITSTATUS)
    ;                                  // wait for sync
  RTC.PITINTCTRL = RTC_PI_bm;          // enable PIT interrupt
  RTC.PITCTRLA = RTC_PERIOD_CYC1024_gc // ~1 s per tick
                 | RTC_PITEN_bm;       // enable PIT
}

ISR(RTC_PIT_vect)
{
  current_sleeps++;
  RTC.PITINTFLAGS = RTC_PI_bm; // clear flag (write 1 to clear)
}

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
bool readSensorData(float &distance_cm, float &temperature_c);

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
  // Pull unused pins high to minimise leakage current
  pinMode(PIN_PB0, INPUT_PULLUP);

  // 1. Hold TRIG_PIN HIGH immediately to prevent sensor entering Switch Mode
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, HIGH);

  // 2. Power off the sensor via MOSFET (active LOW gate logic)
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH); // power off sensor

  // 3. Debug serial (hardware Serial on PB2)
  DBG_BEGIN(SERIAL_BAUD);
  delay(100);
  DBG_PRINTLN("\n\n=== TinyTX SEN0300 Starting ===");

  // 4. Sensor serial (SoftwareSerial on PA7)
  sensorSerial.begin(9600);

  // 5. Radio
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  radio.setFrequency(433000000); // lock to exactly 433.0 MHz

#ifdef IS_RFM69HW_HCW
  radio.setHighPower();
#endif
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

#ifdef DEBUG
  // 6. Verify radio frequency via register readback
  uint32_t frf = ((uint32_t)radio.readReg(0x07) << 16) |
                 ((uint32_t)radio.readReg(0x08) << 8) |
                 radio.readReg(0x09);
  float freq_mhz = (frf * 61.03515625f) / 1000000.0f;
#endif
  DBG_PRINT("Radio frequency: ");
  DBG_PRINT2(freq_mhz, 2);
  DBG_PRINTLN(" MHz");

  DBG_PRINTLN("Setup complete. Starting loop...\n");

  // 7. RTC PIT sleep setup
  RTC_init();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sei();
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
  // Fire on first wake (current_sleeps == 0) or once TARGET_SLEEPS ticks have passed
  if (current_sleeps == 0 || current_sleeps >= TARGET_SLEEPS)
  {
    DBG_PRINTLN("--- Triggering Sensor Measurement ---");
    digitalWrite(POWER_PIN, LOW); // power on sensor
    delay(100);                   // wait for sensor to power up and stabilise
    float dist = 0.0f;
    float temp = 0.0f;

    theData.heartbeat = heartbeat;

    if (readSensorData(dist, temp))
    {
      theData.distance_cm = dist;
      theData.temperature_c = temp;

      DBG_PRINT("Distance = ");
      DBG_PRINT2(theData.distance_cm, 1);
      DBG_PRINT(" cm, Temperature = ");
      DBG_PRINT2(theData.temperature_c, 1);
      DBG_PRINTLN(" C");
    }
    else
    {
      DBG_PRINTLN("Failed to read sensor data.");
      theData.distance_cm = NAN; // indicate invalid reading
      theData.temperature_c = NAN;
    }

    DBG_PRINT("Sending via RFM69... ");

    if (radio.sendWithRetry(GATEWAYID, (const void *)(&theData), sizeof(theData), numRetries, timeout))
    {
      DBG_PRINTLN("ACK received!");
    }
    else
    {
      DBG_PRINTLN("no response.");
    }
    current_sleeps = 0; // reset sleep counter

    DBG_PRINT("Sleeping for ~");
    DBG_PRINT(TARGET_SLEEPS);
    DBG_PRINTLN(" seconds...\n");
#ifdef DEBUG
    Serial.flush();
#endif
  }

  heartbeat = !heartbeat;        // toggle heartbeat for next sleep cycle
  digitalWrite(POWER_PIN, HIGH); // power off sensor before sleeping
  radio.sleep();                 // put radio to sleep before CPU sleeps
  sleep_cpu();                   // sleep until next PIT interrupt (~1 s), then loop() reruns
}

// ---------------------------------------------------------------------------
// readSensorData()
// Triggers the SEN0300, reads and validates the 6-byte frame, returns
// distance (cm) and temperature (°C).
// ---------------------------------------------------------------------------
bool readSensorData(float &distance_cm, float &temperature_c)
{
  // Flush stale bytes
  while (sensorSerial.available())
    sensorSerial.read();

  // Active-LOW trigger pulse (10 ms, within the 0.1–10 ms spec)
  digitalWrite(TRIG_PIN, LOW);
  delay(10);
  digitalWrite(TRIG_PIN, HIGH);

  unsigned char buffer_RTT[6] = {0};
  int bytesRead = 0;
  unsigned long startTime = millis();
  bool foundHeader = false;

  // 1. Align on header byte 0xFF
  while (millis() - startTime < 300)
  {
    if (sensorSerial.available() > 0)
    {
      byte b = sensorSerial.read();
      if (b == 0xFF)
      {
        buffer_RTT[0] = 0xFF;
        foundHeader = true;
        break;
      }
    }
  }

  if (!foundHeader)
  {
    DBG_PRINTLN("  [Error] Timeout: Header byte (0xFF) not found.");
    return false;
  }

  // 2. Read remaining 5 bytes
  bytesRead = 1;
  while (bytesRead < 6 && (millis() - startTime < 500))
  {
    if (sensorSerial.available() > 0)
      buffer_RTT[bytesRead++] = sensorSerial.read();
  }

  if (bytesRead < 6)
  {
    DBG_PRINT("  [Error] Incomplete frame (");
    DBG_PRINT(bytesRead);
    DBG_PRINTLN("/6 bytes).");
    return false;
  }

  // 3. Print raw frame for debugging
  DBG_PRINT("  [Raw RX] ");
  for (int i = 0; i < 6; i++)
  {
    DBG_PRINT("0x");
    if (buffer_RTT[i] < 0x10)
      DBG_PRINT("0");
    DBG_PRINT2(buffer_RTT[i], HEX);
    DBG_PRINT(" ");
  }
  DBG_PRINTLN("");

  // 4. Verify checksum: SUM = (byte0..4) & 0xFF
  byte expected = (buffer_RTT[0] + buffer_RTT[1] + buffer_RTT[2] +
                   buffer_RTT[3] + buffer_RTT[4]) &
                  0xFF;
  if (buffer_RTT[5] != expected)
  {
    DBG_PRINT("  [Error] Checksum mismatch. Expected 0x");
    DBG_PRINT2(expected, HEX);
    DBG_PRINT(", got 0x");
    DBG_PRINT2(buffer_RTT[5], HEX);
    DBG_PRINTLN("");
    return false;
  }

  // 5. Parse distance (bytes 1–2, in mm)
  int range_mm = (buffer_RTT[1] << 8) | buffer_RTT[2];

  // 6. Parse temperature (bytes 3–4; MSB of byte 3 = sign bit)
  bool isNegative = (buffer_RTT[3] & 0x80) != 0;
  int tempVal = ((buffer_RTT[3] & 0x7F) << 8) | buffer_RTT[4];
  float temp = tempVal / 10.0f;
  if (isNegative)
    temp = -temp;

  distance_cm = range_mm / 10.0f;
  temperature_c = temp;

  return true;
}