#include <string.h>

#include <driver/gpio.h>
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>

#include "bare_metal_app_cpu.h"
#include "bus_observer.h"

/// GPIO controlling ~OE.
#define GPIO_OE  GPIO_NUM_4

typedef struct {
    uint32_t value;
} reg0_t;

typedef struct {
    uint32_t value;
} reg1_t;

static inline reg0_t IRAM_ATTR
read_reg0(void) {
    return (reg0_t) { REG_READ(GPIO_IN_REG) };
}

static inline reg1_t IRAM_ATTR
read_reg1(void) {
    return (reg1_t) { REG_READ(GPIO_IN1_REG) };
}

static inline bool IRAM_ATTR
get_clock(reg1_t reg1) {
    return reg1.value & (1 << 7);
}

static inline bool IRAM_ATTR
get_wren(reg0_t reg0) {
    return reg0.value & (1 << 5);
}

static inline uint16_t IRAM_ATTR
get_address(reg0_t reg0, reg1_t reg1) {
    return
        ((reg0.value >> (22 - 0))      & 0b0000000011) |  // A0, A1
        ((reg0.value >> (25 - 2))      & 0b0000011100) | // A2, A3, A4
        ((reg1.value << 5 /* 0 - 5 */) & 0b1111100000); // A5, A6, A7, A8, A9
}

static inline uint8_t IRAM_ATTR
get_data(reg0_t reg0) {
    return
        ((reg0.value >> (13 - 0)) & 0b01111111) |  // D0, D1, D2, D3, D4, D5, D6
        ((reg0.value >> (21 - 7)) & 0b10000000);   // A7
}


/// GPIO Pin mapping:
///
/// WREN -> 5
/// CLK  -> 39
/// A0   -> 22
/// A1   -> 23
/// A2   -> 25
/// A3   -> 26
/// A4   -> 27
/// A5   -> 32
/// A6   -> 33
/// A7   -> 34
/// A8   -> 35
/// A9   -> 36
/// D0   -> 13
/// D1   -> 14
/// D2   -> 15
/// D3   -> 16
/// D4   -> 17
/// D5   -> 18
/// D6   -> 19
/// D7   -> 21

static DRAM_ATTR char app_cpu_init_log[] = "Running bus observer on app cpu!\n";

static void IRAM_ATTR
bus_observer_task(void *context)
{
    ets_printf(app_cpu_init_log);

    bus_observer_t *observer = context;
    portDISABLE_INTERRUPTS();

    while (true) {
        // Wait for clock to go high.
        while (!get_clock(read_reg1()))
            ;

        // WREN and address should be valid.
        if (!get_wren(read_reg0())) {
            // No write. Wait for clock to go low.
            while (get_clock(read_reg1()))
                ;
            continue;
        }

        // Read reg0 and reg1, waiting for clock to go low.
        // We're trying to capture the last value of the GPIOs before clock goes low.
        reg0_t reg0 = {};
        reg1_t reg1 = {};
        while (true) {
            reg0_t reg0_new = read_reg0();
            reg1_t reg1_new = read_reg1();

            if (get_clock(reg1_new)) {
                // Clock is still high, so these are valid reads.
                reg0 = reg0_new;
                reg1 = reg1_new;
            } else {
                // Clock went low. Last read cycle is the final data.
                break;
            }
        }
        uint16_t address = get_address(reg0, reg1);
        uint8_t data = get_data(reg0);

        // Save it, and do some sort of sync?
        atomic_fetch_add_explicit(&observer->count, 1, memory_order_relaxed);
        atomic_store_explicit(&observer->memory[address], data, memory_order_relaxed);
    }
}

/// Start the bus observer.
int bus_observer_start(bus_observer_t *observer)
{
    // Initialize bus observer struct.
    memset(observer->memory, 0xFF, PINBALL_MEMORY_LEN);

    // Initialize all of the GPIO pins.
    int gpios[] = {5, 39, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 13, 14, 15, 16, 17, 18, 19, 21};
    int num_gpios = sizeof(gpios) / sizeof(gpios[0]);
    for (int i = 0; i < num_gpios; i += 1) {
        gpio_reset_pin(gpios[i]);
        gpio_set_direction(gpios[i], GPIO_MODE_INPUT);
    }

    // Set output enable active (low).
    gpio_reset_pin(GPIO_OE);
    gpio_set_direction(GPIO_OE, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_OE, false);

    // Start up the second cpu.
    if (!start_app_cpu(bus_observer_task, (void*)observer)) {
        return -1;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    return 0;
}