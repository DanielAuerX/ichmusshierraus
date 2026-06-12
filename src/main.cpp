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

const char *stationId = HVV_STATION_ID;
const uint32_t wifiRetryDelay = 600000;     // 10 minutes
const uint32_t readableMessageDelay = 2000; // 2 seconds
const uint32_t refreshInterval = 30000;     // 30 seconds

String buildBody()
{
  return String(R"({"version":54,"station":{"id":")") + HVV_STATION_ID + R"(","type":"STATION"},"time":{"date":"heute","time":"jetzt"},"maxList":5,"maxTimeOffset":60,"useRealtime":true})";
}

void setup()
{
  Serial.begin(115200);
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
  String body = buildBody();
  String url = String(BASE_URL_GTI_PUBLIC) + ENDPOINT_DEPARTURE_LIST;
  imhr::HttpResponse response = imhr::sendPostRequest(url, body);
  if (response.success())
  {
    JsonDocument doc;
    deserializeJson(doc, response.body);

    for (JsonObject dep : doc[JSON_KEY_DEPARTURES].as<JsonArray>())
    {
      String line = dep[JSON_KEY_LINE][JSON_KEY_NAME].as<String>();
      String dir = dep[JSON_KEY_LINE][JSON_KEY_DIRECTION].as<String>();
      int del = dep[JSON_KEY_DELAY].as<int>() / 60; // looks like seconds to me
      int mins = dep[JSON_KEY_TIME_OFFSET].as<int>() + del;
      Serial.printf("%-6s → %-25s in %2d min", line.c_str(), dir.c_str(), mins);
      if (del > 0)
        Serial.printf(" (+%d delay)", del);
      Serial.println("");
    }
    JsonObject next = doc[JSON_KEY_DEPARTURES][0];
    int mins = next[JSON_KEY_TIME_OFFSET].as<int>() + (next[JSON_KEY_DELAY].as<int>() / 60);
    String line = next[JSON_KEY_LINE][JSON_KEY_NAME].as<String>();
    imhr::displayDeparture(line.c_str(), mins);
    Serial.println("----------");
  }
  else
  {
    Serial.printf("HTTP error: %d\n", response.code);
    Serial.println(response.body);
    imhr::displayMessage("HTTP error", String(response.code).c_str());
  }

  delay(refreshInterval);
}
