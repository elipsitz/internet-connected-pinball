#include <cstdint>

#include <Arduino.h>
#include <HTTPClient.h>
#include <LittleFS.h>

#include "secrets.h"
#include "bus.h"
#include "game.h"

#define ADDR_FLAG_GAME_OVER (0xC9)
#define ADDR_BALL_IN_PLAY (0x58)
#define ADDR_PLAYER_UP (0xCD)

#define STORAGE_CHECK_PERIOD (60 * 1000)
String snapshot_directory = "/snapshots";
static uint32_t last_storage_check = 0;

// Max snapshots: 5 balls + game over.
#define MAX_SNAPSHOTS 6
static uint8_t snapshot_data[BUS_MEMORY_LEN * MAX_SNAPSHOTS];
static size_t num_snapshots;

static uint8_t last_flag_game_over;
static uint8_t last_player_up;
static uint8_t last_ball_in_play;
static bool in_game;

bool upload_score();
bool upload_score_with_payload(String& payload);
void storage_init();
void storage_save(String& payload);
void storage_try_upload();

void game_init()
{
    in_game = false;
    last_flag_game_over = 0xFF;
    last_player_up = 0xFF;
    last_ball_in_play = 0xFF;

    storage_init();
    num_snapshots = 0;
}

// Reads the rest of the content in the file to the string.
// Like File::readString, except with proper handling with null bytes.
String read_file_to_string(File& f)
{
  String ret;
  ret.reserve(f.size() - f.position());
  uint8_t buffer[256];
  int read;
  do {
    read = f.read(buffer, sizeof(buffer));
    ret.concat(buffer, read);
  } while (read > 0);
  return ret;
}

void storage_init()
{
  LittleFS.begin();
  if (!LittleFS.exists(snapshot_directory)) {
    if (!LittleFS.mkdir(snapshot_directory)) {
      Serial.println("[storage ] Failed to make snapshot directory");
      return;
    }
  }

  // BUG: For some reason, trying to upload right after reboot doesn't work.
  // storage_try_upload();
}

// Check for stored game data and attempt to upload it.
void storage_try_upload()
{
  bool slept = false;
  auto dir = LittleFS.openDir(snapshot_directory);
  while (dir.next()) {
    size_t file_size = dir.fileSize();
    Serial.printf("[storage ] Found snapshot %s, size %zu\n", dir.fileName().c_str(), file_size);
    if (file_size > 0) {
      File f = dir.openFile("r");
      String payload = read_file_to_string(f);
      f.close();

      if (!slept) {
        delay(2000);
        slept = true;
      }
      bool success = upload_score_with_payload(payload);
      if (success) {
        Serial.println("[storage ] Upload successful");
        String path = snapshot_directory;
        path += "/";
        path += dir.fileName();
        if (!LittleFS.remove(path)) {
          Serial.println("[storage ] Failed to delete file");
        }
      } else {
        Serial.println("[storage ] Upload failed, aborting.");
        // No point of trying the rest of them...
        return;
      }
    }
  }
}

void storage_save(String& payload)
{
  String path = snapshot_directory;
  path += "/";
  path += rp2040.hwrand32();
  File f = LittleFS.open(path, "w");
  if (!f) {
    Serial.println("[storage ] File open FAILED!");
    return;
  }
  size_t written = f.write(payload.begin(), payload.length());
  f.close();
  if (written == payload.length()) {
    Serial.println("[storage ] File saved successfully.");
  } else {
    Serial.printf("[storage ] Failed to write whole file (wrote %zu)\n", written);
    LittleFS.remove(path);
  }
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
    // If we move from flag 0.
    if (last_flag_game_over == 0) {
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

  if (millis() - last_storage_check > STORAGE_CHECK_PERIOD) {
    storage_try_upload();
    last_storage_check = millis();
  }
}

bool upload_score()
{
  // First construct payload using current snapshot of memory.
  // TODO send this as a stream rather than copying it.
  String payload = "{\"format\": \"williams-sys7-v2\"}";
  payload.concat((char)0);
  payload.concat(snapshot_data, num_snapshots * BUS_MEMORY_LEN);
  bool success = upload_score_with_payload(payload);

  if (!success) {
    Serial.println("[uploader ] Upload failed, saving to storage");
    storage_save(payload);
  }
  return success;
}

bool upload_score_with_payload(String& payload)
{
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