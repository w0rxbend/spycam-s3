#include "CameraManager.h"

#include "AppConfig.h"
#include "CameraPins.h"
#include "SerialLog.h"

namespace {

camera_config_t makeCameraConfig(bool hasPsram)
{
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = camera_pins::Y2_GPIO_NUM;
  config.pin_d1 = camera_pins::Y3_GPIO_NUM;
  config.pin_d2 = camera_pins::Y4_GPIO_NUM;
  config.pin_d3 = camera_pins::Y5_GPIO_NUM;
  config.pin_d4 = camera_pins::Y6_GPIO_NUM;
  config.pin_d5 = camera_pins::Y7_GPIO_NUM;
  config.pin_d6 = camera_pins::Y8_GPIO_NUM;
  config.pin_d7 = camera_pins::Y9_GPIO_NUM;
  config.pin_xclk = camera_pins::XCLK_GPIO_NUM;
  config.pin_pclk = camera_pins::PCLK_GPIO_NUM;
  config.pin_vsync = camera_pins::VSYNC_GPIO_NUM;
  config.pin_href = camera_pins::HREF_GPIO_NUM;
  config.pin_sccb_sda = camera_pins::SIOD_GPIO_NUM;
  config.pin_sccb_scl = camera_pins::SIOC_GPIO_NUM;
  config.pin_pwdn = camera_pins::PWDN_GPIO_NUM;
  config.pin_reset = camera_pins::RESET_GPIO_NUM;
  config.xclk_freq_hz = app_config::CAMERA_XCLK_FREQ_HZ;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = hasPsram ? app_config::CAMERA_FRAME_SIZE : app_config::CAMERA_FRAME_SIZE_NO_PSRAM;
  config.jpeg_quality = app_config::CAMERA_JPEG_QUALITY;
  config.fb_count = hasPsram ? app_config::CAMERA_FB_COUNT : app_config::CAMERA_FB_COUNT_NO_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = hasPsram ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  return config;
}

// One format string for every stage, so a new field can never be added to
// half of the camera log lines.
void logCameraStatus(const char *stage, const camera_config_t &config, bool hasPsram)
{
  serial_log::info("Camera %s: frame_size=%d quality=%d fb_count=%d psram=%s",
                   stage,
                   static_cast<int>(config.frame_size),
                   config.jpeg_quality,
                   config.fb_count,
                   hasPsram ? "yes" : "no");
}

void applyRotation(sensor_t *sensor, app_config::CameraRotation rotation)
{
  const bool flipVertically = rotation == app_config::CameraRotation::FlipV ||
                              rotation == app_config::CameraRotation::Rotate180;
  const bool flipHorizontally = rotation == app_config::CameraRotation::FlipH ||
                                rotation == app_config::CameraRotation::Rotate180;
  sensor->set_vflip(sensor, flipVertically ? 1 : 0);
  sensor->set_hmirror(sensor, flipHorizontally ? 1 : 0);
}

}  // namespace

namespace camera_manager {

bool begin()
{
  serial_log::info("Initializing camera");

  const bool hasPsram = psramFound();
  const camera_config_t config = makeCameraConfig(hasPsram);
  logCameraStatus("config", config, hasPsram);

  const esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    serial_log::error("Camera init failed: 0x%x", static_cast<unsigned>(err));
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != nullptr) {
    sensor->set_framesize(sensor, config.frame_size);
    sensor->set_quality(sensor, app_config::CAMERA_JPEG_QUALITY);
    applyRotation(sensor, app_config::CAMERA_ROTATION);
  }

  logCameraStatus("ready", config, hasPsram);
  return true;
}

CameraFrame capture()
{
  return CameraFrame(esp_camera_fb_get());
}

} // namespace camera_manager
