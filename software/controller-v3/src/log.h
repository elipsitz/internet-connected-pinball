#pragma once

void log_init();
void log_log(const char* tag, const char* format, ...) __attribute__ ((format (printf, 2, 3)));
