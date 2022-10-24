#pragma once

#include <stdatomic.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

typedef struct {
    // The memory.
    uint8_t memory[1024];
} bus_observer_t;

/// Start the bus observer.
int bus_observer_start(bus_observer_t* observer);