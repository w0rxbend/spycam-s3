#pragma once

#include <stdint.h>

namespace serial_log {

// Ordered from least to most verbose. The numeric values are the ordering, so
// enabling one level implies enabling everything above it in this list.
enum class Level : uint8_t {
  Error = 0,
  Warn = 1,
  Info = 2,
  Debug = 3,
};

// True when a message logged at `message` should be printed while the logger is
// configured at `configured`.
//
// Kept apart from the rest of serial_log because this is the only part of the
// logger that is pure arithmetic: no Serial, no millis(), nothing from Arduino.
// Split out, the host test environment can prove the ordering for every pair of
// levels instead of the rule being verified by squinting at the device output.
inline bool levelEnabled(Level configured, Level message)
{
  return static_cast<uint8_t>(message) <= static_cast<uint8_t>(configured);
}

} // namespace serial_log
