// go to sleep with lowest power mode, for profiling
// burn bootloader with brown out detection disabled
// set clock to 1MHz

#include <avr/sleep.h>
#include <util/delay.h>
#include <RFM69.h>      // https://github.com/lowpowerlab/rfm69
#include <SPI.h>

#define NODEID      99   // id of this device
#define NETWORKID   1   // your network id
#define GATEWAYID   1   // id of your gateway
#define FREQUENCY     RF69_433MHZ
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW! See https://lowpowerlab.com/guide/moteino/transceivers/

RFM69 radio;

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
  RTC.PITINTFLAGS = RTC_PI_bm;          // clear interrupt flag by writing '1' (required)
}

void setup() {

//   define unused pins as PULLUP to save power
  pinMode(PIN_PA0, INPUT_PULLUP);
  //pinMode(PIN_PA1, INPUT_PULLUP); // MOSI
  //pinMode(PIN_PA2, INPUT_PULLUP); // MISO
  //pinMode(PIN_PA3, INPUT_PULLUP); // SCK
  //pinMode(PIN_PA4, INPUT_PULLUP); // NSS
  pinMode(PIN_PA5, INPUT_PULLUP);
  //pinMode(PIN_PA6, INPUT_PULLUP); // DIO0
  pinMode(PIN_PA7, INPUT_PULLUP);
  pinMode(PIN_PB0, INPUT_PULLUP);
  pinMode(PIN_PB1, INPUT_PULLUP);
  pinMode(PIN_PB2, INPUT_PULLUP);
  pinMode(PIN_PB3, INPUT_PULLUP);

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  #ifdef IS_RFM69HW_HCW
    radio.setHighPower(); //must include this only for RFM69HW/HCW!
  #endif

  radio.sleep();  // sleep radio

  RTC_init();                           // initialise the RTC timer
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // set sleep mode to POWER DOWN mode
  sleep_enable();                       // enable sleep mode, but not going to sleep yet
  sei();                                // enable interrupts
}

void loop() {
  _delay_ms(1000); // 1000ms delay for power measurements
  sleep_cpu();
}
