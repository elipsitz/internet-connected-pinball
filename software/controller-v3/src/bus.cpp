#include <cstring>

#include <Arduino.h>
#include <hardware/gpio.h>
#include <hardware/structs/bus_ctrl.h>

#include "bus.h"
#include "bus_control.pio.h"
#include "bus_data.pio.h"

#define PIN_DATA_BASE (0)
#define PIN_CLK (18)
#define PIN_WREN (19)
#define PIN_OE (20)

volatile uint32_t bus_count;
volatile uint8_t bus_memory[BUS_MEMORY_LEN];

String logs;

void bus_run() {
    // // Initialize memory.
    bus_count = 0;
    memset(const_cast<uint8_t*>(bus_memory), 0xFF, BUS_MEMORY_LEN);

    // // Give this core bus priority.
    bus_ctrl_hw->priority |= 0x00000010;

    bus_count = 4;
    // // Initialize PIOs.
    PIO pio = pio0;
    uint control_offset = pio_add_program(pio, &bus_control_program);
    bus_count = 5;
    uint control_sm = pio_claim_unused_sm(pio, true);
    bus_control_program_init(pio, control_sm, control_offset, PIN_WREN);
    uint data_offset = pio_add_program(pio, &bus_data_program);
    bus_count = 6;
    uint data_sm = pio_claim_unused_sm(pio, true);
    bus_data_program_init(pio, data_sm, data_offset, PIN_CLK, PIN_DATA_BASE);
    bus_count = 7;

    // Set output enable (active low).
    gpio_init(PIN_OE);
    gpio_set_dir(PIN_OE, GPIO_OUT);
    gpio_put(PIN_OE, 0);

    bus_count += 1;


    // uint32_t prev = 0;
    // while (true) {
    //     pio_sm_exec_wait_blocking(pio, data_sm, pio_encode_push(false, true));
    //     uint32_t next = pio_sm_get_blocking(pio, data_sm);
    //     bus_count = prev - next;

    //     prev = next;

    //     sleep_ms(1000);
    // }

    bus_count = 0;

    // Read data from PIO loop.
    // TODO: use DMA so we don't miss anything.
    while (true) {
        //pio_sm_exec_wait_blocking(pio, data_sm, pio_encode_push(false, true));
        uint32_t word = pio_sm_get_blocking(pio, data_sm);
        bus_count = word;
        uint8_t data = word & 0xFF;
        uint16_t addr = (word >> 8) & 0x3FF;
        bus_memory[addr] = data;
        //bus_count += 1;
    }
}
