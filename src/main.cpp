#include <Wire.h>
#include "imhr_wifi.h"
#include "imhr_display.h"
#include "imhr_diparture.h"
#include "esp_sleep.h"

const uint32_t wifiRetryDelay = 600000;      // 10 minutes
const uint32_t sleepRetryDelay = 600000;     // 10 minutes
const uint32_t readableMessageDelay = 2000;  // 2 seconds
const uint32_t shortPollingInterval = 30000; // 30 seconds
const int sleepStartHour = 0;
const int sleepStartMin = 30;
const int sleepEndHour = 5;
const int sleepEndMin = 0;
int counter = 0;

static bool isNightTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return false;

  int nowMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  int startMinutes = sleepStartHour * 60 + sleepStartMin;
  int endMinutes = sleepEndHour * 60 + sleepEndMin;

  return nowMinutes >= startMinutes && nowMinutes < endMinutes;
}

void setup()
{
  Serial.begin(115200);
  configTime(3600, 3600, "pool.ntp.org"); // UTC+1, +1 DST
  imhr::displayInit();
  while (!imhr::connectWiFi())
  {
    imhr::displayMessage("!", "wifi problem");
    delay(wifiRetryDelay);
  }
  imhr::displayMessage(":)", "wifi connected!");
  delay(readableMessageDelay);
}

void loop()
{
  if (isNightTime())
  {
    counter++;
    Serial.println("still sleeping... zZzz... retry counter: " + String(counter));
    imhr::displaySleep();
    // it can cause issues with the wifi connection, when waking up...
    // esp_sleep_enable_timer_wakeup(5 * 60 * 1000000ULL);
    // esp_light_sleep_start();
    delay(30000);
    return;
  }
  counter = 0;
  imhr::displayWake();
  imhr::BusDeparture bus = imhr::fetchBusDeparture();
  imhr::TrainDeparture train = imhr::fetchTrainDeparture();

  imhr::displayDeparture(bus.line.c_str(), bus.minutes, train.line.c_str(), train.minutes);
  Serial.println("----------");

  delay(shortPollingInterval);
}
