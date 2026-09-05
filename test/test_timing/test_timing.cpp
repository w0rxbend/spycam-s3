#include <unity.h>

#include <Timing.h>

namespace {

void test_interval_timer_is_not_due_before_the_interval()
{
  timing::IntervalTimer timer(1000, 5000);

  TEST_ASSERT_FALSE(timer.due(5999));
}

void test_interval_timer_is_due_at_exactly_the_interval()
{
  timing::IntervalTimer timer(1000, 5000);

  TEST_ASSERT_TRUE(timer.due(6000));
}

void test_interval_timer_rearms_from_the_firing_time()
{
  timing::IntervalTimer timer(1000, 5000);

  TEST_ASSERT_TRUE(timer.due(6000));
  TEST_ASSERT_FALSE(timer.due(6999));
  TEST_ASSERT_TRUE(timer.due(7000));
}

void test_interval_timer_survives_millis_wraparound()
{
  // 0xFFFFFC18 is 1000 ms before the counter wraps to zero.
  timing::IntervalTimer timer(1000, 0xFFFFFC18u);

  TEST_ASSERT_FALSE(timer.due(0xFFFFFFFFu));
  // 1080 ms of real time have elapsed across the wrap.
  TEST_ASSERT_TRUE(timer.due(0x00000050u));
}

void test_backoff_returns_min_first()
{
  timing::Backoff backoff(500, 30000);

  TEST_ASSERT_EQUAL_UINT32(500, backoff.nextDelayMs());
}

void test_backoff_doubles_then_saturates()
{
  timing::Backoff backoff(500, 30000);

  TEST_ASSERT_EQUAL_UINT32(500, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(1000, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(2000, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(4000, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(8000, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(16000, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(30000, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(30000, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(30000, backoff.nextDelayMs());
}

void test_backoff_reset_returns_to_min()
{
  timing::Backoff backoff(500, 30000);

  backoff.nextDelayMs();
  backoff.nextDelayMs();
  backoff.nextDelayMs();
  backoff.reset();

  TEST_ASSERT_EQUAL_UINT32(500, backoff.nextDelayMs());
}

void test_backoff_never_overflows_when_doubling()
{
  timing::Backoff backoff(0x40000000u, 0xFFFFFFFFu);

  TEST_ASSERT_EQUAL_UINT32(0x40000000u, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(0x80000000u, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, backoff.nextDelayMs());
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, backoff.nextDelayMs());
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_interval_timer_is_not_due_before_the_interval);
  RUN_TEST(test_interval_timer_is_due_at_exactly_the_interval);
  RUN_TEST(test_interval_timer_rearms_from_the_firing_time);
  RUN_TEST(test_interval_timer_survives_millis_wraparound);
  RUN_TEST(test_backoff_returns_min_first);
  RUN_TEST(test_backoff_doubles_then_saturates);
  RUN_TEST(test_backoff_reset_returns_to_min);
  RUN_TEST(test_backoff_never_overflows_when_doubling);
  return UNITY_END();
}
