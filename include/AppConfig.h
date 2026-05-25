#pragma once

#include <Arduino.h>
#include "SerialLog.h"
#include "esp_camera.h"

#if __has_include("credentials.h")
#include "credentials.h"
#endif

/*
 * Performance and throttling guide
 *
 * If the ESP32-S3 overheats, throttles, drops frames, disconnects from WiFi, or
 * resets under load, reduce the camera workload in this order:
 *
 * 1. Lower TARGET_FPS. This directly increases FRAME_INTERVAL_MS and gives the
 *    CPU, camera driver, WiFi stack, and TCP sender more idle time between
 *    frames. Example: try TARGET_FPS values like 8, 5, or 3.
 * 2. Lower CAMERA_FRAME_SIZE. VGA costs much more CPU, RAM, and network
 *    bandwidth than QVGA. Use FRAMESIZE_QVGA when stability matters more than
 *    detail.
 * 3. Increase CAMERA_JPEG_QUALITY. In the ESP camera driver, a larger quality
 *    number means stronger compression and smaller frames, but lower image
 *    quality. Try 18-24 for a lighter stream.
 * 4. Reduce CAMERA_FB_COUNT to 1. Fewer frame buffers lower memory pressure and
 *    buffering latency, at the cost of less capture pipeline overlap.
 * 5. Lower CAMERA_XCLK_FREQ_HZ only if the camera remains unstable after the
 *    steps above. A lower clock can reduce sensor/capture pressure, but may
 *    require testing because some camera modules are pickier about XCLK values.
 *
 * For boards without PSRAM, the *_NO_PSRAM values are used automatically. Keep
 * those settings conservative: QVGA, one frame buffer, and moderate JPEG
 * compression are much easier on internal RAM.
 */

namespace app_config_credentials {

// Possible values: any WiFi SSID string from credentials.h, or the placeholder
// "<SSID>". This is copied before the WIFI_SSID macro is undefined below.
#ifdef WIFI_SSID
constexpr const char *WIFI_SSID_VALUE = WIFI_SSID;
#else
constexpr const char *WIFI_SSID_VALUE = "<SSID>";
#endif

// Possible values: any WiFi password string from credentials.h, an empty string
// for open networks, or the placeholder "<PASSWORD>".
#ifdef WIFI_PASSWORD
constexpr const char *WIFI_PASSWORD_VALUE = WIFI_PASSWORD;
#else
constexpr const char *WIFI_PASSWORD_VALUE = "<PASSWORD>";
#endif

} // namespace app_config_credentials

#ifdef WIFI_SSID
#undef WIFI_SSID
#endif

#ifdef WIFI_PASSWORD
#undef WIFI_PASSWORD
#endif

