#include <unity.h>
#include <cstdlib>
#include <string>
#include "Config.h"
#include "Dcf77.h"

void setUp() {
  setenv("TZ", dcf77::kTimezoneRule, 1);
  tzset();
}
void tearDown() {}

time_t utc(int year, int month, int day, int hour, int minute, int second = 0) {
  tm value{};
  value.tm_year = year - 1900;
  value.tm_mon = month - 1;
  value.tm_mday = day;
  value.tm_hour = hour;
  value.tm_min = minute;
  value.tm_sec = second;
  return timegm(&value);
}

unsigned decode(const dcf77::Frame& frame, unsigned first, unsigned count) {
  unsigned value = 0;
  for (unsigned bit = 0; bit < count; ++bit) {
    if (frame.pulseMs[first + bit] == 200) value |= 1U << bit;
  }
  return (value >> 4) * 10 + (value & 15);
}

void assertParity(const dcf77::Frame& frame, unsigned first, unsigned last) {
  unsigned ones = 0;
  for (unsigned bit = first; bit <= last; ++bit) ones += frame.pulseMs[bit] == 200;
  TEST_ASSERT_EQUAL_UINT(0, ones % 2);
}

void test_known_pacific_frame() {
  const auto frame = dcf77::encodeMinute(utc(2026, 11, 6, 20, 34, 42));
  TEST_ASSERT_EQUAL_UINT(35, decode(frame, 21, 7));
  TEST_ASSERT_EQUAL_UINT(12, decode(frame, 29, 6));
  TEST_ASSERT_EQUAL_UINT(6, decode(frame, 36, 6));
  TEST_ASSERT_EQUAL_UINT(5, decode(frame, 42, 3));
  TEST_ASSERT_EQUAL_UINT(11, decode(frame, 45, 5));
  TEST_ASSERT_EQUAL_UINT(26, decode(frame, 50, 8));
  TEST_ASSERT_EQUAL_UINT8(100, frame.pulseMs[0]);
  TEST_ASSERT_EQUAL_UINT8(100, frame.pulseMs[16]);
  TEST_ASSERT_EQUAL_UINT8(100, frame.pulseMs[17]);
  TEST_ASSERT_EQUAL_UINT8(200, frame.pulseMs[18]);
  TEST_ASSERT_EQUAL_UINT8(100, frame.pulseMs[19]);
  TEST_ASSERT_EQUAL_UINT8(200, frame.pulseMs[20]);
  const uint8_t minuteBits[] = {200, 100, 200, 100, 200, 200, 100, 100};
  TEST_ASSERT_EQUAL_UINT8_ARRAY(minuteBits, frame.pulseMs + 21, 8);
  assertParity(frame, 21, 28);
  assertParity(frame, 29, 35);
  assertParity(frame, 36, 58);
  TEST_ASSERT_EQUAL_UINT8(0, frame.pulseMs[59]);
}

void test_year_rollover_uses_next_date() {
  const auto frame = dcf77::encodeMinute(utc(2027, 1, 1, 7, 59));
  TEST_ASSERT_EQUAL_UINT(0, decode(frame, 21, 7));
  TEST_ASSERT_EQUAL_UINT(0, decode(frame, 29, 6));
  TEST_ASSERT_EQUAL_UINT(1, decode(frame, 36, 6));
  TEST_ASSERT_EQUAL_UINT(5, decode(frame, 42, 3));
  TEST_ASSERT_EQUAL_UINT(1, decode(frame, 45, 5));
  TEST_ASSERT_EQUAL_UINT(27, decode(frame, 50, 8));
  assertParity(frame, 36, 58);
}

void test_leap_day_and_sunday_numbering() {
  const auto leap = dcf77::encodeMinute(utc(2028, 2, 29, 7, 59));
  TEST_ASSERT_EQUAL_UINT(29, decode(leap, 36, 6));
  TEST_ASSERT_EQUAL_UINT(2, decode(leap, 45, 5));
  const auto march = dcf77::encodeMinute(utc(2028, 3, 1, 7, 59));
  TEST_ASSERT_EQUAL_UINT(1, decode(march, 36, 6));
  TEST_ASSERT_EQUAL_UINT(3, decode(march, 45, 5));
  const auto sunday = dcf77::encodeMinute(utc(2026, 11, 8, 20, 0));
  TEST_ASSERT_EQUAL_UINT(7, decode(sunday, 42, 3));
}

