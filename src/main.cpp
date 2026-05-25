#include <Arduino.h>

#include "AppConfig.h"
#include "CameraManager.h"
#include "LatestFrameSlot.h"
#include "SerialLog.h"
#include "TcpFrameSender.h"

namespace
{

  CameraManager camera;
  LatestFrameSlot latestFrame;
  TcpFrameSender sender(app_config::SERVER_HOST, app_config::SERVER_PORT);

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
    uint32_t lastStatusAt = millis();

    for (;;)
    {
      camera_fb_t *frame = camera.capture();
      if (frame == nullptr)
      {
        ++captureFailures;
        serial_log::warn("Camera capture failed");
        vTaskDelay(pdMS_TO_TICKS(250));
        lastWake = xTaskGetTickCount();
        continue;
      }

      ++capturedFrames;
      latestFrame.put(frame);

      const uint32_t now = millis();
      if (now - lastStatusAt >= app_config::STATUS_LOG_INTERVAL_MS)
      {
        serial_log::info("Camera task: captured=%lu capture_failures=%lu stack_free=%u",
                         static_cast<unsigned long>(capturedFrames),
                         static_cast<unsigned long>(captureFailures),
                         static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
        lastStatusAt = now;
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

      camera_fb_t *frame = latestFrame.takeLatest(pdMS_TO_TICKS(1000));
      if (frame == nullptr)
      {
        continue;
      }

      sender.sendFrame(frame);
      camera.release(frame);
    }
  }

} // namespace

void setup()
{
  serial_log::begin(app_config::SERIAL_BAUD, app_config::LOG_LEVEL);
  delay(1000);
  serial_log::info("ESP32-S3-CAM TCP JPEG client starting");
  logHardware();
  serial_log::info("Target: tcp://%s:%u camera_id=%lu target_fps=%lu",
                   app_config::SERVER_HOST,
                   app_config::SERVER_PORT,
                   static_cast<unsigned long>(app_config::CAMERA_ID),
                   static_cast<unsigned long>(app_config::TARGET_FPS));

  if (!latestFrame.begin())
  {
    serial_log::error("Failed to create frame slot synchronization primitives");
    delay(1000);
    ESP.restart();
  }

  if (!camera.begin())
  {
    serial_log::error("Failed to initialize camera");
    delay(1000);
    ESP.restart();
  }

  BaseType_t taskCreated = xTaskCreatePinnedToCore(cameraTask,
                                                   "camera",
                                                   app_config::CAMERA_TASK_STACK_BYTES,
                                                   nullptr,
                                                   app_config::CAMERA_TASK_PRIORITY,
                                                   nullptr,
                                                   app_config::CAMERA_TASK_CORE);
  if (taskCreated != pdPASS)
  {
    serial_log::error("Failed to create camera task");
    delay(1000);
    ESP.restart();
  }
  serial_log::info("Camera task started: core=%d stack=%lu",
                   static_cast<int>(app_config::CAMERA_TASK_CORE),
                   static_cast<unsigned long>(app_config::CAMERA_TASK_STACK_BYTES));

  taskCreated = xTaskCreatePinnedToCore(senderTask,
                                        "tcp_sender",
                                        app_config::SENDER_TASK_STACK_BYTES,
                                        nullptr,
                                        app_config::SENDER_TASK_PRIORITY,
                                        nullptr,
                                        app_config::SENDER_TASK_CORE);
  if (taskCreated != pdPASS)
  {
    serial_log::error("Failed to create TCP sender task");
    delay(1000);
    ESP.restart();
  }
  serial_log::info("TCP sender task started: core=%d stack=%lu",
                   static_cast<int>(app_config::SENDER_TASK_CORE),
                   static_cast<unsigned long>(app_config::SENDER_TASK_STACK_BYTES));
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}
