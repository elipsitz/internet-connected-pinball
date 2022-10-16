#pragma once

#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

typedef struct {
    // The memory.
    uint8_t memory[1024];

    // The task handle for the bus observer.
    TaskHandle_t task_handle;
} bus_observer_t;

/// Start the bus observer.
int bus_observer_start(bus_observer_t* observer);