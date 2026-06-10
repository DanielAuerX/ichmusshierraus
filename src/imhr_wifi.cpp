#include "imhr_wifi.h"
#include <WiFi.h>
#include "secrets.h"

const char *ssid      = WIFI_SSID;
const char *password  = WIFI_PASSWORD;

void connectWiFi()
{
    Serial.println("connecting to wifi...");
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (++attempts > 20)
        {
            Serial.println("\nfailed :( check ssid/password");
            while (true)
                delay(1000);
        }
    }
    Serial.println("\nwifi connected — ip: " + WiFi.localIP().toString());
}