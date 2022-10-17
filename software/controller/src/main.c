#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>

#include "bus_observer.h"

// GPIO with the built-in LED.
#define GPIO_LED GPIO_NUM_2

static const char* TAG = "main";

static bus_observer_t DRAM_ATTR bus_observer;

void
app_main() {
    ESP_LOGI(TAG, "Initializing");
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_LED, true);

    // Initialize NVS and WiFi (before bus observer disables interrupts).
    nvs_flash_init();
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_init_config);

    // Start the bus observer.
    ESP_LOGI(TAG, "Starting bus observer.");
    int result = bus_observer_start(&bus_observer);
    if (result != 0) {
        ESP_LOGE(TAG, "Error initializing bus observer");
    }

    // Start WiFi.
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_start();
    
    // Initialization complete, turn off LED.
    gpio_set_level(GPIO_LED, false);
    ESP_LOGI(TAG, "Initialization complete");

    while (1) {
        ESP_LOGI(TAG, "Alive!");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}