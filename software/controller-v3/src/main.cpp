#include <Arduino.h>
#include <WiFi.h>
#include <LEAmDNS.h>

#include "bus.h"
#include "secrets.h"
#include "ui.h"
#include "game.h"

WiFiMulti multi;

void setup() {
  Serial.begin();

  Serial.println();
  Serial.println();
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
}

void loop() {
  game_check_state();
  ui_loop();
  MDNS.update();
  delay(100);
}

void setup1() {
  bus_run();
}

void loop1() {
}