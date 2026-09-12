#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

namespace imhr
{

    int getDelayInMinutes(JsonObject &departure);
    String formatPlatformLabel(JsonObject &departure);

}
