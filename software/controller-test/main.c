#include <stdio.h>

#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "bus.pio.h"

#define PIN_ADDR_START (0)
#define PIN_CLK (20)
#define PIN_VMA (21)

static inline void do_write(PIO pio, uint sm, uint16_t addr, uint8_t data)
{
    uint32_t word = (((uint32_t)data) << 24) | (((uint32_t)addr) << 12) | ((uint32_t)addr);
    pio_sm_put_blocking(pio, sm, word);
}

int main()
{
    // Clock at 120MHz
    set_sys_clock_khz(120000, true);
    stdio_init_all();

    // Initialize PIO output.
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &bus_program);
    uint sm = pio_claim_unused_sm(pio, true);
    bus_program_init(pio, sm, offset, PIN_CLK, PIN_VMA, PIN_ADDR_START);

    sleep_ms(1000);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    for (uint i = 0; i < 1024; i++) {
        uint16_t addr = (uint16_t)i;
        // addr |= (1 << 11); // Make it out of the range.
        addr |= (1 << 10); // Make put it in the secondary range.
        do_write(pio, sm, addr, i & 0xFF);
    }
    
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    while (1) {
        sleep_ms(1000);
    }
}