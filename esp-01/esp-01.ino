#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "config.h"
#include "secret.h"

const int sda_pin = 0;
const int scl_pin = 3;

#include <Wire.h>
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads;


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
    char buf[200];
    struct sens sensReading;
    sensReading = caughtSensorReading();
    sprintf(buf, "{\"deviceId\": \"%s\", \"batteryV\": %d.%d, \"caught\": %s, \"onPercent\": %d, \"ADC\": %lu, \"millisOn\": %lu, \"millisOff\": %lu, \"measureMs\": %lu}", 
      DEVICE_ID, 
      v[0], 
      v[1], 
      (sensReading.onPercent >= CAUGHT_DUTY_CYCLE_THRESHOLD ? "true" : "false"),
      sensReading.onPercent,
      ads.readADC_SingleEnded(SENSOR_PIN),
      sensReading.millisOn,
      sensReading.millisOff,
      sensReading.measureDuration
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
