#include <string.h>
#include <unity.h>

#include <FrameProtocol.h>

namespace {

void test_write_u32_be_encodes_most_significant_byte_first()
{
  uint8_t bytes[4] = {};

  frame_protocol::writeU32Be(bytes, 0x12345678);

  TEST_ASSERT_EQUAL_HEX8(0x12, bytes[0]);
  TEST_ASSERT_EQUAL_HEX8(0x34, bytes[1]);
  TEST_ASSERT_EQUAL_HEX8(0x56, bytes[2]);
  TEST_ASSERT_EQUAL_HEX8(0x78, bytes[3]);
}

// The receiver reads a fixed-size header and trusts it; there is no resync on a
// bad magic, so the exact byte layout is compared against literals here rather
// than against a decoder written in this file (which would only prove the two
// mirror each other).
void test_build_header_encodes_protocol_fields()
{
  uint8_t header[frame_protocol::HEADER_SIZE] = {};
  const uint8_t expected[frame_protocol::HEADER_SIZE] = {
      0x4A, 0x50, 0x47, 0x44, // "JPGD"
      0x00, 0x00, 0x00, 0x2A, // sequence 42
      0x00, 0x01, 0xE2, 0x40, // payload length 123456
      0x3A, 0xDE, 0x68, 0xB1  // timestamp 987654321
  };

  frame_protocol::buildHeader(header, 42, 123456, 987654321);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, header, frame_protocol::HEADER_SIZE);
}

void test_header_size_constant_is_16_bytes()
{
  TEST_ASSERT_EQUAL_UINT(16, frame_protocol::HEADER_SIZE);
}

void test_magic_is_ascii_jpgd()
{
  uint8_t header[frame_protocol::HEADER_SIZE] = {};

  TEST_ASSERT_EQUAL_HEX32(0x4A504744u, frame_protocol::MAGIC_JPGD);

  frame_protocol::buildHeader(header, 1, 1, 1);

  TEST_ASSERT_EQUAL_HEX8(0x4A, header[0]); // 'J'
  TEST_ASSERT_EQUAL_HEX8(0x50, header[1]); // 'P'
  TEST_ASSERT_EQUAL_HEX8(0x47, header[2]); // 'G'
  TEST_ASSERT_EQUAL_HEX8(0x44, header[3]); // 'D'
}

// Zero and 0xFFFFFFFF catch sign-extension or int-truncation bugs that
// mid-range values pass straight over.
void test_build_header_boundary_values()
{
  uint8_t header[frame_protocol::HEADER_SIZE] = {};

  const uint8_t allZero[frame_protocol::HEADER_SIZE] = {
      0x4A, 0x50, 0x47, 0x44,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00};
  frame_protocol::buildHeader(header, 0, 0, 0);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(allZero, header, frame_protocol::HEADER_SIZE);

  const uint8_t allMax[frame_protocol::HEADER_SIZE] = {
      0x4A, 0x50, 0x47, 0x44,
      0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF};
  frame_protocol::buildHeader(header, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(allMax, header, frame_protocol::HEADER_SIZE);

  const uint8_t mixed[frame_protocol::HEADER_SIZE] = {
      0x4A, 0x50, 0x47, 0x44,
      0x00, 0x00, 0x00, 0x00,
      0xFF, 0xFF, 0xFF, 0xFF,
      0x00, 0x00, 0x00, 0x01};
  frame_protocol::buildHeader(header, 0, 0xFFFFFFFFu, 1);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(mixed, header, frame_protocol::HEADER_SIZE);
}

// buildHeader takes a raw pointer with no length, so an overrun would be
// invisible to the compiler; guard bytes make it visible here.
void test_build_header_writes_exactly_sixteen_bytes()
{
  uint8_t buffer[24];
  memset(buffer, 0xAA, sizeof(buffer));

  frame_protocol::buildHeader(buffer, 42, 123456, 987654321);

  for (size_t i = frame_protocol::HEADER_SIZE; i < sizeof(buffer); ++i) {
    TEST_ASSERT_EQUAL_HEX8(0xAA, buffer[i]);
  }
}

void test_build_camera_id_encodes_big_endian_id()
{
  uint8_t bytes[frame_protocol::CAMERA_ID_SIZE] = {};
  const uint8_t expected[frame_protocol::CAMERA_ID_SIZE] = {0x00, 0x00, 0x00, 0x11};

  frame_protocol::buildCameraId(bytes, 17);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, bytes, frame_protocol::CAMERA_ID_SIZE);
}

void test_build_camera_id_writes_exactly_four_bytes()
{
  uint8_t buffer[8];

  memset(buffer, 0xAA, sizeof(buffer));
  frame_protocol::buildCameraId(buffer, 0);
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[2]);
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[3]);
  for (size_t i = frame_protocol::CAMERA_ID_SIZE; i < sizeof(buffer); ++i) {
    TEST_ASSERT_EQUAL_HEX8(0xAA, buffer[i]);
  }

  memset(buffer, 0xAA, sizeof(buffer));
  frame_protocol::buildCameraId(buffer, 0xFFFFFFFFu);
  TEST_ASSERT_EQUAL_HEX8(0xFF, buffer[0]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, buffer[1]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, buffer[2]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, buffer[3]);
  for (size_t i = frame_protocol::CAMERA_ID_SIZE; i < sizeof(buffer); ++i) {
    TEST_ASSERT_EQUAL_HEX8(0xAA, buffer[i]);
  }
}

// TcpFrameSender emits the header and the camera id as two separate writes;
// this pins the fact that they must concatenate into the documented 20-byte
// prefix the receiver parses in one go.
void test_wire_frame_layout_matches_protocol_doc()
{
  uint8_t wire[20];
  const uint8_t expected[20] = {
      0x4A, 0x50, 0x47, 0x44,
      0x00, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x02,
      0x00, 0x00, 0x00, 0x03,
      0x00, 0x00, 0x00, 0x0A};

  frame_protocol::buildHeader(wire, 1, 2, 3);
  frame_protocol::buildCameraId(wire + frame_protocol::HEADER_SIZE, 10);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, wire, sizeof(wire));
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_write_u32_be_encodes_most_significant_byte_first);
  RUN_TEST(test_build_header_encodes_protocol_fields);
  RUN_TEST(test_header_size_constant_is_16_bytes);
  RUN_TEST(test_magic_is_ascii_jpgd);
  RUN_TEST(test_build_header_boundary_values);
  RUN_TEST(test_build_header_writes_exactly_sixteen_bytes);
  RUN_TEST(test_build_camera_id_encodes_big_endian_id);
  RUN_TEST(test_build_camera_id_writes_exactly_four_bytes);
  RUN_TEST(test_wire_frame_layout_matches_protocol_doc);
  return UNITY_END();
}
