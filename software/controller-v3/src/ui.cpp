#include <Arduino.h>
#include <HTTPUpdateServer.h>
#include <WebServer.h>

#include "bus.h"

WebServer web_server(80);
HTTPUpdateServer http_updater;

void ui_setup()
{
    web_server.on("/", []() {
        web_server.send(200, "text/plain", "ok");
    });
    web_server.on("/memory", []() {
        web_server.send(200, "application/octet-stream", bus_memory, BUS_MEMORY_LEN);
    });
    web_server.on("/status", []() {
        String response = "total count: ";
        response += bus_count;
        web_server.send(200, "text/plain", response);
    });
    http_updater.setup(&web_server);
    web_server.begin();

    Serial.println("[ui      ] started on port 80");
    Serial.println("[ui      ] OTA available at /update");
}

void ui_loop()
{
    web_server.handleClient();
}