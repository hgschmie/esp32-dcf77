#pragma once
#include <stdint.h>

namespace board {
// SuperMini header GPIOs. GPIO15 from the original ESP32 sketch is flash on C3.
constexpr uint8_t kAntenna = 4;
constexpr uint8_t kLed = 5;  // External active-high LED; the onboard LED is GPIO8.
constexpr uint8_t kBoot = 9;
constexpr uint32_t kBootHoldMs = 5000;
constexpr uint32_t kCarrierHz = 77500;
constexpr uint8_t kFullDuty = 32;  // 50% of a 6-bit PWM period.
constexpr uint8_t kReducedDuty = 3;  // ~14.7% fundamental through a tuned antenna.
static_assert(kAntenna != kLed && kAntenna != kBoot, "GPIOs must be distinct");
}  // namespace board

