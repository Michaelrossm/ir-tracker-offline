// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void formatNumberOrNull(char *destination, size_t capacity, double value,
                        uint8_t decimals = 3) {
  if (!capacity) return;
  if (std::isfinite(value))
    snprintf(destination, capacity, "%.*f", static_cast<int>(decimals), value);
  else
    strlcpy(destination, "null", capacity);
}

String numberOrNull(double value, uint8_t decimals = 3) {
  char formatted[40];
  formatNumberOrNull(formatted, sizeof(formatted), value, decimals);
  return String(formatted);
}

uint32_t valueAgeSeconds(uint32_t updatedMs) {
  return updatedMs ? (millis() - updatedMs) / 1000U : UINT32_MAX;
}

String ageOrNull(uint32_t updatedMs) {
  return updatedMs ? String(valueAgeSeconds(updatedMs)) : "null";
}

bool valueFresh(uint32_t updatedMs) {
  return updatedMs && millis() - updatedMs < kReadingStaleMs;
}

uint32_t meterSerialMode() {
  return config.meterProtocol == MeterProtocol::Iec62056 ||
                 config.meterProtocol == MeterProtocol::Iec62056Active
             ? SERIAL_7E1
             : SERIAL_8N1;
}

uint32_t meterBaud() {
  return config.meterProtocol == MeterProtocol::Iec62056Active ? 300U
                                                               : config.baud;
}

String resetReasonText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power_on";
    case ESP_RST_EXT: return "external";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_watchdog";
    case ESP_RST_TASK_WDT: return "task_watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep_sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    default: return "unknown";
  }
}
