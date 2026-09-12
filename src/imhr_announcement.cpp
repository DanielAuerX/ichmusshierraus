#include "imhr_announcement.h"
#include "imhr_json_keys.h"
#include "imhr_endpoints.h"
#include "imhr_secrets.h"
#include "imhr_http_client.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "imhr_http_client.h"

namespace imhr
{

    static const String announcementsUrl = String(BASE_URL_GTI_PUBLIC) + ENDPOINT_ANNOUNCEMENTS;
    static const char *ignoreKeywords[] = ANNOUNCEMENT_IGNORE_KEYWORDS;

    static bool isIgnored(JsonObject &announcement)
    {
        String reason = announcement[JSON_KEY_REASON].as<String>();
        if (reason != "UNDEFINED_PROBLEM")
            return false;

        String summary = announcement[JSON_KEY_SUMMARY].as<String>();
        for (int i = 0; i < ANNOUNCEMENT_IGNORE_COUNT; i++)
        {
            if (summary.indexOf(ignoreKeywords[i]) >= 0)
                return true;
        }
        return false;
    }

    static String buildAnnouncementsBody()
    {
        JsonDocument doc;
        doc[JSON_KEY_VERSION] = 54;
        doc[JSON_KEY_LANGUAGE] = "de";
        doc[JSON_KEY_NAMES][0] = ANNOUNCEMENT_LINE; // ask the server to scope results instead of returning the whole network's announcements
        String body;
        serializeJson(doc, body);
        return body;
    }

    static bool affects(JsonObject &announcement)
    {
        for (JsonObject location : announcement[JSON_KEY_LOCATIONS].as<JsonArray>())
        {
            String name = location["name"].as<String>();
            if (name.startsWith(ANNOUNCEMENT_LINE))
                return true;
        }
        return false;
    }

    static bool requestAnnouncements(HTTPClient &http, const String &body, const String &signature)
    {
        http.begin(announcementsUrl);
        http.setTimeout(15000);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("geofox-auth-user", HVV_USER);
        http.addHeader("geofox-auth-signature", signature);

        int code = http.POST((uint8_t *)body.c_str(), body.length());
        if (code != 200)
        {
            Serial.printf("Announcements HTTP error: %d\n", code);
            return false;
        }
        return true;
    }

    static JsonDocument buildAnnouncementFilter()
    {
        JsonDocument filter;
        filter["announcements"][0]["locations"][0]["name"] = true;
        filter["announcements"][0]["summary"] = true;
        filter["announcements"][0]["reason"] = true;
        return filter;
    }

    static bool parseAnnouncements(Stream &stream, JsonDocument &doc)
    {
        JsonDocument filter = buildAnnouncementFilter();
        DeserializationError error = deserializeJson(doc, stream, DeserializationOption::Filter(filter));
        Serial.printf("free heap after parsing: %d\n", ESP.getFreeHeap());

        if (error)
        {
            Serial.printf("ERROR: announcements json error: %s\n", error.c_str());
            return false;
        }
        return true;
    }

    static String collectMatchingSummaries(JsonDocument &doc, int &matchCount)
    {
        String combined;
        matchCount = 0;

        for (JsonObject announcement : doc[JSON_KEY_ANNOUNCEMENTS].as<JsonArray>())
        {
            if (!affects(announcement))
                continue;

            String summary = announcement[JSON_KEY_SUMMARY].as<String>();

            if (isIgnored(announcement))
            {
                Serial.println(String(ANNOUNCEMENT_LINE) + " announcement (ignored): " + summary);
                continue;
            }

            Serial.println(String(ANNOUNCEMENT_LINE) + " announcement: " + summary);

            if (matchCount > 0)
                combined += "   +++   ";
            combined += summary;
            matchCount++;
        }

        return combined;
    }

    Announcement fetchAnnouncement()
    {
        String body = buildAnnouncementsBody();
        String signature = signRequest(body.c_str(), HVV_SECRET);

        HTTPClient http;
        if (!requestAnnouncements(http, body, signature))
        {
            http.end();
            return {false, ""};
        }

        JsonDocument doc;
        bool parsed = parseAnnouncements(http.getStream(), doc);
        http.end();

        if (!parsed)
            return {false, ""};

        int matchCount;
        String combined = collectMatchingSummaries(doc, matchCount);

        if (matchCount == 0)
            return {false, ""};

        Serial.printf("found %d announcement(s)\n", matchCount);
        Serial.printf("free heap after building combined string: %d\n", ESP.getFreeHeap());
        combined += "                          ";
        return {true, combined};
    }

}