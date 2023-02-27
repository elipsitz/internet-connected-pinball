// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// -------- //
// bus_data //
// -------- //

#define bus_data_wrap_target 0
#define bus_data_wrap 5

static const uint16_t bus_data_program_instructions[] = {
            //     .wrap_target
    0x20c4, //  0: wait   1 irq, 4                   
    0xa041, //  1: mov    y, x                       
    0xa020, //  2: mov    x, pins                    
    0x00c1, //  3: jmp    pin, 1                     
    0xa0c2, //  4: mov    isr, y                     
    0x8000, //  5: push   noblock                    
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program bus_data_program = {
    .instructions = bus_data_program_instructions,
    .length = 6,
    .origin = -1,
};

static inline pio_sm_config bus_data_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + bus_data_wrap_target, offset + bus_data_wrap);
    return c;
}

static inline void bus_data_program_init(
    PIO pio, uint sm, uint offset, uint pin_clk, uint pin_data_base
) {
    // Connect pins.
    const int pin_data_count = 18;
    pio_sm_set_consecutive_pindirs(pio, sm, pin_data_base, pin_data_count, false);
    pio_sm_config c = bus_data_program_get_default_config(offset);
    sm_config_set_jmp_pin(&c, pin_clk);
    sm_config_set_in_pins(&c, pin_data_base);
    // We only use the RX FIFO.
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    sm_config_set_in_shift(&c, false, false, 32);
    // Load config, jump to the start, and enable the state machine.
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

#endif