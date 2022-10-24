/*
 * MIT License
 *
 * Copyright (c) 2020 Daniel Frejek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Adapted from https://github.com/Winkelkatze/ESP32-Bare-Metal-AppCPU
// And https://github.com/darthcloud/esp32_baremetal_core1_bitbang_test

#include <stdatomic.h>
#include <esp_attr.h>
#include <esp_log.h>
#include <soc/dport_reg.h>
#include <soc/cpu.h>
#include <esp_heap_caps.h>
#include <esp32/rom/ets_sys.h>
#include <esp32/rom/uart.h>
#include <soc/soc_memory_layout.h>
#include "xt_instr_macros.h"
#include "xtensa/config/specreg.h"
#include <xtensa/xtensa_api.h>
#include <stdio.h>
#include "esp32/rom/cache.h"

#include "bare_metal_app_cpu.h"

#define TAG "bare_metal"

#define BAREMETAL_APP_CPU_DEBUG 0

// Reserve static data for built-in ROM functions
#define APP_CPU_RESERVE_ROM_DATA 1

#ifndef APP_CPU_STACK_SIZE
#define APP_CPU_STACK_SIZE 1024
#endif

#ifndef CONFIG_FREERTOS_UNICORE
#error Bare metal app core use requires UNICORE build
#endif

// Interrupt vector, this comes from the SDK
extern int _vector_table;

static volatile uint32_t app_cpu_stack_ptr;
static volatile uint8_t app_cpu_initial_start;

#ifdef APP_CPU_RESERVE_ROM_DATA
SOC_RESERVE_MEMORY_REGION(0x3ffe3f20, 0x3ffe4350, rom_app_data);
#endif

static atomic_uintptr_t app_cpu_main_fn_ptr = (uintptr_t)NULL;
static atomic_uintptr_t app_cpu_main_context = (uintptr_t)NULL;

static void IRAM_ATTR app_cpu_main_trampoline()
{
    app_cpu_main_fn_t main_fn = (app_cpu_main_fn_t)app_cpu_main_fn_ptr;
    void* context = (void*)app_cpu_main_context;
    main_fn(context);
}

// The main MUST NOT be inlined!
// Otherwise, it will cause mayhem on the stack since we are modifying the stack pointer before the main is called.
// Having a volatile pointer around should force the compiler to behave.
static void (*volatile app_cpu_main_ptr)() = &app_cpu_main_trampoline;

static inline void cpu_write_dtlb(uint32_t vpn, unsigned attr)
{
    asm volatile("wdtlb  %1, %0; dsync\n" ::"r"(vpn), "r"(attr));
}

static inline void cpu_write_itlb(unsigned vpn, unsigned attr)
{
    asm volatile("witlb  %1, %0; isync\n" ::"r"(vpn), "r"(attr));
}

static inline void cpu_configure_region_protection()
{
    const uint32_t pages_to_protect[] = {0x00000000, 0x80000000, 0xa0000000, 0xc0000000, 0xe0000000};
    for (int i = 0; i < sizeof(pages_to_protect) / sizeof(pages_to_protect[0]); ++i)
    {
        cpu_write_dtlb(pages_to_protect[i], 0xf);
        cpu_write_itlb(pages_to_protect[i], 0xf);
    }
    cpu_write_dtlb(0x20000000, 0);
    cpu_write_itlb(0x20000000, 0);
}

/*
 * This is the entry point for the APP CPU.
 * When the CPU is enabled the first time, we just "warm up" things, so we do some initialization before turning the clock off again.
 * Then the start function will load the real stack pointer and switch the CPU on again.
 */
