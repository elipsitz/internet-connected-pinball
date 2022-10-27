#include <stdio.h>

#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "bus.pio.h"

/*
    26 available GPIO pins

    1 clock
    1 VMA
    (assume read/write is always 'write')

    8 data

    at least 11 for address. Ideally 13 or 14.

 */

int main() {
    set_sys_clock_khz(120000, true); // Clock at 120MHz
    stdio_init_all();

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &bus_program);
    uint sm = pio_claim_unused_sm(pio, true);
    bus_program_init(pio, sm, offset, PICO_DEFAULT_LED_PIN);

    sleep_ms(2000);
    pio_sm_put_blocking(pio, sm, 10000000);

    while (true) {
        sleep_ms(1000);
    }
}