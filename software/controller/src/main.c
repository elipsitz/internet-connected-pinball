#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define GPIO_LED GPIO_NUM_2

void app_main() {
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    bool led_state = false;
    while (true) {
        gpio_set_level(GPIO_LED, led_state);
        led_state = !led_state;

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}