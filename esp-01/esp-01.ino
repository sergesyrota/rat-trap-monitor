#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>

#include "config.h"
#include "secret.h"

const int sda_pin = 0;
const int scl_pin = 3;

#include <Wire.h>
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads;

// Values we store in EEPROM
TrapStats romStats;

// For debugging purposes, to see if we read EEPROM correctly; I = version mismatch, init to 0s; 
char eepromRead = 'N';

void readStats() {
  // Check to make sure stats values are real, by looking at first byte
  EEPROM.get(EEPROM_ADDRESS, romStats);
  if (romStats.checkVersion != STATS_VERSION) {
    eepromRead = 'I';
    romStats.checkVersion = STATS_VERSION;
    romStats.shockCounter = 0;
    romStats.caughtCounter = 0;
    romStats.lastState = TRAP_IDLE;
    saveStats();
  } else {
    eepromRead = 'Y';
  }
}

void saveStats()
{
  EEPROM.put(EEPROM_ADDRESS, romStats);
  EEPROM.commit();
}

void setup() {
  ////Serial.begin(115200);

  // prepare LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);
  digitalWrite(1, LOW);
  pinMode(sda_pin, OUTPUT);
  pinMode(scl_pin, OUTPUT);

  // Reconfigure I2C pins to what we have on ESP-01
  Wire.begin(0,3);
  
  ads.begin();
  ads.setGain(GAIN_ONE);

  EEPROM.begin(sizeof(TrapStats));
  readStats();

  connectWifi();
}

void loop() {
  // wait for WiFi connection
  unsigned long sleepDuration = PING_INTERVAL;
  if ((WiFi.status() == WL_CONNECTED)) {
    WiFiClient client;
    HTTPClient http;
    
    http.begin(client, SERVER_ENDPOINT); //HTTP
    http.addHeader("Content-Type", "application/json");
    int v[2];
    getVoltage(v);
    char buf[300];
    struct sens sensReading;
    sensReading = caughtSensorReading();

    // Interpret measurements, and update permanent memory if needed
    interpretTrapState(sensReading);

    sprintf(buf, "{\"deviceId\": \"%s\", \"batteryV\": %d.%d, \"caught\": %s, \"onPercent\": %d, \"ADC\": %lu, \"millisOn\": %lu, \"millisOff\": %lu, \"measureMs\": %lu, \"counter\": {\"shock\": %lu, \"caught\": %lu}}", 
      DEVICE_ID, 
      v[0], 
      v[1], 
      (romStats.lastState == TRAP_CAUGHT ? "true" : "false"), // at this point, "last" is already current
      sensReading.onPercent,
      ads.readADC_SingleEnded(SENSOR_PIN),
      sensReading.millisOn,
      sensReading.millisOff,
      sensReading.measureDuration,
      romStats.shockCounter,
      romStats.caughtCounter
    );
    int httpCode = http.POST(buf);
    http.end();
    // If server didn't respond with a 200 code, retry in a minute, rather than normal interval
    if (httpCode != 200) {
      sleepDuration = 60;
    }
  } else {
    // If WiFi failed to connect, retry in a minute, rather than normal interval
    sleepDuration = 60;
  }
#ifdef DEEP_SLEEP_ENABLED
  ESP.deepSleep(sleepDuration*1e6);
#else
  delay(sleepDuration*1000); // When not in power save mode, just sleep
  ESP.restart();
#endif
}

void connectWifi() {
  //  Force the ESP into client-only mode
  WiFi.mode(WIFI_STA); 
  
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  //  Enable light sleep
  //wifi_set_sleep_type(LIGHT_SLEEP_T);
}

// For ease of conversion to string later, we need integer volts separately from integer millivolts
// buf[0] is volts, buf[1] is millivolts (without the volts)
void getVoltage(int *buf) {
  int reading = ads.readADC_SingleEnded(BATTERY_PIN);
  float volts = reading * BATTERY_CONVERSION_FACTOR;
  buf[0] = (int)floor(volts);
  int mV = (volts - buf[0]) * 1000;
  buf[1] = mV;
}

void interpretTrapState(struct sens sensReading) {
    bool stateChanged = false;
    if (sensReading.onPercent >= CAUGHT_DUTY_CYCLE_THRESHOLD) {
      currentState = TRAP_CAUGHT;
      if (romStats.lastState != currentState) {
        romStats.caughtCounter++;
        stateChanged = true;
      }
    } else if (sensReading.millisOn == 0 and sensReading.millisOff == 1) {
      // Special state when it's 100% on, but so that we don't trigger "caught" interpretation
      // That means it's actively shocking now (or the trap is off by the switch)
      currentState = TRAP_SHOCKING;
      if (romStats.lastState != currentState) {
        romStats.shockCounter++;
        stateChanged = true;
      }
    } else {
      currentState = TRAP_IDLE;
      // If the trap was cleaned after catching something - reset the shock counter
      if (romStats.lastState == TRAP_CAUGHT) {
        romStats.shockCounter=0;
      }
    }
    if (romStats.lastState != currentState) {
      stateChanged = true;
      romStats.lastState = currentState;
    }
    // Track state changes, so we don't have to write to EEPROM every time; there is limited number of writes on some boards
    if (stateChanged) {
      saveStats();
    }
}

struct sens caughtSensorReading() {
  unsigned long startTs = millis();
  unsigned long lowStart = 0;
  unsigned long lowEnd = 0;
  unsigned long highStart = 0;
  unsigned long highEnd = 0;
  // 5 second limit, but we'll end when we find both edges
  int previous = -1;
  while ((millis() - startTs) < 5000) {
    int current = (ads.readADC_SingleEnded(SENSOR_PIN) >= SENSOR_CAUGHT_THRESHOLD ? 1 : 0);
    if (previous == -1) {
      previous = current;
    }
    if (current != previous) {
      if (current == false) {
        if (highStart != 0) {
          highEnd = millis();
        }
        if (lowStart == 0) {
          lowStart = millis();
        } else {
          // We shold now have a full range
          break;
        }
      } else {
        if (lowStart != 0) {
          lowEnd = millis();
        }
        if (highStart == 0) {
          highStart = millis();
        } else {
          // We shold now have a full range
          break;
        }
      }
      previous = current;
    }
    delay(10);
  }
  struct sens out;
  out.measureDuration = (millis() - startTs);
  out.millisOn = highEnd - highStart;
  out.millisOff = lowEnd - lowStart;
  if (lowStart == 0 && highStart == 0) {
    if (previous == 1) {
      // Still consider this as off, as permenent on means either grilling now, or the trap is off completely.
      out.millisOn = 0;
      out.millisOff = 1;
    } else {
      out.millisOn = 0;
      out.millisOff = 2;
    }
  } else if (lowStart == 0) {
    // There is transition to on state, but not transition to off; So all on.
    out.millisOn = 0;
    out.millisOff = 3;
  } else if (highStart == 0) {
    // The opposite from above
    out.millisOn = 4;
    out.millisOff = 0;
  } else {
    if (lowEnd == 0) {
      lowEnd = millis() + 3000;
    }
    if (highEnd == 0) {
      highEnd = millis() + 3000;
    }
  }
  out.onPercent = (int)(100.0*((float)out.millisOn/(out.millisOn+out.millisOff)));
  return out;
}
