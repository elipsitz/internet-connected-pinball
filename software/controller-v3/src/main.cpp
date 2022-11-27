#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <LEAmDNS.h>

#include "bus.h"
#include "secrets.h"
#include "ui.h"

#define ADDR_FLAG_GAME_OVER (0xC9)

WiFiMulti multi;

bool upload_score();

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
}

void loop() {
  // Check every 100ms to see if we need to upload scores.
  static uint8_t last_flag_game_over = 0xFF;
  uint8_t flag_game_over = bus_memory[ADDR_FLAG_GAME_OVER];
  if (flag_game_over != last_flag_game_over) {
    Serial.printf("[game    ] flag game over %d -> %d\n", last_flag_game_over, flag_game_over);

    if (flag_game_over == 0) {
      Serial.println("[game    ] Started new game.");
    }
    if (last_flag_game_over == 0) {
      Serial.println("[game    ] Game over!");
      upload_score();
    }

    last_flag_game_over = flag_game_over;
  }

  ui_loop();
  MDNS.update();
  delay(100);
}

void setup1() {
  bus_run();
}

void loop1() {
}

bool upload_score()
{
  // First construct payload using current snapshot of memory.
  String payload = "{\"machine_id\": 1, \"format\": \"williams-sys7-v1\"}";
  payload.concat((char)0);
  payload.concat(const_cast<const uint8_t*>(bus_memory), BUS_MEMORY_LEN);

  Serial.println("[uploader] Uploading scores...");
  HTTPClient http;
  if (CONFIG_WEB_HTTPS) {
    http.setInsecure(); // Do not validate the server certificate.
  }
  if (!http.begin(CONFIG_WEB_HOST, CONFIG_WEB_PORT, CONFIG_WEB_ADD_SCORE_ENDPOINT, CONFIG_WEB_HTTPS)) {
    Serial.printf("[uploader] Failed: http.begin failed\n");
    return false;
  }
  http.addHeader("Authorization", CONFIG_WEB_AUTH);
  http.addHeader("Content-Type", "application/octet-stream");
  int httpCode = http.POST(payload);

  bool success = false;
  if (httpCode == HTTP_CODE_OK) {
    Serial.print("[uploader] ok: ");
    Serial.println(http.getString());
    success = true;
  } else if (httpCode > 0) {
    Serial.printf("[uploader] Failed: http %d\n", httpCode);
  } else {
    Serial.printf("[uploader] Failed: error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return success;
}