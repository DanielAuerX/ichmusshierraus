#include <Wire.h>
#include "imhr_wifi.h"
#include "imhr_display.h"
#include "imhr_diparture.h"

const uint32_t wifiRetryDelay = 600000;      // 10 minutes
const uint32_t readableMessageDelay = 2000;  // 2 seconds
const uint32_t shortPollingInterval = 30000; // 30 seconds

void setup()
{
  Serial.begin(115200);
  imhr::displayInit();
  configTime(3600, 3600, "pool.ntp.org"); // UTC+1, +1 DST
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
  imhr::BusDeparture bus = imhr::fetchBusDeparture();
  imhr::TrainDeparture train = imhr::fetchTrainDeparture();

  imhr::displayDeparture(bus.line.c_str(), bus.minutes, train.line.c_str(), train.minutes);
  Serial.println("----------");

  delay(shortPollingInterval);
}
