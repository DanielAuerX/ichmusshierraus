#include "imhr_wifi.h"
#include <WiFi.h>
#include "imhr_secrets.h"

namespace imhr
{
    static const char *ssid = WIFI_SSID;
    static const char *password = WIFI_PASSWORD;

    bool connectWiFi()
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
                return false;
            }
        }
        Serial.println("\nwifi connected — ip: " + WiFi.localIP().toString());
        return true;
    }
}
