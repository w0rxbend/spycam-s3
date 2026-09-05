#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_camera.h"

// Owns one frame buffer obtained from esp_camera_fb_get() and hands it back to
// the driver when it goes out of scope.
//
// The driver only ever lends out CAMERA_FB_COUNT buffers (2 on this board), so
// one buffer that is never returned wedges the capture pipeline permanently:
// esp_camera_fb_get() blocks forever and the stream stops. Making ownership a
// type instead of a convention means a future early return cannot leak a frame.
//
// There is deliberately no way to extract the raw camera_fb_t* again. Every
// call site either reads the pixels (data()/size()) or moves the whole handle
// along, so the single return-to-driver site below stays the only one.
class CameraFrame {
public:
  CameraFrame() noexcept : frame_(nullptr) {}

  explicit CameraFrame(camera_fb_t *frame) noexcept : frame_(frame) {}

  ~CameraFrame() { returnToDriver(); }

  CameraFrame(const CameraFrame &) = delete;
  CameraFrame &operator=(const CameraFrame &) = delete;

  // Written out by hand because the target core builds as -std=gnu++11, where a
  // user-declared destructor suppresses the implicit move operations.
  CameraFrame(CameraFrame &&other) noexcept : frame_(other.frame_)
  {
    other.frame_ = nullptr;
  }

  CameraFrame &operator=(CameraFrame &&other) noexcept
  {
    if (this != &other) {
      returnToDriver();
      frame_ = other.frame_;
      other.frame_ = nullptr;
    }
    return *this;
  }

  // True when this handle owns a frame buffer. Explicit so a CameraFrame cannot
  // silently convert to an int or to another CameraFrame's bool.
  explicit operator bool() const noexcept { return frame_ != nullptr; }

  const uint8_t *data() const noexcept { return frame_ != nullptr ? frame_->buf : nullptr; }

  size_t size() const noexcept { return frame_ != nullptr ? frame_->len : 0; }

private:
  void returnToDriver() noexcept
  {
    if (frame_ != nullptr) {
      esp_camera_fb_return(frame_);
      frame_ = nullptr;
    }
  }

  camera_fb_t *frame_;
};
