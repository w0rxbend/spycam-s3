#include "SerialLog.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace {

serial_log::Level currentLevel = serial_log::Level::Info;

// Longer log lines are cut to fit; logMessage() marks a cut line so a
// truncated diagnostic is not mistaken for a complete one.
constexpr size_t kMaxLogMessageLen = 192;

const char *levelName(serial_log::Level level)
{
  switch (level) {
    case serial_log::Level::Error:
      return "ERROR";
    case serial_log::Level::Warn:
      return "WARN";
    case serial_log::Level::Info:
      return "INFO";
    case serial_log::Level::Debug:
      return "DEBUG";
  }
  return "?";
}

void logMessage(serial_log::Level level, const char *format, va_list args)
{
  if (!serial_log::enabled(level)) {
    return;
  }

  char message[kMaxLogMessageLen];
  const int written = vsnprintf(message, sizeof(message), format, args);
  if (written >= static_cast<int>(sizeof(message))) {
    static const char kTruncationMarker[] = "...[cut]";
    memcpy(message + sizeof(message) - sizeof(kTruncationMarker),
           kTruncationMarker,
           sizeof(kTruncationMarker));
  }
  Serial.printf("[%10lu] %-5s %s\n",
                static_cast<unsigned long>(millis()),
                levelName(level),
                message);
}

} // namespace

namespace serial_log {

void begin(uint32_t baud, Level level)
{
  currentLevel = level;
  Serial.begin(baud);
  delay(200);
  Serial.println("--- Serial log started ---");
}

void setLevel(Level level)
{
  currentLevel = level;
}

bool enabled(Level level)
{
  return levelEnabled(currentLevel, level);
}

void error(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  logMessage(Level::Error, format, args);
  va_end(args);
}

void warn(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  logMessage(Level::Warn, format, args);
  va_end(args);
}

void info(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  logMessage(Level::Info, format, args);
  va_end(args);
}

void debug(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  logMessage(Level::Debug, format, args);
  va_end(args);
}

} // namespace serial_log
