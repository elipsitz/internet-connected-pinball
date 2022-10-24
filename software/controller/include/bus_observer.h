#pragma once

#include <stdatomic.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define PINBALL_MEMORY_LEN (1024)

typedef struct {
    // The memory.
    uint8_t memory[PINBALL_MEMORY_LEN];
} bus_observer_t;

/// Start the bus observer.
int bus_observer_start(bus_observer_t* observer);