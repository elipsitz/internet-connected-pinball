#include <cstring>

#include <Arduino.h>

#include "bus.h"


uint32_t bus_count;
uint8_t bus_memory[BUS_MEMORY_LEN];

void bus_setup() {
    bus_count = 0;
    memset(bus_memory, 0xFF, BUS_MEMORY_LEN);
}

void bus_loop() {
    for (int i = 0; i < BUS_MEMORY_LEN; i++) {
        bus_memory[i] = i & 0xFF;
    }
    bus_memory[0xC9] = 0xFF;

    delay(3000);
    bus_memory[0xC9] = 0;
    delay(3000);
    bus_memory[0xC9] = 1;
    delay(3000);
}