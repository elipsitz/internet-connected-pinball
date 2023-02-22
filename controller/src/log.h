#pragma once

#include <deque>

void log_init();
void log_log(const char* tag, const char* format, ...) __attribute__ ((format (printf, 2, 3)));
std::deque<String>& log_get_logs();