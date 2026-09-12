#pragma once

#include <Arduino.h>

namespace imhr
{

    struct BusDeparture
    {
        String line;
        int minutes;
    };

    struct TrainDeparture
    {
        String line;
        int minutes;
    };

    TrainDeparture fetchTrainDeparture();
    BusDeparture fetchBusDeparture();

}