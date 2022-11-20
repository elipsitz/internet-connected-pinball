#include <cstring>

#include <Arduino.h>

#include "bus.h"
#include "bus_control.pio.h"
#include "bus_data.pio.h"

#define PIN_DATA_BASE (0)
#define PIN_CLK (18)
#define PIN_WREN (19)

uint32_t bus_count;
uint8_t bus_memory[BUS_MEMORY_LEN];

void bus_run() {
    // Initialize memory.
    bus_count = 0;
    memset(bus_memory, 0xFF, BUS_MEMORY_LEN);

    // Initialize PIOs.
    PIO pio = pio0;
    uint control_offset = pio_add_program(pio, &bus_control_program);
    uint control_sm = pio_claim_unused_sm(pio, true);
    bus_control_program_init(pio, control_sm, control_offset, PIN_WREN);
    uint data_offset = pio_add_program(pio, &bus_data_program);
    uint data_sm = pio_claim_unused_sm(pio, true);
    bus_data_program_init(pio, data_sm, data_offset, PIN_CLK, PIN_DATA_BASE);

    // Read data from PIO loop.
    // TODO: use DMA so we don't miss anything.
    while (true) {
        uint32_t word = pio_sm_get_blocking(pio, data_sm);
        uint8_t data = word & 0xFF;
        uint16_t addr = (word >> 8) & 0x3FF;
        bus_memory[addr] = data;
        bus_count += 1;
    }
}