namespace app_config {

// Possible values: CameraRotation::None, CameraRotation::FlipV,
// CameraRotation::FlipH, or CameraRotation::Rotate180. These map to the
// sensor's vertical and horizontal mirror/flip settings after camera startup.
enum class CameraRotation : uint8_t {
  // Leave the image exactly as the sensor captures it.
  None      = 0,
  // Flip the image vertically.
  FlipV     = 1,
  // Flip the image horizontally.
  FlipH     = 2,
  // Flip both axes, equivalent to a 180-degree rotation.
  Rotate180 = 3,
};

// Possible values: any WiFi SSID string. The default comes from credentials.h
// when WIFI_SSID is defined there; otherwise it stays as "<SSID>".
constexpr const char *WIFI_SSID = app_config_credentials::WIFI_SSID_VALUE;

// Possible values: any WiFi password string, or "" for an open network. The
// default comes from credentials.h when WIFI_PASSWORD is defined there.
constexpr const char *WIFI_PASSWORD = app_config_credentials::WIFI_PASSWORD_VALUE;

// Possible values: an IPv4 address like "192.168.1.200" or a DNS hostname.
// This is the TCP receiver that accepts camera frames.
constexpr const char *SERVER_HOST = "192.168.1.200";

// Possible values: 1-65535. This must match the TCP port opened by the frame
// receiver on SERVER_HOST.
constexpr uint16_t SERVER_PORT = 5000;

// Possible values: 0-4294967295. This ID is sent with every frame so the
// receiver can distinguish multiple cameras.
constexpr uint32_t CAMERA_ID = 10;

// Possible values: any serial baud rate supported by the USB/serial monitor;
// common values are 9600, 115200, 230400, 460800, and 921600.
constexpr uint32_t SERIAL_BAUD = 115200;

// Possible values: serial_log::Level::Error, serial_log::Level::Warn,
// serial_log::Level::Info, or serial_log::Level::Debug. Higher verbosity prints
// more diagnostics and can add a little serial overhead.
constexpr serial_log::Level LOG_LEVEL = serial_log::Level::Info;

// Possible values: milliseconds from 0 upward. 0 prints status whenever the
// status check runs; 1000-10000 is a practical range for normal diagnostics.
constexpr uint32_t STATUS_LOG_INTERVAL_MS = 5000;

// Possible values: FRAMESIZE_96X96, FRAMESIZE_QQVGA, FRAMESIZE_QCIF,
// FRAMESIZE_HQVGA, FRAMESIZE_240X240, FRAMESIZE_QVGA, FRAMESIZE_CIF,
// FRAMESIZE_HVGA, FRAMESIZE_VGA, FRAMESIZE_SVGA, FRAMESIZE_XGA,
// FRAMESIZE_HD, FRAMESIZE_SXGA, FRAMESIZE_UXGA, FRAMESIZE_FHD,
// FRAMESIZE_P_HD, FRAMESIZE_P_3MP, FRAMESIZE_QXGA, FRAMESIZE_QHD,
// FRAMESIZE_WQXGA, FRAMESIZE_P_FHD, or FRAMESIZE_QSXGA. Larger sizes improve
// detail but increase RAM, CPU, and network load; use smaller sizes to reduce
// throttling.
constexpr framesize_t CAMERA_FRAME_SIZE = FRAMESIZE_VGA;

// Possible values: driver quality integers, commonly 10-30 for ESP camera
// JPEG. Lower numbers mean better image quality and larger frames; higher
// numbers mean stronger compression, smaller frames, and lower load.
constexpr int CAMERA_JPEG_QUALITY = 14;

// Possible values: 1 or more frame buffers; 1 and 2 are the practical choices.
// 2 improves capture throughput with PSRAM, while 1 reduces memory pressure and
// latency.
constexpr int CAMERA_FB_COUNT = 2;

// Possible values: same FRAMESIZE_* values as CAMERA_FRAME_SIZE. Keep this
// smaller than the PSRAM setting because frames are stored in internal RAM.
constexpr framesize_t CAMERA_FRAME_SIZE_NO_PSRAM = FRAMESIZE_QVGA;

// Possible values: 1 or more frame buffers; 1 is strongly recommended without
// PSRAM to avoid exhausting internal RAM.
constexpr int CAMERA_FB_COUNT_NO_PSRAM = 1;

// Possible values: CameraRotation::None, CameraRotation::FlipV,
// CameraRotation::FlipH, or CameraRotation::Rotate180. Change this when the
// mounted camera image appears upside down or mirrored.
constexpr CameraRotation CAMERA_ROTATION = CameraRotation::None;

// Possible values: camera XCLK frequency in Hz; common values are 10000000,
// 16000000, and 20000000. 20 MHz is typical, while lower values may reduce
// capture pressure but can be module-dependent.
constexpr uint32_t CAMERA_XCLK_FREQ_HZ = 20000000;

// Possible values: positive FPS values; practical streaming values are usually
// 1-30. Lower this first to reduce CPU, camera, WiFi, and server load. Do not
// set it to 0 because FRAME_INTERVAL_MS divides by it.
constexpr uint32_t TARGET_FPS = 12;

// Possible values: derived from TARGET_FPS as 1000 / TARGET_FPS. Edit
// TARGET_FPS instead of this value so logs and pacing stay consistent.
constexpr uint32_t FRAME_INTERVAL_MS = 1000 / TARGET_FPS;

// Possible values: milliseconds from 0 upward. This is the maximum time to wait
// for WiFi before restarting the connection attempt; 10000-30000 is typical.
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;

// Possible values: milliseconds from 0 upward. This is the maximum time to wait
// while opening the TCP connection to SERVER_HOST:SERVER_PORT.
constexpr uint32_t TCP_CONNECT_TIMEOUT_MS = 8000;

// Possible values: milliseconds from 0 upward. This caps how long one frame
// send may block before the sender treats it as failed and reconnects.
constexpr uint32_t SEND_TIMEOUT_MS = 10000;

// Possible values: milliseconds from 0 upward. This is the first reconnect
// delay after WiFi, TCP, or send failure; lower reconnects faster but retries
// more aggressively.
constexpr uint32_t RECONNECT_BACKOFF_MIN_MS = 500;

// Possible values: milliseconds greater than or equal to
// RECONNECT_BACKOFF_MIN_MS. This caps exponential reconnect backoff after
// repeated failures.
constexpr uint32_t RECONNECT_BACKOFF_MAX_MS = 30000;

// Possible values: task stack size in bytes; must be large enough for camera
// capture work. Increase if stack overflow occurs, decrease only after testing.
constexpr uint32_t CAMERA_TASK_STACK = 6144;

// Possible values: task stack size in bytes; must be large enough for TCP/WiFi
// send work. Increase if stack overflow occurs, decrease only after testing.
constexpr uint32_t SENDER_TASK_STACK = 8192;

// Possible values: usually leave equal to CAMERA_TASK_STACK. This alias is the
// value passed to xTaskCreatePinnedToCore() for the camera task.
constexpr uint32_t CAMERA_TASK_STACK_BYTES = CAMERA_TASK_STACK;

// Possible values: usually leave equal to SENDER_TASK_STACK. This alias is the
// value passed to xTaskCreatePinnedToCore() for the TCP sender task.
constexpr uint32_t SENDER_TASK_STACK_BYTES = SENDER_TASK_STACK;

// Possible values: FreeRTOS priorities from 0 to configMAX_PRIORITIES - 1.
// Higher values run before lower-priority tasks; keep camera above sender when
// capture timing matters.
constexpr UBaseType_t CAMERA_TASK_PRIORITY = 2;

// Possible values: FreeRTOS priorities from 0 to configMAX_PRIORITIES - 1.
// Lower than CAMERA_TASK_PRIORITY lets capture pacing win when the device is
// busy.
constexpr UBaseType_t SENDER_TASK_PRIORITY = 1;

// Possible values: 0, 1, or tskNO_AFFINITY. On dual-core ESP32-S3 boards, core
// 1 is commonly used for application/camera work.
constexpr BaseType_t CAMERA_TASK_CORE = 1;

// Possible values: 0, 1, or tskNO_AFFINITY. Core 0 is commonly used alongside
// WiFi/system work; keep sender and camera split if that is stable.
constexpr BaseType_t SENDER_TASK_CORE = 0;

} // namespace app_config
