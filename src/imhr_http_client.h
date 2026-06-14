#pragma once

#include <Arduino.h>

namespace imhr
{
    struct HttpResponse
    {
        int code;
        String body;
        bool success() const { return code == 200; }
    };

    HttpResponse sendPostRequest(const String &url, const String &body);
}
