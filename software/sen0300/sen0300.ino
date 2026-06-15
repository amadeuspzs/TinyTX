/*
  SEN0300 Integration
  
  Reads distance and temperature from DFRobot SEN0300 Waterproof Ultrasonic Sensor
  and transmits data via RFM69.

  Wiring Connections:
  ------------------------------------
  Sensor Pin   | TinyTX Pin (Arduino Pin)
  ------------------------------------
  VCC          | VCC
  GND          | GND
  RX (Trigger) | Pin 1 (PA5)
  TX (Data)    | Pin 3 (PA7)
  ------------------------------------

  Pin 5 (PB2) is used for Hardware Serial TX (Debug Output).
  Pin 6 (PB1) is configured as the dummy SoftwareSerial TX pin.

  Frequency is set exactly to 433.0MHz for compatibility.
*/

#include <SoftwareSerial.h>
#include <SPI.h>
#include "config.h"

// Instantiate the radio object
#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif

// SoftwareSerial instance for reading from the sensor (TX pin is dummy and left unconnected)
SoftwareSerial sensorSerial(RX_PIN, DUMMY_TX_PIN);

int numRetries = 3;
int timeout = 50;

Payload theData;

// Function declarations
bool readSensorData(float &distance_cm, float &temperature_c);

void setup()
{
  // 1. Lock the pin HIGH immediately to prevent Switch Mode
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, HIGH);

  // Define the power pin
  pinMode(POWER_PIN, OUTPUT);
  // for now, just switch on (reverse logic)
  digitalWrite(POWER_PIN, LOW);
  delay(2000);
  
  // Initialize hardware Serial (defaults to PB2 on ATtiny1614)
  Serial.begin(SERIAL_BAUD);
  delay(100);
  Serial.println("\n\n=== TinyTX SEN0300 Starting ===");

  // Initialize SoftwareSerial for the sensor
  sensorSerial.begin(9600);

  // Initialize the radio
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  
  // Set frequency to exactly 433.0MHz
  radio.setFrequency(433000000);

#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); // Must include for RFM69HW/HCW
#endif

#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  // Verify radio configuration by reading the frequency register
  uint32_t frf = ((uint32_t)radio.readReg(0x07) << 16) |
                 ((uint32_t)radio.readReg(0x08) << 8) |
                 radio.readReg(0x09);
  float freq_mhz = (frf * 61.03515625) / 1000000.0;
  Serial.print("Radio actual frequency: ");
  Serial.print(freq_mhz, 2);
  Serial.println(" MHz");
  
  Serial.println("Setup completed successfully. Starting measurement loop...\n");
}

void loop()
{
  Serial.println("--- Triggering Sensor Measurement ---");
  
  float dist = 0.0;
  float temp = 0.0;

  if (readSensorData(dist, temp))
  {
    theData.distance_cm = dist;
    theData.temperature_c = temp;

    Serial.print("Parsed values: Distance = ");
    Serial.print(theData.distance_cm, 1);
    Serial.print(" cm, Temperature = ");
    Serial.print(theData.temperature_c, 1);
    Serial.println(" C");

    Serial.print("Sending payload (");
    Serial.print(sizeof(theData));
    Serial.print(" bytes) via RFM69... ");

    if (radio.sendWithRetry(GATEWAYID, (const void *)(&theData), sizeof(theData), numRetries, timeout))
    {
      Serial.println("ACK received!");
    }
    else
    {
      Serial.println("no response.");
    }
  }
  else
  {
    Serial.println("Failed to read sensor data.");
  }

  radio.sleep(); // Put radio to sleep to save power
  
  Serial.print("Sleeping for ");
  Serial.print(TRANSMITPERIOD / 1000);
  Serial.println(" seconds...\n");
  
  delay(TRANSMITPERIOD);
}

// Read and parse data from the SEN0300 sensor
bool readSensorData(float &distance_cm, float &temperature_c)
{
  // Flush input buffer to clear stale serial data
  while (sensorSerial.available())
  {
    sensorSerial.read();
  }

  // Trigger the measurement with a LOW pulse
  digitalWrite(TRIG_PIN, LOW);
  delay(10); // try 10ms as per datasheet
  //delayMicroseconds(500); // 0.5ms is well within the 0.1-10ms spec
  digitalWrite(TRIG_PIN, HIGH);

  unsigned char buffer_RTT[6] = {0};
  int bytesRead = 0;
  unsigned long startTime = millis();
  bool foundHeader = false;

  // 1. Align on header byte 0xFF
  while (millis() - startTime < 300) // 300ms timeout
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
    Serial.println("  [Error] Timeout: Header byte (0xFF) not found.");
    return false;
  }

  // 2. Read the remaining 5 bytes of the frame
  bytesRead = 1;
  while (bytesRead < 6 && (millis() - startTime < 500))
  {
    if (sensorSerial.available() > 0)
    {
      buffer_RTT[bytesRead++] = sensorSerial.read();
    }
  }

  if (bytesRead < 6)
  {
    Serial.print("  [Error] Timeout: Incomplete frame (received ");
    Serial.print(bytesRead);
    Serial.println(" of 6 bytes).");
    return false;
  }

  // Print raw hex buffer for debugging
  Serial.print("  [Raw RX] ");
  for (int i = 0; i < 6; i++)
  {
    Serial.print("0x");
    if (buffer_RTT[i] < 0x10) Serial.print("0");
    Serial.print(buffer_RTT[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // 3. Verify checksum
  // SUM = (Header + DATA_H + DATA_L + Temp_H + Temp_L) & 0xFF
  byte expectedChecksum = (buffer_RTT[0] + buffer_RTT[1] + buffer_RTT[2] + buffer_RTT[3] + buffer_RTT[4]) & 0xFF;
  if (buffer_RTT[5] != expectedChecksum)
  {
    Serial.print("  [Error] Checksum mismatch. Expected: 0x");
    Serial.print(expectedChecksum, HEX);
    Serial.print(", Got: 0x");
    Serial.println(buffer_RTT[5], HEX);
    return false;
  }

  // 4. Parse distance (bytes 1 and 2)
  int range_mm = (buffer_RTT[1] << 8) | buffer_RTT[2];

  // 5. Parse temperature (bytes 3 and 4)
  // Byte 3 MSB is the sign bit (1 = negative, 0 = positive)
  bool isNegative = (buffer_RTT[3] & 0x80) == 0x80;
  int tempVal = ((buffer_RTT[3] & 0x7F) << 8) | buffer_RTT[4];
  float temp = tempVal / 10.0;
  if (isNegative)
  {
    temp = -temp;
  }

  distance_cm = range_mm / 10.0;
  temperature_c = temp;

  return true;
}
