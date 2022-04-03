/*

  HC-SR04

  Note: this sketch uses the 3.3V "CS100" version
  
*/

// Constants
const float speed_sound = 337.31; // see https://en.wikipedia.org/wiki/Speed_of_sound#Tables

// Pins
const int TRIG_PIN = 7; // PIN_PB0
const int ECHO_PIN = 6; // PIN_PB1

// Anything over 400 cm (23200 us pulse) is "out of range"
const unsigned int MAX_DIST = 23200;

void setup() {

  // The Trigger pin will tell the sensor to range find
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  //Set Echo pin as input to measure the duration of 
  //pulses coming back from the distance sensor
  pinMode(ECHO_PIN, INPUT);

  // We'll use the serial monitor to view the sensor output
  Serial.begin(9600);
}

void loop() {

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
    Serial.println("Out of range");
  } else {
    Serial.print(cm);
    Serial.println(" cm");
  }

  // Wait at least 60ms before next measurement
  delay(60);
}