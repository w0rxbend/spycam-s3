#pragma once

#include <stdint.h>

namespace timing {

// Fires at most once per intervalMs. The unsigned subtraction stays correct
// when the millisecond counter wraps (every ~49.7 days on ESP32).
class IntervalTimer {
public:
  IntervalTimer(uint32_t intervalMs, uint32_t nowMs) : intervalMs_(intervalMs), lastAt_(nowMs) {}

  bool due(uint32_t nowMs)
  {
    if (nowMs - lastAt_ < intervalMs_) {
      return false;
    }
    lastAt_ = nowMs;
    return true;
  }

private:
  uint32_t intervalMs_;
  uint32_t lastAt_;
};

// Exponential reconnect delay: returns minMs, then doubles each call, capped
// at maxMs. The comparison against maxMs / 2 avoids overflowing the doubling.
class Backoff {
public:
  Backoff(uint32_t minMs, uint32_t maxMs)
      : minMs_(minMs), maxMs_(maxMs < minMs ? minMs : maxMs), currentMs_(minMs) {}

  uint32_t nextDelayMs()
  {
    const uint32_t delayMs = currentMs_;
    currentMs_ = (currentMs_ > maxMs_ / 2) ? maxMs_ : currentMs_ * 2;
    return delayMs;
  }

  void reset() { currentMs_ = minMs_; }

private:
  uint32_t minMs_;
  uint32_t maxMs_;
  uint32_t currentMs_;
};

} // namespace timing
