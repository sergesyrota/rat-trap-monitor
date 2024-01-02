#define DEVICE_ID "Kappa"
// Comment out to disable
#define DEEP_SLEEP_ENABLED

// How long to sleep between signals to the server
#define PING_INTERVAL 240

#define BATTERY_PIN 3
#define SENSOR_PIN 0

// 0.00985 and 5 for ADS 1015
// 0.000607 and 20 for ADS 1115
// Sensor type is set at the beginning of the code
#define BATTERY_CONVERSION_FACTOR 0.000623
#define SENSOR_CAUGHT_THRESHOLD 1500UL
// what % of time trap LED should be ON to consider as a "caught" signal?
#define CAUGHT_DUTY_CYCLE_THRESHOLD 30

struct sens {
  unsigned long millisOn;
  unsigned long millisOff;
  unsigned long measureDuration;
  int onPercent;
};

enum TrapState {
  TRAP_IDLE = 0,
  TRAP_SHOCKING = 1,
  TRAP_CAUGHT = 2
};

#define EEPROM_ADDRESS 0
#define STATS_VERSION 'A'

struct TrapStats {
  char checkVersion; // This is to be able to init to 0 when we're first time powering up; 1 byte symbol + 1 byte separator
  unsigned long shockCounter;
  unsigned long caughtCounter;
  TrapState lastState;
};
