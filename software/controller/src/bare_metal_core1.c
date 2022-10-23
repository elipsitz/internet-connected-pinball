#include <esp_attr.h>
#include <esp_log.h>

// We need to initialize Core 1 ("APP cpu") before Core 0's heap is
// initialized, otherwise there's heap corruption.
//
// ESP-IDF theoretically allows you to override start_cpu0 (a weak symbol)
// which would be the perfect place to call this code, but the
// start_cpu0_default function is static so we can't call it.
//
// Another approach involves wrapping the g_startup_fn symbol, which is
// an array with a pointer to the start_cpu0 function.
void start_cpu0(void);

void IRAM_ATTR
custom_start_cpu0(void)
{
    ESP_EARLY_LOGI("init", "Custom start_cpu0");
    start_cpu0();
}

typedef void (*sys_startup_fn_t)(void);
const DRAM_ATTR sys_startup_fn_t __wrap_g_startup_fn[1] = { custom_start_cpu0 };