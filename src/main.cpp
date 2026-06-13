#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "imhr_secrets.h"
#include "imhr_endpoints.h"
#include "imhr_wifi.h"
#include "imhr_http_client.h"
#include "imhr_display.h"
#include "imhr_json_keys.h"

const char *stationIdBus = HVV_STATION_ID_BUS;
const char *stationIdTrain = HVV_STATION_ID_TRAIN;
const uint32_t wifiRetryDelay = 600000;      // 10 minutes
const uint32_t readableMessageDelay = 2000;  // 2 seconds
const uint32_t shortPollingInterval = 30000; // 30 seconds
const int trainBufferMins = 10;
static const char *wrongDirections[] = WRONG_DIRECTIONS;


String getTimePlusTen()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return "jetzt";

  timeinfo.tm_min += trainBufferMins;
  mktime(&timeinfo); // normalize hour overflow

  char buf[6];
  strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
  return String(buf);
}

String buildBusBody()
{
  return String(R"({"version":54,"station":{"id":")") + HVV_STATION_ID_BUS + R"(","type":"STATION"},"time":{"date":"heute","time":"jetzt"},"maxList":3,"maxTimeOffset":60,"useRealtime":true})";
}

String buildTrainBody()
{
  String time = getTimePlusTen();
  Serial.println("Requesting departures for time: " + time);
  return String(R"({"version":54,"station":{"id":")") + HVV_STATION_ID_TRAIN + R"(","type":"STATION"},"time":{"date":"heute","time":")" + time + R"("},"maxList":5,"maxTimeOffset":60,"useRealtime":true,"serviceTypes":["ZUG"]})";
}

// "direction" is always 1...
bool isWrongDirection(const String &direction) {
    for (int i = 0; i < WRONG_DIRECTIONS_COUNT; i++) {
        if (direction.indexOf(wrongDirections[i]) >= 0)
            return true;
    }
    return false;
}


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
  String url = String(BASE_URL_GTI_PUBLIC) + ENDPOINT_DEPARTURE_LIST;

  // BUUUS
  int busMins = -1;
  String busLine = "Bus";
  imhr::HttpResponse busResponse = imhr::sendPostRequest(url, buildBusBody());
  if (busResponse.success())
  {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, busResponse.body);
    if (err)
    {
      Serial.printf("Bus JSON error: %s\n", err.c_str());
    }
    else
    {
      for (JsonObject dep : doc[JSON_KEY_DEPARTURES].as<JsonArray>())
      {
        String line = dep[JSON_KEY_LINE][JSON_KEY_NAME].as<String>();
        String dir = dep[JSON_KEY_LINE][JSON_KEY_DIRECTION].as<String>();
        int del = dep[JSON_KEY_DELAY].as<int>() / 60;
        int mins = dep[JSON_KEY_TIME_OFFSET].as<int>() + del;
        Serial.printf("%-6s → %-25s in %2d min", line.c_str(), dir.c_str(), mins);
        if (del > 0)
        {
          Serial.printf(" (+%d delay)", del);
        }
        Serial.println();
      }

      JsonObject next = doc[JSON_KEY_DEPARTURES][0];
      busMins = next[JSON_KEY_TIME_OFFSET].as<int>() + (next[JSON_KEY_DELAY].as<int>() / 60);
      busLine = next[JSON_KEY_LINE][JSON_KEY_NAME].as<String>();
    }
  }
  else
  {
    Serial.printf("Bus HTTP error: %d\n", busResponse.code);
  }

  // TRAIN
  int trainMins = -1;
  String lineDisplay = "";

  imhr::HttpResponse trainResponse = imhr::sendPostRequest(url, buildTrainBody());
  if (trainResponse.success())
  {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, trainResponse.body);
    if (err)
    {
      Serial.printf("Train JSON error: %s\n", err.c_str());
    }
    else
    {
      for (JsonObject dep : doc[JSON_KEY_DEPARTURES].as<JsonArray>())
      {
        String direction = dep[JSON_KEY_LINE][JSON_KEY_DIRECTION].as<String>();
        if (isWrongDirection(direction))
        {
          continue; // wrong direction.. ich muss hier raus
        }
        int del = dep[JSON_KEY_DELAY].as<int>() / 60;
        trainMins = dep[JSON_KEY_TIME_OFFSET].as<int>() + trainBufferMins + del;

        if (!dep[JSON_KEY_REALTIME_PLATFORM].isNull())
        {
          lineDisplay = dep[JSON_KEY_REALTIME_PLATFORM].as<String>();
        }
        else if (!dep[JSON_KEY_PLATFORM].isNull())
        {
          lineDisplay = dep[JSON_KEY_PLATFORM].as<String>();
        }
        lineDisplay.replace("Gleis ", "");
        lineDisplay = dep[JSON_KEY_LINE][JSON_KEY_NAME].as<String>() + "-" + lineDisplay;

        Serial.printf("%-25s → %-25s in %2d min \n",
                      lineDisplay.c_str(), direction.c_str(), trainMins);

        if (del > 0)
        {
          Serial.printf(" (+%d delay)", del);
        }
        Serial.println();
        break;
      }
    }
  }
  else
  {
    Serial.printf("Train HTTP error: %d\n", trainResponse.code);
  }

  imhr::displayDeparture(busLine.c_str(), busMins, lineDisplay.c_str(), trainMins);
  Serial.println("----------");

  delay(shortPollingInterval);
}