void test_pst_and_pdt_offsets() {
  auto winter = dcf77::encodeMinute(utc(2026, 1, 15, 20, 0));
  auto summer = dcf77::encodeMinute(utc(2026, 7, 15, 20, 0));
  TEST_ASSERT_EQUAL_UINT(12, decode(winter, 29, 6));
  TEST_ASSERT_EQUAL_UINT(13, decode(summer, 29, 6));
  TEST_ASSERT_EQUAL_INT(0, winter.announcedLocal.tm_isdst);
  TEST_ASSERT_TRUE(summer.announcedLocal.tm_isdst > 0);
  TEST_ASSERT_EQUAL_UINT8(200, summer.pulseMs[17]);
  TEST_ASSERT_EQUAL_UINT8(100, summer.pulseMs[18]);
}

void test_spring_transition() {
  const auto before = dcf77::encodeMinute(utc(2026, 3, 8, 9, 58));
  TEST_ASSERT_EQUAL_UINT(1, decode(before, 29, 6));
  TEST_ASSERT_EQUAL_UINT(59, decode(before, 21, 7));
  TEST_ASSERT_EQUAL_UINT8(200, before.pulseMs[18]);
  const auto transition = dcf77::encodeMinute(utc(2026, 3, 8, 9, 59));
  TEST_ASSERT_EQUAL_UINT(3, decode(transition, 29, 6));
  TEST_ASSERT_EQUAL_UINT(0, decode(transition, 21, 7));
  TEST_ASSERT_EQUAL_UINT8(200, transition.pulseMs[17]);
  TEST_ASSERT_EQUAL_UINT8(100, transition.pulseMs[18]);
  TEST_ASSERT_EQUAL_UINT8(200, transition.pulseMs[16]);
  TEST_ASSERT_EQUAL_UINT8(100, dcf77::encodeMinute(utc(2026, 3, 8, 10, 0)).pulseMs[16]);
}

void test_fall_transition() {
  const auto frame = dcf77::encodeMinute(utc(2026, 11, 1, 8, 59));
  TEST_ASSERT_EQUAL_UINT(1, decode(frame, 29, 6));
  TEST_ASSERT_EQUAL_UINT(0, decode(frame, 21, 7));
  TEST_ASSERT_EQUAL_UINT8(200, frame.pulseMs[16]);
  TEST_ASSERT_EQUAL_UINT8(100, frame.pulseMs[17]);
  TEST_ASSERT_EQUAL_UINT8(200, frame.pulseMs[18]);
  const auto after = dcf77::encodeMinute(utc(2026, 11, 1, 9, 0));
  TEST_ASSERT_EQUAL_UINT8(100, after.pulseMs[16]);
  TEST_ASSERT_EQUAL_UINT(1, decode(after, 21, 7));
}

void test_change_announcement_only_preceding_hour() {
  TEST_ASSERT_EQUAL_UINT8(100, dcf77::encodeMinute(utc(2026, 3, 8, 8, 59)).pulseMs[16]);
  TEST_ASSERT_EQUAL_UINT8(200, dcf77::encodeMinute(utc(2026, 3, 8, 9, 0)).pulseMs[16]);
  TEST_ASSERT_EQUAL_UINT8(100, dcf77::encodeMinute(utc(2026, 11, 1, 7, 59)).pulseMs[16]);
  TEST_ASSERT_EQUAL_UINT8(200, dcf77::encodeMinute(utc(2026, 11, 1, 8, 0)).pulseMs[16]);
}

const int64_t kEpoch = static_cast<int64_t>(utc(2026, 11, 6, 20, 34)) * 1000000;

void test_no_signal_without_ntp_or_mid_minute_start() {
  dcf77::Modulator signal;
  TEST_ASSERT_TRUE(signal.update(kEpoch, 0, false) == dcf77::Drive::Off);
  TEST_ASSERT_TRUE(signal.update(kEpoch + 30000000, 30000000, true) == dcf77::Drive::Off);
  for (int64_t t = 30005000; t < 60000000; t += 5000) {
    TEST_ASSERT_TRUE(signal.update(kEpoch + t, t, true) == dcf77::Drive::Off);
  }
  TEST_ASSERT_TRUE(signal.update(kEpoch + 60000000, 60000000, true) == dcf77::Drive::Reduced);
  TEST_ASSERT_TRUE(signal.transmitting());
}

void test_full_minute_pulse_durations_and_marker() {
  dcf77::Modulator signal;
  unsigned reducedSamples[60]{};
  for (int64_t t = 0; t < 60000000; t += 5000) {
    const auto drive = signal.update(kEpoch + t, t, true);
    TEST_ASSERT_TRUE(drive != dcf77::Drive::Off);
    if (drive == dcf77::Drive::Reduced) ++reducedSamples[t / 1000000];
  }
  const auto frame = dcf77::encodeMinute(kEpoch / 1000000);
  for (unsigned second = 0; second < 60; ++second) {
    TEST_ASSERT_EQUAL_UINT(frame.pulseMs[second], reducedSamples[second] * 5);
  }
  TEST_ASSERT_EQUAL_UINT(0, reducedSamples[59]);
  TEST_ASSERT_TRUE(signal.update(kEpoch + 60000000, 60000000, true) == dcf77::Drive::Reduced);
  TEST_ASSERT_EQUAL_UINT(0, signal.droppedFrames());
}

