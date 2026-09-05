#include <Arduino.h>
#include <Timing.h>

#include <utility>

#include "AppConfig.h"
#include "CameraFrame.h"
#include "CameraManager.h"
#include "LatestFrameSlot.h"
#include "SerialLog.h"
#include "TcpFrameSender.h"

namespace
{

  LatestFrameSlot latestFrame;
  TcpFrameSender sender(app_config::SERVER_HOST, app_config::SERVER_PORT);

  // Give the serial buffer a moment to flush the fatal message before reboot.
  constexpr uint32_t kFatalRestartDelayMs = 1000;
  // Let the USB-CDC serial port enumerate before the startup banner is printed.
  constexpr uint32_t kBootBannerDelayMs = 1000;
  // Pause after a failed capture so a wedged sensor cannot spin the task.
  constexpr uint32_t kCaptureRetryDelayMs = 250;
  // How long the sender waits for a new frame before looping to re-check the link.
  constexpr uint32_t kFrameWaitMs = 1000;
  constexpr uint32_t kIdleLoopDelayMs = 1000;

  [[noreturn]] void fatal(const char *reason)
  {
    serial_log::error("%s", reason);
    delay(kFatalRestartDelayMs);
    ESP.restart();
    // ESP.restart() is declared void, so the compiler cannot see that it never returns.
    for (;;)
    {
    }
  }

  bool startTask(TaskFunction_t fn, const char *taskName, const char *logLabel,
                 uint32_t stackBytes, UBaseType_t priority, BaseType_t core)
  {
    const BaseType_t created = xTaskCreatePinnedToCore(fn, taskName, stackBytes, nullptr, priority, nullptr, core);
    if (created != pdPASS)
    {
      return false;
    }
    serial_log::info("%s task started: core=%d stack=%lu", logLabel,
                     static_cast<int>(core), static_cast<unsigned long>(stackBytes));
    return true;
  }

  void logHardware()
  {
    serial_log::info("Hardware: chip=%s rev=%u cpu=%uMHz flash=%lu psram=%lu",
                     ESP.getChipModel(),
                     static_cast<unsigned>(ESP.getChipRevision()),
                     static_cast<unsigned>(ESP.getCpuFreqMHz()),
                     static_cast<unsigned long>(ESP.getFlashChipSize()),
                     static_cast<unsigned long>(ESP.getPsramSize()));
    serial_log::info("Memory: heap_free=%lu/%lu psram_free=%lu/%lu",
                     static_cast<unsigned long>(ESP.getFreeHeap()),
                     static_cast<unsigned long>(ESP.getHeapSize()),
                     static_cast<unsigned long>(ESP.getFreePsram()),
                     static_cast<unsigned long>(ESP.getPsramSize()));
  }

  void cameraTask(void *parameter)
  {
    static_cast<void>(parameter);
    TickType_t lastWake = xTaskGetTickCount();
    uint32_t capturedFrames = 0;
    uint32_t captureFailures = 0;
    timing::IntervalTimer statusLog(app_config::STATUS_LOG_INTERVAL_MS, millis());

    for (;;)
    {
      CameraFrame frame = camera_manager::capture();
      if (!frame)
      {
        ++captureFailures;
        serial_log::warn("Camera capture failed");
        vTaskDelay(pdMS_TO_TICKS(kCaptureRetryDelayMs));
        lastWake = xTaskGetTickCount();
        continue;
      }

      ++capturedFrames;
      latestFrame.put(std::move(frame));

      if (statusLog.due(millis()))
      {
        serial_log::info("Camera task: captured=%lu capture_failures=%lu stack_free=%u",
                         static_cast<unsigned long>(capturedFrames),
                         static_cast<unsigned long>(captureFailures),
                         static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
      }

      vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(app_config::FRAME_INTERVAL_MS));
    }
  }

  void senderTask(void *parameter)
  {
    static_cast<void>(parameter);
    sender.begin();

    for (;;)
    {
      if (!sender.ensureConnected())
      {
        continue;
      }

      CameraFrame frame = latestFrame.takeLatest(pdMS_TO_TICKS(kFrameWaitMs));
      if (!frame)
      {
        continue;
      }

      // No explicit release: `frame` returns the buffer to the driver when this
      // loop iteration ends, on every path out of it.
      sender.sendFrame(frame);
    }
  }

} // namespace

void setup()
{
  serial_log::begin(app_config::SERIAL_BAUD, app_config::LOG_LEVEL);
  delay(kBootBannerDelayMs);
  serial_log::info("ESP32-S3-CAM TCP JPEG client starting");
  logHardware();
  serial_log::info("Target: tcp://%s:%u camera_id=%lu target_fps=%lu",
                   app_config::SERVER_HOST,
                   static_cast<unsigned>(app_config::SERVER_PORT),
                   static_cast<unsigned long>(app_config::CAMERA_ID),
                   static_cast<unsigned long>(app_config::TARGET_FPS));

  if (!latestFrame.begin())
  {
    fatal("Failed to create frame slot synchronization primitives");
  }

  if (!camera_manager::begin())
  {
    fatal("Failed to initialize camera");
  }

  if (!startTask(cameraTask, "camera", "Camera", app_config::CAMERA_TASK_STACK_BYTES,
                 app_config::CAMERA_TASK_PRIORITY, app_config::CAMERA_TASK_CORE))
  {
    fatal("Failed to create camera task");
  }

  if (!startTask(senderTask, "tcp_sender", "TCP sender", app_config::SENDER_TASK_STACK_BYTES,
                 app_config::SENDER_TASK_PRIORITY, app_config::SENDER_TASK_CORE))
  {
    fatal("Failed to create TCP sender task");
  }
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(kIdleLoopDelayMs));
}
