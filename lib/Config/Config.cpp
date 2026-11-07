#include "Config.h"
#include <string.h>

namespace config {
bool validNtpHost(const char* host) {
  if (!host) return false;
  const size_t length = strlen(host);
  if (!length || length > kMaxHostLength) return false;
  bool onlyDigitsAndDots = true;
  size_t labelLength = 0;
  for (size_t i = 0; i < length; ++i) {
    const char c = host[i];
    if (c == '.') {
      if (!labelLength || host[i - 1] == '-') return false;
      labelLength = 0;
      continue;
    }
    const bool digit = c >= '0' && c <= '9';
    const bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    if (!digit && !letter && c != '-') return false;
    if (labelLength == 0 && c == '-') return false;
    if (++labelLength > 63) return false;
    if (!digit) onlyDigitsAndDots = false;
  }
  if (!labelLength || host[length - 1] == '-') return false;
  if (!onlyDigitsAndDots) return true;
  unsigned octet = 0, digits = 0, parts = 0;
  for (size_t i = 0; i <= length; ++i) {
    if (host[i] == '.' || host[i] == '\0') {
      if (!digits || digits > 3 || octet > 255) return false;
      ++parts;
      octet = digits = 0;
    } else {
      octet = octet * 10 + (host[i] - '0');
      ++digits;
    }
  }
  return parts == 4;
}
}  // namespace config
