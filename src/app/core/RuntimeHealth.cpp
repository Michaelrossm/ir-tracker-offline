// RuntimeHealth.cpp
// Existing main.cpp helpers moved here without changing behavior.
// Keeping them in the amalgamated build preserves cross-module optimization.

#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void updateLed() {
  static uint32_t lastToggle = 0;
  static bool state = false;

  if (gpioScan.active) return;

#if IR_TRACKER_ENABLE_FACTORY_TEST
  if ((factoryTest.running || factoryTest.finished) &&
      !factoryTest.ledConfirmed)
    return;
#endif

  if (config.ledPin < 0) return;

  if (ecoLedSuppressed()) {
    if (state) {
      state = false;
      digitalWrite(config.ledPin, config.ledInverted);
    }
    return;
  }

  const uint32_t intervalMs = trackerFaultActive() ? 150U : 1000U;
  if (millis() - lastToggle < intervalMs) return;

  lastToggle = millis();
  state = !state;
  digitalWrite(config.ledPin, state ^ config.ledInverted);
}

void monitorHeap() {
  static uint32_t lastCheckMs = 0;
  if (millis() - lastCheckMs < 30000) return;
  lastCheckMs = millis();

  const uint32_t freeHeap = ESP.getFreeHeap();
  const bool low = freeHeap < kHeapWarningBytes;

  if (low && !heapWarningActive) {
    eventLog.add("WARN", "HEAP_LOW",
                 "Freier RAM unter Sicherheitsgrenze: " +
                     String(freeHeap) + " Bytes");
  } else if (!low && heapWarningActive) {
    eventLog.add("INFO", "HEAP_RECOVERED",
                 "Freier RAM wieder im sicheren Bereich");
  }

  heapWarningActive = low;
}
