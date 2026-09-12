#include "imhr_departure_logic.h"
#include "imhr_json_keys.h"

namespace imhr
{

    int getDelayInMinutes(JsonObject &departure)
    {
        if (departure[JSON_KEY_DELAY].isNull())
            return 0;
        return departure[JSON_KEY_DELAY].as<int>() / 60;
    }

    String formatPlatformLabel(JsonObject &departure)
    {
        const String platformPrefix = "Gleis ";
        String platform;
        if (!departure[JSON_KEY_REALTIME_PLATFORM].isNull())
            platform = departure[JSON_KEY_REALTIME_PLATFORM].as<String>();
        else if (!departure[JSON_KEY_PLATFORM].isNull())
            platform = departure[JSON_KEY_PLATFORM].as<String>();

        platform.replace(platformPrefix, "");

        String lineName = departure[JSON_KEY_LINE][JSON_KEY_NAME].as<String>();
        return platform.length() == 0 ? lineName : lineName + "-" + platform;
    }

}
