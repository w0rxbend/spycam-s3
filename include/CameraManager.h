#pragma once

#include "CameraFrame.h"

// There is exactly one camera peripheral on the board and esp_camera keeps all
// of its state internally, so there is nothing for an object to own here.
namespace camera_manager {

bool begin();
// Returns an empty CameraFrame when the sensor could not produce a frame.
CameraFrame capture();

} // namespace camera_manager