void test_short_timer_jitter_does_not_accumulate() {
  dcf77::Modulator signal;
  for (int64_t t = 3000; t < 60000000; t += 5000) {
    const auto drive = signal.update(kEpoch + t, t, true);
    const int64_t withinSecond = t % 1000000;
    if (t / 1000000 == 0) {
      TEST_ASSERT_TRUE(drive == (withinSecond < 100000 ? dcf77::Drive::Reduced : dcf77::Drive::Full));
    }
  }
  TEST_ASSERT_TRUE(signal.update(kEpoch + 60003000, 60003000, true) == dcf77::Drive::Reduced);
}

void test_late_callback_abandons_frame_until_next_minute() {
  dcf77::Modulator signal;
  signal.update(kEpoch, 0, true);
  TEST_ASSERT_TRUE(signal.update(kEpoch + 30000, 30000, true) == dcf77::Drive::Off);
  for (int64_t t = 35000; t < 60000000; t += 5000) signal.update(kEpoch + t, t, true);
  TEST_ASSERT_FALSE(signal.transmitting());
  TEST_ASSERT_TRUE(signal.update(kEpoch + 60000000, 60000000, true) == dcf77::Drive::Reduced);
  TEST_ASSERT_EQUAL_UINT(1, signal.droppedFrames());
}

void test_clock_steps_in_both_directions_abandon_frame() {
  for (int direction : {-1, 1}) {
    dcf77::Modulator signal;
    signal.update(kEpoch, 0, true);
    TEST_ASSERT_TRUE(signal.update(kEpoch + 5000 + direction * 1000000, 5000, true) == dcf77::Drive::Off);
    TEST_ASSERT_FALSE(signal.transmitting());
    TEST_ASSERT_EQUAL_UINT(1, signal.droppedFrames());
  }
}

void test_clock_expiry_silences_output() {
  dcf77::Modulator signal;
  signal.update(kEpoch, 0, true);
  TEST_ASSERT_TRUE(signal.update(kEpoch + 5000, 5000, false) == dcf77::Drive::Off);
  TEST_ASSERT_FALSE(signal.transmitting());
  TEST_ASSERT_TRUE(signal.update(kEpoch + 30000000, 30000000, true) == dcf77::Drive::Off);
}

void test_ntp_hosts() {
  for (const char* host : {"pool.ntp.org", "time.cloudflare.com", "ntp-1.lan", "192.168.1.10", "NTP", "255.255.255.255"}) {
    TEST_ASSERT_TRUE_MESSAGE(config::validNtpHost(host), host);
  }
  for (const char* host : {"", "http://pool.ntp.org", "ntp:123", "ntp/path", "a b", "a\nb", "<script>",
                           "a..b", "-ntp.org", "ntp-.org", ".ntp", "ntp.", "256.1.2.3", "1.2.3", "1.2.3.4.5", "1.2.3.0000"}) {
    TEST_ASSERT_FALSE_MESSAGE(config::validNtpHost(host), host);
  }
  TEST_ASSERT_FALSE(config::validNtpHost(nullptr));
  TEST_ASSERT_FALSE(config::validNtpHost(std::string(64, 'a').c_str()));
  const std::string maxHost = std::string(63, 'a') + "." + std::string(63, 'b') + "." +
                            std::string(63, 'c') + "." + std::string(61, 'd');
  TEST_ASSERT_TRUE(config::validNtpHost(maxHost.c_str()));
  TEST_ASSERT_FALSE(config::validNtpHost((maxHost + "e").c_str()));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_known_pacific_frame);
  RUN_TEST(test_year_rollover_uses_next_date);
  RUN_TEST(test_leap_day_and_sunday_numbering);
  RUN_TEST(test_pst_and_pdt_offsets);
  RUN_TEST(test_spring_transition);
  RUN_TEST(test_fall_transition);
  RUN_TEST(test_change_announcement_only_preceding_hour);
  RUN_TEST(test_no_signal_without_ntp_or_mid_minute_start);
  RUN_TEST(test_full_minute_pulse_durations_and_marker);
  RUN_TEST(test_short_timer_jitter_does_not_accumulate);
  RUN_TEST(test_late_callback_abandons_frame_until_next_minute);
  RUN_TEST(test_clock_steps_in_both_directions_abandon_frame);
  RUN_TEST(test_clock_expiry_silences_output);
  RUN_TEST(test_ntp_hosts);
  return UNITY_END();
}
