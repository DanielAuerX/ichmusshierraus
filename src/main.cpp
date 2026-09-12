#include <Wire.h>
#include "imhr_wifi.h"
#include "imhr_display.h"
#include "imhr_announcement.h"
#include "imhr_departure.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

const uint32_t wifiRetryDelay = 600000;      // 10 minutes
const uint32_t sleepRetryDelay = 600000;     // 10 minutes
const uint32_t readableMessageDelay = 2000;  // 2 seconds
const uint32_t shortPollingInterval = 30000; // 30 seconds
const uint32_t announcementPollingInterval = shortPollingInterval * 20; // 10 minutes
const int sleepStartHour = 0;
const int sleepStartMin = 30;
const int sleepEndHour = 5;
const int sleepEndMin = 0;
int counter = 0;

static SemaphoreHandle_t dataMutex;

static imhr::BusDeparture sharedBus = {"Bus", -1};
static imhr::TrainDeparture sharedTrain = {"Zug", -1};
static imhr::Announcement sharedAnnouncement = {false, ""};
static bool departureDirty = false;
static bool announcementDirty = false;

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

static void fetchTask(void *param)
{
  unsigned long lastDepartureFetch = 0;
  unsigned long lastAnnouncementFetch = 0;

  for (;;)
  {
    if (!isNightTime())
    {
      unsigned long now = millis();

      if (lastDepartureFetch == 0 || now - lastDepartureFetch >= shortPollingInterval)
      {
        lastDepartureFetch = now;

        imhr::BusDeparture bus = imhr::fetchBusDeparture();
        imhr::TrainDeparture train = imhr::fetchTrainDeparture();

        xSemaphoreTake(dataMutex, portMAX_DELAY);
        sharedBus = bus;
        sharedTrain = train;
        departureDirty = true;
        xSemaphoreGive(dataMutex);

        Serial.println("----------");
      }

      if (lastAnnouncementFetch == 0 || now - lastAnnouncementFetch >= announcementPollingInterval)
      {
        lastAnnouncementFetch = now;
        Serial.printf("announcement fetch: %lu; free heap space before fetch: %lu\n", now, ESP.getFreeHeap());

        imhr::Announcement announcement = imhr::fetchAnnouncement();

        xSemaphoreTake(dataMutex, portMAX_DELAY);
        sharedAnnouncement = announcement;
        announcementDirty = true;
        xSemaphoreGive(dataMutex);

        Serial.printf("free heap space after fetch: %lu\n", ESP.getFreeHeap());
      }
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
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

  dataMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(fetchTask, "fetchTask", 8192, nullptr, 1, nullptr, 0);
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

  bool hasDepartureUpdate = false;
  bool hasAnnouncementUpdate = false;
  imhr::BusDeparture bus;
  imhr::TrainDeparture train;
  imhr::Announcement announcement;

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  if (departureDirty)
  {
    bus = sharedBus;
    train = sharedTrain;
    hasDepartureUpdate = true;
    departureDirty = false;
  }
  if (announcementDirty)
  {
    announcement = sharedAnnouncement;
    hasAnnouncementUpdate = true;
    announcementDirty = false;
  }
  xSemaphoreGive(dataMutex);

  if (hasDepartureUpdate)
    imhr::displayDeparture(bus.line.c_str(), bus.minutes, train.line.c_str(), train.minutes);

  if (hasAnnouncementUpdate)
    imhr::setAnnouncementMessage(announcement.hasMessage ? announcement.summary.c_str() : nullptr);

  imhr::tickScroll();
  delay(1);
}
