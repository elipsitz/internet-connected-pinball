#include <cstdint>

#include <Arduino.h>
#include <HTTPClient.h>

#include "secrets.h"
#include "bus.h"
#include "game.h"

#define ADDR_FLAG_GAME_OVER (0xC9)
#define ADDR_BALL_IN_PLAY (0x58)
#define ADDR_PLAYER_UP (0xCD)

// Max snapshots: 5 balls + game over.
#define MAX_SNAPSHOTS 6
static uint8_t snapshot_data[BUS_MEMORY_LEN * MAX_SNAPSHOTS];
static size_t num_snapshots;

static uint8_t last_flag_game_over;
static uint8_t last_player_up;
static uint8_t last_ball_in_play;
static bool in_game;

bool upload_score();

void game_init()
{
    in_game = false;
    last_flag_game_over = 0xFF;
    last_player_up = 0xFF;
    last_ball_in_play = 0xFF;
}

void capture_snapshot()
{
    if (num_snapshots == MAX_SNAPSHOTS) {
        Serial.println("[game    ] Error, hit max snapshots");
        return;
    }
    memcpy(
        &snapshot_data[num_snapshots * BUS_MEMORY_LEN],
        const_cast<const uint8_t*>(bus_memory),
        BUS_MEMORY_LEN
    );
    num_snapshots += 1;
}

void game_check_state()
{
  uint8_t flag_game_over = bus_memory[ADDR_FLAG_GAME_OVER];
  uint8_t player_up = bus_memory[ADDR_PLAYER_UP];
  uint8_t ball_in_play = bus_memory[ADDR_BALL_IN_PLAY];

  if (flag_game_over != last_flag_game_over) {
    Serial.printf("[game    ] flag game over %d -> %d\n", last_flag_game_over, flag_game_over);

    if (flag_game_over == 0) {
      Serial.println("[game    ] Started new game.");
      in_game = true;
      num_snapshots = 0;
      capture_snapshot();
    }
    // If we move from flag 0...
    // or if we move *to* 2 (and we previously didn't think we were in a game,
    // indicating that we missed the game start).
    if (last_flag_game_over == 0 || (flag_game_over == 2 && !in_game)) {
      Serial.println("[game    ] Game over!");
      in_game = false;
      capture_snapshot();
      upload_score();
    }
  } else if (flag_game_over == 0) {
    // In a game.
    if (ball_in_play == 0xF1 && last_ball_in_play != 0xF1) {
        // We restarted the current game.
        Serial.println("[game    ] Restarted current game.");
        in_game = true;
        num_snapshots = 0;
        capture_snapshot();
    } else if (ball_in_play > last_ball_in_play) {
        Serial.println("[game    ] Next ball");
        capture_snapshot();
    }
  }

  last_flag_game_over = flag_game_over;
  last_player_up = player_up;
  last_ball_in_play = ball_in_play;
}

bool upload_score()
{
  // First construct payload using current snapshot of memory.
  // TODO send this as a stream rather than copying it.
  String payload = "{\"format\": \"williams-sys7-v2\"}";
  payload.concat((char)0);
  payload.concat(snapshot_data, num_snapshots * BUS_MEMORY_LEN);

  Serial.printf("[uploader] Uploading scores, size = %u\n", payload.length());
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