#include <Arduino.h>
#include <WebServer.h>

#include "bus.h"

WebServer server(80);

void ui_setup()
{
    server.on("/", []() {
        server.send(200, "text/plain", "ok");
    });
    server.on("/memory", []() {
        server.send(200, "application/octet-stream", bus_memory, BUS_MEMORY_LEN);
    });
    server.on("/status", []() {
        String response = "total count: " + bus_count;
        server.send(200, "text/plain", response);
    });
    server.begin();

    Serial.println("[ui      ] started on port 80");
}

void ui_loop()
{
    server.handleClient();
}