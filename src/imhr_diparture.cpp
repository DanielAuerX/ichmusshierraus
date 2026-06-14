#include "imhr_diparture.h"
#include "imhr_json_keys.h"
#include "imhr_endpoints.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "imhr_http_client.h"
#include "imhr_secrets.h"

namespace imhr
{

    const int trainBufferMins = 10;
    static const char *wrongDirections[] = WRONG_DIRECTIONS;

    const String url = String(BASE_URL_GTI_PUBLIC) + ENDPOINT_DEPARTURE_LIST;

    String getTimeWithBuffer()
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

    String buildTrainBody()
    {
        String time = getTimeWithBuffer();
        Serial.println("Requesting departures for time: " + time);
        return String(R"({"version":54,"station":{"id":")") + HVV_STATION_ID_TRAIN + R"(","type":"STATION"},"time":{"date":"heute","time":")" + time + R"("},"maxList":5,"maxTimeOffset":60,"useRealtime":true,"serviceTypes":["ZUG"]})";
    }

    // "direction" is always 1...
    bool isWrongDirection(const String &direction)
    {
        for (int i = 0; i < WRONG_DIRECTIONS_COUNT; i++)
        {
            if (direction.indexOf(wrongDirections[i]) >= 0)
                return true;
        }
        return false;
    }

    int getDelayInMinutes(JsonObject &departure)
    {
        if (departure[JSON_KEY_DELAY].isNull())
            return 0;
        return departure[JSON_KEY_DELAY].as<int>() / 60;
    }

    TrainDeparture fetchTrainDeparture()
    {
        int minutesDisplay = -1;
        String lineDisplay = "Zug";

        imhr::HttpResponse pollingResponse = imhr::sendPostRequest(url, buildTrainBody());
        if (pollingResponse.success())
        {
            JsonDocument jsonDocument;
            DeserializationError error = deserializeJson(jsonDocument, pollingResponse.body);
            if (error)
            {
                Serial.printf("Train JSON error: %s\n", error.c_str());
            }
            else
            {
                for (JsonObject currentDiparture : jsonDocument[JSON_KEY_DEPARTURES].as<JsonArray>())
                {
                    String direction = currentDiparture[JSON_KEY_LINE][JSON_KEY_DIRECTION].as<String>();
                    if (isWrongDirection(direction))
                    {
                        continue; // wrong direction.. ich muss hier raus
                    }
                    int delay = getDelayInMinutes(currentDiparture);
                    minutesDisplay = currentDiparture[JSON_KEY_TIME_OFFSET].as<int>() + trainBufferMins + delay;

                    if (!currentDiparture[JSON_KEY_REALTIME_PLATFORM].isNull())
                    {
                        lineDisplay = currentDiparture[JSON_KEY_REALTIME_PLATFORM].as<String>();
                    }
                    else if (!currentDiparture[JSON_KEY_PLATFORM].isNull())
                    {
                        lineDisplay = currentDiparture[JSON_KEY_PLATFORM].as<String>();
                    }
                    lineDisplay.replace("Gleis ", "");
                    lineDisplay = currentDiparture[JSON_KEY_LINE][JSON_KEY_NAME].as<String>() + "-" + lineDisplay;

                    Serial.printf("%-25s → %-25s in %2d min \n",
                                  lineDisplay.c_str(), direction.c_str(), minutesDisplay);

                    if (delay > 0)
                    {
                        Serial.printf(" (+%d delay)", delay);
                    }
                    Serial.println();
                    break;
                }
            }
        }
        else
        {
            Serial.printf("Train HTTP error: %d\n", pollingResponse.code);
        }
        return {lineDisplay, minutesDisplay};
    }

    String buildBusBody()
    {
        return String(R"({"version":54,"station":{"id":")") + HVV_STATION_ID_BUS + R"(","type":"STATION"},"time":{"date":"heute","time":"jetzt"},"maxList":3,"maxTimeOffset":60,"useRealtime":true})";
    }

    BusDeparture fetchBusDeparture()
    {
        int minutes = -1;
        String line = "Bus";
        imhr::HttpResponse pollingResponse = imhr::sendPostRequest(url, buildBusBody());
        if (pollingResponse.success())
        {
            JsonDocument jsonDocument;
            DeserializationError error = deserializeJson(jsonDocument, pollingResponse.body);
            if (error)
            {
                Serial.printf("Bus JSON error: %s\n", error.c_str());
            }
            else
            {
                for (JsonObject currentDeparture : jsonDocument[JSON_KEY_DEPARTURES].as<JsonArray>())
                {
                    String line = currentDeparture[JSON_KEY_LINE][JSON_KEY_NAME].as<String>();
                    String direction = currentDeparture[JSON_KEY_LINE][JSON_KEY_DIRECTION].as<String>();
                    int delay = getDelayInMinutes(currentDeparture);
                    int minutes = currentDeparture[JSON_KEY_TIME_OFFSET].as<int>() + delay;
                    Serial.printf("%-6s → %-25s in %2d min", line.c_str(), direction.c_str(), minutes);
                    if (delay > 0)
                    {
                        Serial.printf(" (+%d delay)", delay);
                    }
                    Serial.println();
                }

                JsonObject next = jsonDocument[JSON_KEY_DEPARTURES][0];
                minutes = next[JSON_KEY_TIME_OFFSET].as<int>() + (next[JSON_KEY_DELAY].as<int>() / 60);
                line = next[JSON_KEY_LINE][JSON_KEY_NAME].as<String>();
            }
        }
        else
        {
            Serial.printf("Bus HTTP error: %d\n", pollingResponse.code);
        }

        return {line, minutes};
    }

}
