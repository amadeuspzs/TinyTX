// Constants
const float speed_sound = 337.31; // @10degC see https://en.wikipedia.org/wiki/Speed_of_sound#Tables

// Pins
#define TRIG_PIN    1 // yellow aka PA5
#define ECHO_PIN    3 // blue aka PA7
#define POWER_PIN   4 // PB3

// Anything over 400 cm (23200 us pulse) is "out of range"
#define MAX_DIST 400

// Radio config
#define NODEID      3   // id of this device
#define NETWORKID   1   // your network id
#define GATEWAYID   1   // id of your gateway
#define FREQUENCY     RF69_433MHZ
//#define ENCRYPTKEY    "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW! See https://lowpowerlab.com/guide/moteino/transceivers/
// #define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
// #define ATC_RSSI      -80 // target RSSI (dBm)

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

int numRetries = 3; // default is 2
int timeout = 50; // ms to wait for ACK, default is 30

volatile int current_sleeps = 0;        // counter for how many times PIT has woken up
int target_sleeps = 19;                 // target for total sleep time. Increment of the RTC_PERIOD in RTC_init()
                                        // e.g. RTC_PERIOD_CYC32768_gc = 32s, target_sleeps = 19 -> ~10 mins sleep
unsigned long echoTime;
float cm;

typedef struct {
  float cm;
} Payload;
Payload theData;

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DIST); // NewPing setup of pins and maximum distance.
