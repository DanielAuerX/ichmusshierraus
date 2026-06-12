#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "imhr_secrets.h"
#include "imhr_endpoints.h"
#include "imhr_wifi.h"
#include "imhr_http_client.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

const char *stationId = HVV_STATION_ID;

String buildBody() {
  return String(R"({"version":54,"station":{"id":")") 
       + HVV_STATION_ID 
       + R"(","type":"STATION"},"time":{"date":"heute","time":"jetzt"},"maxList":5,"maxTimeOffset":60,"useRealtime":true})";
}

void setup()
{
  Serial.begin(115200);
  while (!connectWiFi())
  {
    delay(600000);
  }
  display.begin();
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 12, "connected!");
  display.sendBuffer();
}

void loop()
{
  String body = buildBody();
  String url = String(BASE_URL_GTI_PUBLIC) + ENDPOINT_DEPARTURE_LIST;
  HttpResponse response = sendPostRequest(url, body);
  if (response.success())
  {
    JsonDocument doc;
    deserializeJson(doc, response.body);

    for (JsonObject dep : doc["departures"].as<JsonArray>())
    {
      String line = dep["line"]["name"].as<String>();
      String dir = dep["line"]["direction"].as<String>();
      int mins = dep["timeOffset"].as<int>() + dep["delay"].as<int>();
      int del = dep["delay"].as<int>();
      Serial.printf("%-6s → %-25s in %2d min", line.c_str(), dir.c_str(), mins);
      if (del > 0)
        Serial.printf(" (+%d delay)", del);
      Serial.println();
    }

    JsonObject next = doc["departures"][0];
    int mins = next["timeOffset"].as<int>() + next["delay"].as<int>();
    String line = next["line"]["name"].as<String>();

    char line1[32];
    char line2[32];
    snprintf(line1, sizeof(line1), "Bus %s", line.c_str());
    snprintf(line2, sizeof(line2), "%d min", mins);

    display.clearBuffer();
    display.setFont(u8g2_font_logisoso28_tr); // big font
    display.drawStr(0, 30, line1);
    display.drawStr(0, 62, line2);
    display.sendBuffer();
  }
  else
  {
    Serial.printf("HTTP error: %d\n", response.code);
    Serial.println(response.body);
  }

  delay(30000);
}
