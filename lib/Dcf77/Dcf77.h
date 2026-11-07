#pragma once
#include <stdint.h>
#include <time.h>

namespace dcf77 {
constexpr const char* kTimezoneName = "America/Los_Angeles";
// POSIX offsets have the opposite sign to GMT offsets. Current US DST rules.
constexpr const char* kTimezoneRule = "PST8PDT,M3.2.0/2,M11.1.0/2";
constexpr int64_t kSecondUs = 1000000;
constexpr int64_t kMinuteUs = 60 * kSecondUs;

struct Frame {
  uint8_t pulseMs[60]{};  // 100 = zero, 200 = one, 0 = minute marker.
  time_t announcedUtc = 0;
  tm announcedLocal{};
};

// TZ must be initialized with kTimezoneRule before using the encoder.
Frame encodeMinute(time_t transmittingUtc);

enum class Drive : uint8_t { Off, Full, Reduced };

// Called from one timer task. Wall time chooses pulse phase, avoiding accumulated
// ticker drift. Long stalls or clock steps abandon the current telegram.
class Modulator {
 public:
  Drive update(int64_t utcUs, int64_t monotonicUs, bool clockUsable);
  bool transmitting() const { return active_; }
  uint32_t droppedFrames() const { return droppedFrames_; }

 private:
  Frame frame_{};
  int64_t frameMinute_ = -1;
  int64_t blockedMinute_ = -1;
  int64_t lastUtcUs_ = 0;
  int64_t lastMonotonicUs_ = 0;
  uint32_t droppedFrames_ = 0;
  bool haveSample_ = false;
  bool active_ = false;
};
}  // namespace dcf77

