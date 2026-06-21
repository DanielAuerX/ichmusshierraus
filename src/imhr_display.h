#pragma once
#include <U8g2lib.h>

namespace imhr
{
    void displayInit();
    void displayWake();
    void displaySleep();
    void displayMessage(const char *line1, const char *line2 = nullptr);
    void displayDeparture(const char *busLine, int busMins,
                          const char *trainPlatform, int trainMins);
}