#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>
#include "mbedtls/md.h"
#include <Wire.h>
#include <U8g2lib.h>
#include "secrets.h"
#include "imhr_wifi.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

const char *hvvUser   = HVV_USER;
const char *hvvSecret = HVV_SECRET;
const char *stationId = HVV_STATION_ID;

String buildBody() {
  return String(R"({"version":54,"station":{"id":")") 
       + HVV_STATION_ID 
       + R"(","type":"STATION"},"time":{"date":"heute","time":"jetzt"},"maxList":5,"maxTimeOffset":60,"useRealtime":true})";
}

void setup()
{
  Serial.begin(115200);
  connectWiFi();
  display.begin();
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 12, "connected!");
  display.sendBuffer();
}

String signRequest(const char *payload, const char *secret)
{
  byte hmac[20];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
  mbedtls_md_hmac_starts(&ctx, (unsigned char *)secret, strlen(secret));
  mbedtls_md_hmac_update(&ctx, (unsigned char *)payload, strlen(payload));
  mbedtls_md_hmac_finish(&ctx, hmac);
  mbedtls_md_free(&ctx);
  return base64::encode(hmac, 20);
}

void loop()
{
  String body = buildBody();
  String signature = signRequest(body.c_str(), hvvSecret);
  HTTPClient http;
  http.begin("https://gti.geofox.de/gti/public/departureList");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("geofox-auth-user", hvvUser);
  http.addHeader("geofox-auth-signature", signature);

  int code = http.POST((uint8_t *)body.c_str(), body.length());
  if (code == 200)
  {
    JsonDocument doc;
    deserializeJson(doc, http.getStream());

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
    Serial.printf("HTTP error: %d\n", code);
    Serial.println(http.getString());
  }

  http.end();
  delay(60000);
}
