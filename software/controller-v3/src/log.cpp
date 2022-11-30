#include <stdio.h>

#include <Arduino.h>

#include "log.h"

void log_init()
{
    Serial.begin();
}

void log_log(const char* tag, const char* format, ...)
{
    String formatted;
    
    uint32_t time = millis();
    char buffer[128];
    int len = snprintf(buffer, sizeof(buffer), "[%d.%03d][%8.8s] ", time / 1000, time % 1000, tag);
    formatted.concat(buffer, len);

    va_list args;
    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    if (len > 0) {
        if (len >= sizeof(buffer)) {
            len = sizeof(buffer) - 1;
        }
        formatted.concat(buffer, len);
    }
    va_end(args);

    Serial.println(formatted);
}