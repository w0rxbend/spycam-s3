ESP32-S3-CAM TCP JPEG Client

Captures JPEG frames directly from an ESP32-S3 camera board and sends the latest frame to any TCP server implementing the frame protocol described below.

## Target Hardware

The default PlatformIO environment targets a local `goouuu_esp32_s3_cam` board profile for the GOOUUU ESP32-S3-CAM / ESP32-S3-WROOM N16R8 style board with 16 MB flash and 8 MB OPI PSRAM.

The camera pin map in `include/CameraPins.h` uses the GOOUUU OV2640 assignment:

```text
SIOD=4 SIOC=5 VSYNC=6 HREF=7 PCLK=13 XCLK=15
Y2=11 Y3=9 Y4=8 Y5=10 Y6=12 Y7=18 Y8=17 Y9=16
PWDN=-1 RESET=-1
```

If your ESP32-S3-CAM board is a different vendor layout, update only `include/CameraPins.h`.

## Client architecture

```text
camera task -> latest_frame slot -> tcp sender task
```

The camera task captures JPEG frames directly from the ESP32 camera driver. The latest-frame slot stores only one unsent frame at a time. If the TCP sender is slow or the server is unavailable, stale frames are returned to the camera driver and dropped.

The camera task hands each frame buffer to the latest-frame slot and it is only returned to the driver after the send completes, so the camera driver needs at least two frame buffers (`CAMERA_FB_COUNT >= 2`) to overlap capture with sending.

The sender task owns all network I/O. It reconnects WiFi and TCP forever with exponential backoff, then resumes streaming from the newest available frame.

## Repository layout

```text
lib/frame_protocol/src/FrameProtocol.h   wire-format encoding      (portable)
lib/timing/src/Timing.h                  interval timer, backoff   (portable)
include/                                 hardware-coupled headers
src/                                     firmware implementation
test/                                    host-side unit tests
```

The two directories under `lib/` are PlatformIO private libraries. Everything in them is plain C++11 with no Arduino, FreeRTOS, or `esp_camera` include, which is what lets the `native` environment compile and unit-test them on your development machine instead of on the board. PlatformIO's Library Dependency Finder discovers them automatically from the `#include` lines, so neither environment lists them in `lib_deps`.

Keep it that way: adding a hardware include to one of those headers would silently drop it out of reach of `just test`. Hardware-coupled headers belong in `include/`.

## Client-server protocol

Transport is a long-lived raw TCP connection. There is no HTTP, websocket, JSON, delimiter, or text framing.

The client sends a repeated binary frame:

```text
header (16 bytes)
  bytes 0..3      magic: 0x4A504744, ASCII "JPGD", uint32 big-endian
  bytes 4..7      sequence number, uint32 big-endian
  bytes 8..11     JPEG payload length in bytes, uint32 big-endian
  bytes 12..15    camera millis() timestamp, uint32 big-endian

camera id (4 bytes)
  bytes 16..19    camera id, uint32 big-endian

payload
  bytes 20..N     JPEG data, exactly payload length bytes
```

The fixed prefix before the JPEG is 20 bytes in total. A receiver may read it as one 20-byte read or as 16 + 4; the client sends it as two writes on the same stream.

After one frame is sent, the next frame starts immediately with another header on the same TCP stream.

Receiver rules:

- Read exactly 16 bytes for the header.
- Decode all integer fields as big-endian `uint32`.
- Reject the frame if magic is not `JPGD`.
- Read exactly 4 bytes for the camera id.
- Read exactly `payload length` bytes for the JPEG.
- Treat TCP disconnects as normal; the ESP32-S3 client will reconnect and continue with later frames.

The sequence number is useful for detecting dropped frames. The timestamp is the ESP32 `millis()` value when the frame header is built, not wall-clock time, and it wraps to 0 roughly every 49.7 days — a receiver computing frame-to-frame deltas must handle that.

## Configuration

Configure WiFi, server address, camera id, frame size, JPEG quality, and target FPS in `include/AppConfig.h`.

Add `include/credentials.h` locally if you want compile-time WiFi credentials:

```cpp
#pragma once
#define WIFI_SSID "your-ssid"
#define WIFI_PASSWORD "your-password"
```

## Common commands

```sh
just test
just lint
just build
just flash /dev/ttyACM0
just monitor /dev/ttyACM0
```
