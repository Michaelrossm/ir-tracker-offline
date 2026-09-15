#pragma once
// Host-only shim. Firmware builds use the framework's Arduino.h.
#include <cstddef>
#include <cstdint>
#include <string>
#include <cstring>
using String = std::string;
inline uint32_t millis() { return 10000; }
inline void delay(unsigned) {}
inline size_t strlcpy(char *out, const char *in, size_t capacity) {
  const size_t length = strlen(in);
  if (capacity) {
    const size_t count = length < capacity ? length : capacity - 1;
    memcpy(out, in, count);
    out[count] = 0;
  }
  return length;
}
