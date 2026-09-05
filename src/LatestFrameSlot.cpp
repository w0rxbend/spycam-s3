#include "LatestFrameSlot.h"

#include <utility>

#include "SerialLog.h"

bool LatestFrameSlot::begin()
{
  mutex_ = xSemaphoreCreateMutex();
  frameReady_ = xSemaphoreCreateBinary();
  return mutex_ != nullptr && frameReady_ != nullptr;
}

void LatestFrameSlot::put(CameraFrame frame)
{
  if (!frame) {
    return;
  }

  if (mutex_ == nullptr || frameReady_ == nullptr) {
    serial_log::warn("Dropping frame because latest-frame slot is not initialized");
    return;
  }

  {
    // Declared outside the locked region on purpose: handing a buffer back to
    // the camera driver is a driver call, and the capture task must not make it
    // while the sender task could be blocked on this mutex. Moving the stale
    // frame out here means the slot swap does no work beyond two pointer
    // assignments, and the release happens when staleFrame dies below.
    CameraFrame staleFrame;

    xSemaphoreTake(mutex_, portMAX_DELAY);
    staleFrame = std::move(frame_);
    frame_ = std::move(frame);
    xSemaphoreGive(mutex_);

    if (staleFrame) {
      serial_log::debug("Dropping stale frame before sender consumed it");
    }
  }

  xSemaphoreGive(frameReady_);
}

CameraFrame LatestFrameSlot::takeLatest(TickType_t waitTicks)
{
  if (mutex_ == nullptr || frameReady_ == nullptr) {
    return CameraFrame();
  }

  const TickType_t startedAt = xTaskGetTickCount();

  for (;;) {
    const TickType_t elapsed = xTaskGetTickCount() - startedAt;
    if (elapsed >= waitTicks) {
      return CameraFrame();
    }

    if (xSemaphoreTake(frameReady_, waitTicks - elapsed) != pdTRUE) {
      return CameraFrame();
    }

    CameraFrame frame;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    frame = std::move(frame_);
    xSemaphoreGive(mutex_);

    if (frame) {
      return frame;
    }
  }
}
