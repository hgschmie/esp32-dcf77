#pragma once
#include <stddef.h>

namespace config {
constexpr const char* kDefaultNtpServer = "pool.ntp.org";
constexpr size_t kMaxHostLength = 253;
// Bare DNS hostname or IPv4 address, without a scheme, path, port or whitespace.
bool validNtpHost(const char* host);
}  // namespace config

