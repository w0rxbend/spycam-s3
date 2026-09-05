#include <unity.h>

#include <LogLevel.h>

namespace {

using serial_log::Level;
using serial_log::levelEnabled;

// One test per configured level, each covering all four message levels, so the
// whole 4x4 truth table is spelled out rather than sampled. A change to the
// enum values that quietly reorders severity fails here instead of silently
// dropping errors on the device.

void test_error_level_prints_only_errors()
{
  TEST_ASSERT_TRUE(levelEnabled(Level::Error, Level::Error));
  TEST_ASSERT_FALSE(levelEnabled(Level::Error, Level::Warn));
  TEST_ASSERT_FALSE(levelEnabled(Level::Error, Level::Info));
  TEST_ASSERT_FALSE(levelEnabled(Level::Error, Level::Debug));
}

void test_warn_level_prints_errors_and_warnings()
{
  TEST_ASSERT_TRUE(levelEnabled(Level::Warn, Level::Error));
  TEST_ASSERT_TRUE(levelEnabled(Level::Warn, Level::Warn));
  TEST_ASSERT_FALSE(levelEnabled(Level::Warn, Level::Info));
  TEST_ASSERT_FALSE(levelEnabled(Level::Warn, Level::Debug));
}

void test_info_level_prints_everything_except_debug()
{
  TEST_ASSERT_TRUE(levelEnabled(Level::Info, Level::Error));
  TEST_ASSERT_TRUE(levelEnabled(Level::Info, Level::Warn));
  TEST_ASSERT_TRUE(levelEnabled(Level::Info, Level::Info));
  TEST_ASSERT_FALSE(levelEnabled(Level::Info, Level::Debug));
}

void test_debug_level_prints_every_message()
{
  TEST_ASSERT_TRUE(levelEnabled(Level::Debug, Level::Error));
  TEST_ASSERT_TRUE(levelEnabled(Level::Debug, Level::Warn));
  TEST_ASSERT_TRUE(levelEnabled(Level::Debug, Level::Info));
  TEST_ASSERT_TRUE(levelEnabled(Level::Debug, Level::Debug));
}

// Errors are the one thing that must never be filtered out: a board configured
// at its quietest still has to report why it failed.
void test_errors_are_enabled_at_every_configured_level()
{
  TEST_ASSERT_TRUE(levelEnabled(Level::Error, Level::Error));
  TEST_ASSERT_TRUE(levelEnabled(Level::Warn, Level::Error));
  TEST_ASSERT_TRUE(levelEnabled(Level::Info, Level::Error));
  TEST_ASSERT_TRUE(levelEnabled(Level::Debug, Level::Error));
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_error_level_prints_only_errors);
  RUN_TEST(test_warn_level_prints_errors_and_warnings);
  RUN_TEST(test_info_level_prints_everything_except_debug);
  RUN_TEST(test_debug_level_prints_every_message);
  RUN_TEST(test_errors_are_enabled_at_every_configured_level);
  return UNITY_END();
}
