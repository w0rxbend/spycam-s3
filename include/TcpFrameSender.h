#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Timing.h>

#include "CameraFrame.h"

class TcpFrameSender {
public:
  TcpFrameSender(const char *host, uint16_t port);
  TcpFrameSender(const TcpFrameSender &) = delete;
  TcpFrameSender &operator=(const TcpFrameSender &) = delete;
  TcpFrameSender(TcpFrameSender &&) = delete;
  TcpFrameSender &operator=(TcpFrameSender &&) = delete;

  void begin();
  bool ensureConnected();
  // Sends one frame over the already-open connection. The caller must have
  // called ensureConnected() successfully first. The frame is borrowed, not
  // owned; the caller's CameraFrame keeps it alive for the whole call.
  bool sendFrame(const CameraFrame &frame);
  void disconnect();

private:
  bool ensureWifiConnected();
  bool ensureTcpConnected();
  bool sendAll(const uint8_t *data, size_t len);
  void waitBackoff();
  void resetBackoff();

  const char *host_;
  uint16_t port_;
  WiFiClient client_;
  uint32_t sequence_;
  timing::Backoff backoff_;
  uint32_t sentFrames_;
  uint32_t failedSends_;
  timing::IntervalTimer statusLog_;
};
