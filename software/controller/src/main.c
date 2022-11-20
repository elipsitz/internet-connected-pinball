#include <driver/gpio.h>
#include <esp_event.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>

#include "bus_observer.h"
#include "wifi.h"
#include "ui.h"

// GPIO with the built-in LED.
#define GPIO_LED GPIO_NUM_2

static const char* TAG = "main";

static bus_observer_t DRAM_ATTR bus_observer;

void
app_main(void) {
    ESP_LOGI(TAG, "Initializing");
    ESP_ERROR_CHECK(gpio_reset_pin(GPIO_LED));
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_LED, true));

    // Start the bus observer.
    ESP_LOGI(TAG, "Starting bus observer.");
    int result = bus_observer_start(&bus_observer);
    if (result != 0) {
        ESP_LOGE(TAG, "Error initializing bus observer");
    }

    // Initialize non-volatile storage.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Start wifi and web UI.
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init();
    ui_init(&bus_observer);

    // TODO set up memory poller / HTTP uploader.
    // TODO enable OTA.
    
    // Initialization complete, turn off LED.
    ESP_ERROR_CHECK(gpio_set_level(GPIO_LED, false));
    ESP_LOGI(TAG, "Initialization complete");
    
    uint32_t prev_count = 0;
    while (1) {
        uint32_t new_count = bus_observer.count;
        ESP_LOGI(TAG, "Alive! %d", new_count - prev_count);
        prev_count = new_count;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}