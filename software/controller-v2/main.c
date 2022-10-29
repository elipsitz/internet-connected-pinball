#include <stdio.h>
#include <string.h>

#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "bus_control.pio.h"
#include "bus_data.pio.h"

#define PIN_DATA_BASE (0)
#define PIN_CLK (18)
#define PIN_WREN (19)

int main()
{
    stdio_init_all();

    printf("\n\n\nInit");
    sleep_ms(1000);
    printf(".");
    sleep_ms(1000);
    printf(".");
    sleep_ms(1000);
    printf(".");
    sleep_ms(1000);
    printf("\nGo!\n");

    // Initialize PIOs.
    PIO pio = pio0;
    uint control_offset = pio_add_program(pio, &bus_control_program);
    uint control_sm = pio_claim_unused_sm(pio, true);
    bus_control_program_init(pio, control_sm, control_offset, PIN_WREN);
    uint data_offset = pio_add_program(pio, &bus_data_program);
    uint data_sm = pio_claim_unused_sm(pio, true);
    bus_data_program_init(pio, data_sm, data_offset, PIN_CLK, PIN_DATA_BASE);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    uint64_t start = time_us_64();
    uint64_t end = start + 10000000;

    uint32_t arr[1024];
    memset(arr, 0, sizeof(arr));

    uint32_t total = 0;
    uint32_t sum = 0;
    while (time_us_64() < end) {
        // Instead of pio_sm_get_blocking, this does a nonblocking busy loop.
        uint32_t word = pio_sm_get(pio, data_sm);
        if ((pio->fdebug & 0x00000200) != 0) {
            pio->fdebug = 0x00000200;
            continue;
        }

        uint8_t data = word & 0xFF;
        uint16_t addr = (word >> 8) & 0x3FF;
        arr[addr] += 1;
        sum += data;
        total += 1;
    }

    printf("Total seen: %u\n", total);
    printf("Sum: %u\n", sum);

    int c = 0;
    for (int i = 0; i < 256 / 8; i++) {
        for (int j = 0; j < 8; j++) {
            printf("%X ", arr[c++]);
        }
        printf("\n");
    }
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    while (1) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(1000);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
    }
}