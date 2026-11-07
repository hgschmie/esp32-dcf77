#include "Dcf77.h"

namespace dcf77 {
namespace {
// BCD fields and parity follow SensorsIot/DCF77-Transmitter-for-ESP32's
// CodeTime/Bin2Bcd encoder. See THIRD_PARTY.md for the source revision.
void writeBcd(Frame& frame, unsigned value, unsigned first, unsigned count) {
  unsigned bcd = ((value / 10) << 4) | (value % 10);
  for (unsigned i = 0; i < count; ++i) {
    frame.pulseMs[first + i] = (bcd & (1U << i)) ? 200 : 100;
  }
}

void writeParity(Frame& frame, unsigned first, unsigned parityBit) {
  bool odd = false;
  for (unsigned i = first; i < parityBit; ++i) odd ^= frame.pulseMs[i] == 200;
  frame.pulseMs[parityBit] = odd ? 200 : 100;
}
}  // namespace

Frame encodeMinute(time_t transmittingUtc) {
  Frame frame;
  const time_t minute = transmittingUtc - transmittingUtc % 60;
  // Advance the UTC epoch before conversion: midnight, year rollover and DST
  // all affect the date as well as the hour in the announced NEXT minute.
  frame.announcedUtc = minute + 60;
  localtime_r(&frame.announcedUtc, &frame.announcedLocal);
  tm current{}, inOneHour{};
  const time_t later = minute + 3600;
  localtime_r(&minute, &current);
  localtime_r(&later, &inOneHour);

  for (unsigned i = 0; i < 59; ++i) frame.pulseMs[i] = 100;
  frame.pulseMs[16] = (current.tm_isdst != inOneHour.tm_isdst) ? 200 : 100;
  frame.pulseMs[17] = frame.announcedLocal.tm_isdst > 0 ? 200 : 100;
  frame.pulseMs[18] = frame.announcedLocal.tm_isdst > 0 ? 100 : 200;
  frame.pulseMs[20] = 200;
  const tm& t = frame.announcedLocal;
  writeBcd(frame, t.tm_min, 21, 7);
  writeParity(frame, 21, 28);
  writeBcd(frame, t.tm_hour, 29, 6);
  writeParity(frame, 29, 35);
  writeBcd(frame, t.tm_mday, 36, 6);
  writeBcd(frame, t.tm_wday == 0 ? 7 : t.tm_wday, 42, 3);
  writeBcd(frame, t.tm_mon + 1, 45, 5);
  writeBcd(frame, (t.tm_year + 1900) % 100, 50, 8);
  writeParity(frame, 36, 58);
  frame.pulseMs[59] = 0;
  return frame;
}

Drive Modulator::update(int64_t utcUs, int64_t monotonicUs, bool clockUsable) {
  if (!clockUsable || utcUs < 0) {
    active_ = false;
    haveSample_ = false;
    return Drive::Off;
  }
  const int64_t minute = utcUs / kMinuteUs;
  const int64_t phase = utcUs % kMinuteUs;
  if (haveSample_) {
    const int64_t elapsed = monotonicUs - lastMonotonicUs_;
    const int64_t step = (utcUs - lastUtcUs_) - elapsed;
    if (elapsed < 0 || elapsed > 25000 || step > 50000 || step < -50000) {
      if (active_) ++droppedFrames_;
      active_ = false;
      blockedMinute_ = minute;
    }
  }
  lastUtcUs_ = utcUs;
  lastMonotonicUs_ = monotonicUs;
  haveSample_ = true;
  if (frameMinute_ != minute) active_ = false;
  // Never start in the middle of a minute. Nominal timer period is 5 ms.
  if (!active_ && phase < 10000 && minute != blockedMinute_) {
    frame_ = encodeMinute(static_cast<time_t>(utcUs / kSecondUs));
    frameMinute_ = minute;
    active_ = true;
  }
  if (!active_) return Drive::Off;
  const unsigned second = phase / kSecondUs;
  const int64_t withinSecond = phase % kSecondUs;
  return withinSecond < frame_.pulseMs[second] * 1000
             ? Drive::Reduced : Drive::Full;
}
}  // namespace dcf77

