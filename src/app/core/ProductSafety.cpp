// ProductSafety.cpp
// Hardened boot-loop protection for IR Tracker Offline 2.0.0.
// Included from main.cpp inside the existing anonymous namespace.
// Never formats or clears History/NVS.

#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

RTC_DATA_ATTR static uint32_t irSafeMagic = 0;
RTC_DATA_ATTR static uint8_t irEarlyCrashCount = 0;
RTC_DATA_ATTR static uint8_t irPreviousBootHealthy = 0;

constexpr uint32_t kIrSafeMagic = 0x49525346; // IRSF
constexpr uint8_t kIrEarlyCrashLimit = 3;
constexpr uint32_t kIrHealthyBootMs = 60UL * 1000UL;

struct ProductSafetyState {
  bool safeRecovery = false;
  bool healthyMarked = false;
  uint8_t earlyCrashCount = 0;
  esp_reset_reason_t resetReason = ESP_RST_UNKNOWN;
} productSafety;

constexpr bool productCrashLikeReset(const esp_reset_reason_t reason) {
  return reason == ESP_RST_PANIC ||
         reason == ESP_RST_INT_WDT ||
         reason == ESP_RST_TASK_WDT ||
         reason == ESP_RST_WDT ||
         reason == ESP_RST_BROWNOUT;
}

void beginProductSafety(const esp_reset_reason_t reason) {
  productSafety = ProductSafetyState{};
  productSafety.resetReason = reason;

  if (irSafeMagic != kIrSafeMagic) {
    irSafeMagic = kIrSafeMagic;
    irEarlyCrashCount = 0;
    irPreviousBootHealthy = 0;
  }

  if (productCrashLikeReset(reason)) {
    if (!irPreviousBootHealthy && irEarlyCrashCount < UINT8_MAX)
      ++irEarlyCrashCount;
  } else {
    // Intentional OTA/software reset must never count as a crash loop.
    irEarlyCrashCount = 0;
  }

  irPreviousBootHealthy = 0;
  productSafety.earlyCrashCount = irEarlyCrashCount;
  productSafety.safeRecovery = irEarlyCrashCount >= kIrEarlyCrashLimit;
}

void logProductSafetyBoot() {
  if (productSafety.safeRecovery) {
    eventLog.add(
        "ERROR", "SAFE_RECOVERY",
        "Sicherer Wiederherstellungsmodus nach " +
            String(productSafety.earlyCrashCount) +
            " fruehen Fehlerstarts. Automatische Updates und History-Migration "
            "sind fuer diese Laufzeit gesperrt.");
  } else if (productSafety.earlyCrashCount) {
    eventLog.add("WARN", "EARLY_CRASH_BOOT",
                 "Frueher Fehlerstart erkannt: " +
                     String(productSafety.earlyCrashCount) + "/" +
                     String(kIrEarlyCrashLimit));
  }
}

void manageProductSafety() {
  if (productSafety.healthyMarked || millis() < kIrHealthyBootMs) return;

  // Safe recovery remains active until an intentional restart.
  irPreviousBootHealthy = 1;
  irEarlyCrashCount = 0;
  productSafety.healthyMarked = true;
  eventLog.add("INFO", "BOOT_HEALTHY",
               "Startphase seit 60 Sekunden stabil; Bootloop-Zaehler geloescht");
}

bool productSafeRecoveryActive() { return productSafety.safeRecovery; }
bool productHistoryMigrationAllowed() { return !productSafety.safeRecovery; }
bool productAutomaticUpdateAllowed() { return !productSafety.safeRecovery; }

String productSafetyJson() {
  String json;
  json.reserve(256);
  json = "{\"safe_recovery\":";
  json += productSafety.safeRecovery ? "true" : "false";
  json += ",\"early_crash_count\":" + String(productSafety.earlyCrashCount);
  json += ",\"healthy_marked\":";
  json += productSafety.healthyMarked ? "true" : "false";
  json += ",\"history_migration_allowed\":";
  json += productHistoryMigrationAllowed() ? "true" : "false";
  json += ",\"automatic_update_allowed\":";
  json += productAutomaticUpdateAllowed() ? "true" : "false";
  json += "}";
  return json;
}
