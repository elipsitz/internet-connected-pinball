#include <Arduino.h>
#include <WiFi.h>
#include <LEAmDNS.h>
#include <pico/cyw43_arch.h>

#include "bus.h"
#include "secrets.h"
#include "ui.h"
#include "game.h"
#include "log.h"

WiFiMulti multi;

#define PING_PERIOD (60 * 1000)
static uint32_t last_ping = 0;

void setup() {
  log_init();
  log_log("main", "Initializing...");
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  // Connect to wifi.
  log_log("wifi", "Connecting to %s", CONFIG_WIFI_SSID);
  multi.addAP(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);
  if (multi.run() != WL_CONNECTED) {
    log_log("wifi", "Unable to connect, rebooting in 10 seconds...");
    delay(10000);
    rp2040.reboot();
  }
  log_log("wifi", "connected");
  String ip = WiFi.localIP().toString();
  log_log("wifi", "Connected! IP address: %s", ip.c_str());

  log_log("mdns", "hostname %s.local", CONFIG_HOSTNAME);
  MDNS.begin(CONFIG_HOSTNAME);
  ui_setup();
  MDNS.addService("http", "tcp", 80);

  game_init();

  digitalWrite(PIN_LED, LOW);
  log_log("main", "Init done");
}

void loop() {
  game_check_state();
  ui_loop();
  MDNS.update();

  // XXX: Hack to keep the WiFi connection active.
  // https://github.com/micropython/micropython/issues/9455 (?)
  if (millis() - last_ping > PING_PERIOD) {
    log_log("wifi", "Pinging server...");
    WiFi.ping(CONFIG_WEB_HOST);
    log_log("wifi", "ping complete.");
    last_ping = millis();
  }

  delay(100);
}

void setup1() {
  bus_run();
}

void loop1() {
}