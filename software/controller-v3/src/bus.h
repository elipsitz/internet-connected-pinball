#pragma once

#include <cstdint>

#define BUS_MEMORY_LEN (1024)

// rp2040's M0+ cores do not actually have atomics.
// This should be okay though, since we're only reading from
// one core, and writing from the other.
extern uint32_t bus_count;
extern uint8_t bus_memory[BUS_MEMORY_LEN];

void bus_run();