static void IRAM_ATTR app_cpu_init()
{
    // init interrupt handler
    cpu_hal_set_vecbase(&_vector_table);

    /* New cpu_configure_region_protection within bootloader_init_mem fail */
    /* Use version from v4.0.2 here and call cpu_init_memctl replacement */
    cpu_configure_region_protection();
    cpu_hal_init_hwloop();

#ifdef APP_CPU_RESERVE_ROM_DATA
    uartAttach();
    ets_install_uart_printf();
    uart_tx_switch(CONFIG_ESP_CONSOLE_UART_NUM);
#endif

    app_cpu_initial_start = 1;
    while (1)
    {
        // This will halt the CPU until it is needed
        DPORT_REG_CLR_BIT(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN);

        // clock cpu will still execute 1 instruction after the clock gate is turned off.
        // so just have some NOPs here, so the stack pointer will be correct
        asm volatile(
            "nop\n"
            "nop\n"
            "nop\n"
            "nop\n"
            "nop\n");

        // load the new stack pointer for our main
        // this is VERY important, since the initial stack pointer now points somewhere in the heap!
        asm volatile(
            "l32i a1, %0, 0\n" ::"r"(&app_cpu_stack_ptr));

        // Finally call the main.
        // Its imperative for the main to be NOT INLINED!
        (app_cpu_main_trampoline)();
    }
}

bool start_app_cpu(app_cpu_main_fn_t main_fn, void* context)
{
#if BAREMETAL_APP_CPU_DEBUG
    // printf("App main at %08X\n", (uint32_t)&app_cpu_main);
    printf("App init at %08X\n", (uint32_t)&app_cpu_init);
    printf("APP_CPU RESET: %u\n", DPORT_READ_PERI_REG(DPORT_APPCPU_CTRL_A_REG));
    printf("APP_CPU CLKGATE: %u\n", DPORT_READ_PERI_REG(DPORT_APPCPU_CTRL_B_REG));
    printf("APP_CPU STALL: %u\n", DPORT_READ_PERI_REG(DPORT_APPCPU_CTRL_C_REG));
    printf("APP_CACHE_CTRL: %08x\n", DPORT_READ_PERI_REG(DPORT_APP_CACHE_CTRL_REG));
#endif

    if (!app_cpu_initial_start)
    {
        printf("APP CPU was not initialized!\n");
        return false;
    }

    if (DPORT_REG_GET_BIT(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN))
    {
        printf("APP CPU is already running!\n");
        return false;
    }

    app_cpu_main_fn_ptr = (uintptr_t)main_fn;
    app_cpu_main_context = (void*)context;

    if (!app_cpu_stack_ptr)
    {
        // We need to allocate the stack for the APP CPU here, since the original stack from the time when the APP CPU was initialized is now some part of the heap.
        app_cpu_stack_ptr = (uint32_t)heap_caps_malloc(APP_CPU_STACK_SIZE, MALLOC_CAP_DMA);

        // Don't forget to set the stack ptr to the end of the segment
        app_cpu_stack_ptr += APP_CPU_STACK_SIZE - sizeof(size_t);
    }

#if BAREMETAL_APP_CPU_DEBUG
    printf("APP CPU STACK PTR: %08X\n", (uint32_t)app_cpu_stack_ptr);
#endif

    DPORT_SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN);
    return true;
}

/// Initializes the app cpu. Must be called before Core 0 heap is initialized.
static void init_app_cpu_baremetal()
{
    // just in case...
    // disable the clock gate of the app core
    DPORT_REG_CLR_BIT(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN);

    app_cpu_initial_start = 0;

    // disable all interrupts
    // should be disabled by default anyway, but in case we have a bootloader, we don't know what it has already done
    for (int i = ETS_WIFI_MAC_INTR_SOURCE; i <= ETS_CACHE_IA_INTR_SOURCE; i++)
    {
        intr_matrix_set(1, i, ETS_INVALID_INUM);
    }

    // Reset the CPU
    DPORT_REG_SET_BIT(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING);
    DPORT_REG_CLR_BIT(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING);

    // Load the entry vector
    DPORT_WRITE_PERI_REG(DPORT_APPCPU_CTRL_D_REG, ((uint32_t)&app_cpu_init));

    // And turn the clock on
    DPORT_SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN);

    // finally wait for the CPU to start
    while (!app_cpu_initial_start)
    {
    }
}

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

static void IRAM_ATTR
custom_start_cpu0(void)
{
    ESP_EARLY_LOGI(TAG, "Initializing app CPU");
    init_app_cpu_baremetal();
    ESP_EARLY_LOGI(TAG, "Done initializing app CPU");
    start_cpu0();
}

typedef void (*sys_startup_fn_t)(void);
const DRAM_ATTR sys_startup_fn_t __wrap_g_startup_fn[1] = {custom_start_cpu0};