#pragma once

#include <Arduino.h>
#include <LogLevel.h>

namespace serial_log {

void begin(uint32_t baud, Level level);
void setLevel(Level level);
bool enabled(Level level);

void error(const char *format, ...) __attribute__((format(printf, 1, 2)));
void warn(const char *format, ...) __attribute__((format(printf, 1, 2)));
void info(const char *format, ...) __attribute__((format(printf, 1, 2)));
void debug(const char *format, ...) __attribute__((format(printf, 1, 2)));

} // namespace serial_log
