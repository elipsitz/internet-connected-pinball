#include <Arduino.h>
#include <HTTPUpdateServer.h>
#include <WebServer.h>
#include <hardware/gpio.h>

#include "bus.h"
#include "log.h"

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
        String response;
        response += "uptime (sec): ";
        response += ((double)millis()) / 1000.0;
        response += "\ntotal bus writes: ";
        response += bus_count;
        response += "\nheap usage (bytes): ";
        response += rp2040.getUsedHeap();
        response += " of ";
        response += rp2040.getTotalHeap();
        web_server.send(200, "text/plain", response);
    });
    web_server.on("/gpio", []() {
        uint32_t gpio = gpio_get_all();
        String response = "raw: ";
        response += gpio;
        response += "\n";
        const char* gpio_names[] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "CL", "WR", "OE", "__", "__", "__", "__", "__", "__", "__", "__", "__"};
        for (int i = 0; i < 30; i++) {
            response += "GPIO ";
            response += i;
            response += " (";
            response += gpio_names[i];
            response += "): ";
            response += (gpio >> i) & 1;
            response += "\n";
        }
        web_server.send(200, "text/plain", response);
    });
    web_server.on("/logs", []() {
        String response;
        for (String& log : log_get_logs()) {
            response += log;
            response += '\n';
        }
        web_server.send(200, "text/plain", response);
    });
    http_updater.setup(&web_server);
    web_server.begin();

    log_log("ui", "started on port 80");
    log_log("ui", "OTA available at /update");
}

void ui_loop()
{
    web_server.handleClient();
}