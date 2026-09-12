#pragma once
#include <Arduino.h>

namespace imhr {

struct Announcement {
    bool hasMessage;
    String summary;
};

Announcement fetchAnnouncement();

}