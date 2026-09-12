#include "imhr_display_logic.h"
#include <cstdio>

namespace imhr
{

    void determineMinuteDisplay(char *minuteDisplay, size_t size, int minutes)
    {
        if (minutes < 0)
            snprintf(minuteDisplay, size, "--");
        else if (minutes == 0)
            snprintf(minuteDisplay, size, "sofort");
        else
            snprintf(minuteDisplay, size, "%d min", minutes);
    }

}
