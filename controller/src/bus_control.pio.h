// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ----------- //
// bus_control //
// ----------- //

#define bus_control_wrap_target 0
#define bus_control_wrap 4

static const uint16_t bus_control_program_instructions[] = {
            //     .wrap_target
    0x2012, //  0: wait   0 gpio, 18                 
    0x2092, //  1: wait   1 gpio, 18                 
    0x00c4, //  2: jmp    pin, 4                     
    0x0000, //  3: jmp    0                          
    0xc004, //  4: irq    nowait 4                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program bus_control_program = {
    .instructions = bus_control_program_instructions,
    .length = 5,
    .origin = -1,
};

static inline pio_sm_config bus_control_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + bus_control_wrap_target, offset + bus_control_wrap);
    return c;
}

static inline void bus_control_program_init(
    PIO pio, uint sm, uint offset, uint pin_wren
) {
    pio_sm_config c = bus_control_program_get_default_config(offset);
    sm_config_set_jmp_pin(&c, pin_wren);
    pio_sm_set_consecutive_pindirs(pio, sm, 18, 2, false);
    pio_gpio_init(pio, 18);
    pio_gpio_init(pio, 19);
    // Load config, jump to the start, and enable the state machine.
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

#endif
