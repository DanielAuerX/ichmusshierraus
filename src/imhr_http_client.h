#pragma once

#include <Arduino.h>

struct HttpResponse {
    int code;
    String body;
    bool success() const { return code == 200; }
};

String signRequest(const char *payload, const char *secret);

HttpResponse sendPostRequest(const String &url, const String &body);