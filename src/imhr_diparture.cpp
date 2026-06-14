#include "imhr_diparture.h"
#include "imhr_json_keys.h"
#include "imhr_endpoints.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "imhr_http_client.h"
#include "imhr_secrets.h"

namespace imhr
{

    static const int trainBufferMins = 10;
    static const char *wrongDirections[] = WRONG_DIRECTIONS;

    static const String departureListUrl = String(BASE_URL_GTI_PUBLIC) + ENDPOINT_DEPARTURE_LIST;

    static String getTimeWithBuffer()
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

    static JsonDocument buildBaseBody()
    {
        JsonDocument body;
        body[JSON_KEY_VERSION] = 54;
        body[JSON_KEY_MAX_TIME_OFFSET] = 60;
        body[JSON_KEY_USE_REALTIME] = true;
        body[JSON_KEY_TIME][JSON_KEY_DATE] = "heute";
        body[JSON_KEY_STATION][JSON_KEY_TYPE] = "STATION";
        return body;
    }

    static String buildTrainBody()
    {
        JsonDocument trainBody = buildBaseBody();
        trainBody[JSON_KEY_MAX_LIST] = 5;
        trainBody[JSON_KEY_STATION][JSON_KEY_ID] = HVV_STATION_ID_TRAIN;
        const String timeWithBuffer = getTimeWithBuffer();
        trainBody[JSON_KEY_TIME][JSON_KEY_TIME] = timeWithBuffer;
        trainBody[JSON_KEY_SERVICE_TYPES][0] = "ZUG";

        String body;
        serializeJson(trainBody, body);

        Serial.println("requesting departures for time: " + timeWithBuffer);
        return body;
    }

    static String buildBusBody()
    {
        JsonDocument busBody = buildBaseBody();
        busBody[JSON_KEY_MAX_LIST] = 3;
        busBody[JSON_KEY_STATION][JSON_KEY_ID] = HVV_STATION_ID_BUS;
        busBody[JSON_KEY_TIME][JSON_KEY_TIME] = "jetzt";

        String body;
        serializeJson(busBody, body);
        return body;
    }

    // "direction" is always 1...
    static bool isWrongDirection(const String &direction)
    {
        for (int i = 0; i < WRONG_DIRECTIONS_COUNT; i++)
        {
            if (direction.indexOf(wrongDirections[i]) >= 0)
                return true;
        }
        return false;
    }

    static int getDelayInMinutes(JsonObject &departure)
    {
        if (departure[JSON_KEY_DELAY].isNull())
            return 0;
        return departure[JSON_KEY_DELAY].as<int>() / 60;
    }

    static String formatPlatformLabel(JsonObject &departure)
    {
        const String platformPrefix = "Gleis ";
        String platform;
        if (!departure[JSON_KEY_REALTIME_PLATFORM].isNull())
            platform = departure[JSON_KEY_REALTIME_PLATFORM].as<String>();
        else if (!departure[JSON_KEY_PLATFORM].isNull())
            platform = departure[JSON_KEY_PLATFORM].as<String>();

        platform.replace(platformPrefix, "");

        String lineName = departure[JSON_KEY_LINE][JSON_KEY_NAME].as<String>();
        return platform.isEmpty() ? lineName : lineName + "-" + platform;
    }

    static void logCurrentDeparture(int minutes, const String &direction, const String &line, int delay)
    {
        Serial.printf("%-25s → %-25s in %2d min",
                      line.c_str(), direction.c_str(), minutes);

        if (delay > 0)
        {
            Serial.printf(" (+%d delay)", delay);
        }
        Serial.println();
    }

    template <typename Callback>
    static void fetchDepartures(const String &body, const char *label, Callback onDeparture)
    {
        HttpResponse response = sendPostRequest(departureListUrl, body);
        if (!response.success())
        {
            Serial.printf("%s HTTP error: %d\n", label, response.code);
            Serial.printf("%s HTTP body: %s\n", label, response.body.c_str());
            return;
        }

        JsonDocument jsonDocument;
        DeserializationError error = deserializeJson(jsonDocument, response.body);
        if (error)
        {
            Serial.printf("%s JSON error: %s\n", label, error.c_str());
            return;
        }

        for (JsonObject departure : jsonDocument[JSON_KEY_DEPARTURES].as<JsonArray>())
        {
            if (onDeparture(departure))
                break;
        }
    }

    TrainDeparture fetchTrainDeparture()
    {
        int minutesDisplay = -1;
        String lineDisplay = "Zug";

        fetchDepartures(buildTrainBody(), "Train", [&](JsonObject &departure)
                        {
        String direction = departure[JSON_KEY_LINE][JSON_KEY_DIRECTION].as<String>();
        if (isWrongDirection(direction))
            return false; // skip.. ich muss hier raus
 
        int delay = getDelayInMinutes(departure);
        minutesDisplay = departure[JSON_KEY_TIME_OFFSET].as<int>() + trainBufferMins + delay;
        lineDisplay = formatPlatformLabel(departure);
        logCurrentDeparture(minutesDisplay, direction, lineDisplay, delay);
        return true; });

        return {lineDisplay, minutesDisplay};
    }

    BusDeparture fetchBusDeparture()
    {
        int minutesDisplay = -1;
        String lineDisplay = "Bus";
        bool isFirst = true;

        fetchDepartures(buildBusBody(), "Bus", [&](JsonObject &departure)
                        {
                            String line = departure[JSON_KEY_LINE][JSON_KEY_NAME].as<String>();
                            String direction = departure[JSON_KEY_LINE][JSON_KEY_DIRECTION].as<String>();
                            int delay = getDelayInMinutes(departure);
                            int minutes = departure[JSON_KEY_TIME_OFFSET].as<int>() + delay;
                            logCurrentDeparture(minutes, direction, line, delay);

                            if (isFirst)
                            {
                                minutesDisplay = minutes;
                                lineDisplay = line;
                                isFirst = false;
                            }
                            return false; // log all departures
                        });

        return {lineDisplay, minutesDisplay};
    }

}
