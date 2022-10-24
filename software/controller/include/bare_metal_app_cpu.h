#pragma once

/// The type of the app cpu main function.
typedef void (*app_cpu_main_fn_t)(void* context);

/// Start the app cpu with the given main function.
///
/// The main function and everything it calls and uses must be
/// in IRAM/DRAM.
///
/// Returns true if succesful.
bool start_app_cpu(app_cpu_main_fn_t main_fn, void* context);