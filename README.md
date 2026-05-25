ESP32-S3-CAM TCP JPEG Client

Captures JPEG frames directly from an ESP32-S3 camera board and sends the latest frame to the same TCP server protocol used by `../spycam`.

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

The sender task owns all network I/O. It reconnects WiFi and TCP forever with exponential backoff, then resumes streaming from the newest available frame.

## Client-server protocol

Transport is a long-lived raw TCP connection. There is no HTTP, websocket, JSON, delimiter, or text framing.

The client sends a repeated binary frame:

```text
bytes 0..3      magic: 0x4A504744, ASCII "JPGD", uint32 big-endian
bytes 4..7      sequence number, uint32 big-endian
bytes 8..11     JPEG payload length in bytes, uint32 big-endian
bytes 12..15    camera millis() timestamp, uint32 big-endian
bytes 16..19    camera id, uint32 big-endian
bytes 20..N     JPEG payload, exactly payload length bytes
```

After one frame is sent, the next frame starts immediately with another header on the same TCP stream.

Receiver rules:

- Read exactly 16 bytes for the header.
- Decode all integer fields as big-endian `uint32`.
- Reject the frame if magic is not `JPGD`.
- Read exactly 4 bytes for the camera id.
- Read exactly `payload length` bytes for the JPEG.
- Treat TCP disconnects as normal; the ESP32-S3 client will reconnect and continue with later frames.

The sequence number is useful for detecting dropped frames. The timestamp is the ESP32 `millis()` value when the frame header is built; it is not wall-clock time.

## Configuration

Configure WiFi, server address, camera id, frame size, JPEG quality, and target FPS in `include/AppConfig.h`.

The project intentionally does not copy `../spycam/include/credentials.h`. Add `include/credentials.h` locally if you want compile-time WiFi credentials:

```cpp
#pragma once
#define WIFI_SSID "your-ssid"
#define WIFI_PASSWORD "your-password"
```

## Common commands

```sh
just test
just build
just flash /dev/ttyACM0
just monitor /dev/ttyACM0
```
