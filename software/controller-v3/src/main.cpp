#include <Arduino.h>
#include <WiFi.h>
#include <LEAmDNS.h>
#include <pico/cyw43_arch.h>

#include "bus.h"
#include "secrets.h"
#include "ui.h"
#include "game.h"

WiFiMulti multi;

#define PING_PERIOD (60 * 1000)
static uint32_t last_ping = 0;

void setup() {
  Serial.begin();
  Serial.print("[main    ] Booted up.");
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  // Connect to wifi.
  Serial.print("[wifi    ] Connecting to ");
  Serial.println(CONFIG_WIFI_SSID);
  multi.addAP(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);
  if (multi.run() != WL_CONNECTED) {
    Serial.println("Unable to connect to network, rebooting in 10 seconds...");
    delay(10000);
    rp2040.reboot();
  }
  Serial.println("");
  Serial.println("[wifi    ] connected");
  Serial.print("[wifi    ] IP address: ");
  Serial.println(WiFi.localIP());

  Serial.printf("[mdns    ] hostname %s.local\n", CONFIG_HOSTNAME);
  MDNS.begin(CONFIG_HOSTNAME);
  ui_setup();
  MDNS.addService("http", "tcp", 80);

  game_init();

  digitalWrite(PIN_LED, LOW);
}

void loop() {
  game_check_state();
  ui_loop();
  MDNS.update();

  // Make sure we're still connectd.
  if (!WiFi.connected()) {
    Serial.print("[wifi    ] No longer connected! Reconnecting...");
    if (multi.run() != WL_CONNECTED) {
      Serial.println("[wifi    ] Unable to reconnect. Rebooting.");
      delay(500);
      rp2040.reboot();
    }
    Serial.print("[wifi    ] Reconnected");
  }

  // XXX: Hack to keep the WiFi connection active.
  // https://github.com/micropython/micropython/issues/9455 (?)
  if (millis() - last_ping > PING_PERIOD) {
    Serial.println("[wifi    ] Pinging server...");
    WiFi.ping(CONFIG_WEB_HOST);
    Serial.println("[wifi    ] ping complete.");
    last_ping = millis();
  }

  delay(100);
}

void setup1() {
  bus_run();
}

void loop1() {
}