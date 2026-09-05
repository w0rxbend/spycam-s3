#pragma once

#include <Arduino.h>
#include "CameraFrame.h"

class LatestFrameSlot {
public:
  LatestFrameSlot() = default;
  LatestFrameSlot(const LatestFrameSlot &) = delete;
  LatestFrameSlot &operator=(const LatestFrameSlot &) = delete;
  LatestFrameSlot(LatestFrameSlot &&) = delete;
  LatestFrameSlot &operator=(LatestFrameSlot &&) = delete;

  bool begin();
  // Takes ownership of the frame. Any frame the sender has not picked up yet is
  // dropped, so the slot always holds the newest frame and never a queue.
  void put(CameraFrame frame);
  // Returns an empty CameraFrame if no frame arrived within waitTicks.
  CameraFrame takeLatest(TickType_t waitTicks);

private:
  SemaphoreHandle_t mutex_ = nullptr;
  SemaphoreHandle_t frameReady_ = nullptr;
  CameraFrame frame_;
};
