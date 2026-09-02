// This module is included by main.cpp inside its private namespace.
// Standalone compilation is excluded in platformio.ini to preserve behavior.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

const char *wifiTxProfileName() {
  switch (wifiTxProfile) {
    case WifiTxProfile::Medium: return "medium";
    case WifiTxProfile::Reduced: return "reduced";
    default: return "full";
  }
}

wifi_power_t wifiTxProfilePower(WifiTxProfile profile) {
  switch (profile) {
    case WifiTxProfile::Medium: return WIFI_POWER_15dBm;
    case WifiTxProfile::Reduced: return WIFI_POWER_11dBm;
    default: return WIFI_POWER_19_5dBm;
  }
}

float wifiTxPowerDbm() {
  return static_cast<int>(wifiTxProfilePower(wifiTxProfile)) / 4.0f;
}

bool setWifiTxProfile(WifiTxProfile profile) {
  if (wifiTxProfile == profile) return true;
  if (!WiFi.setTxPower(wifiTxProfilePower(profile))) {
    ++wifiTxPowerErrors;
    return false;
  }
  wifiTxProfile = profile;
  ++wifiTxPowerChanges;
  return true;
}

void forceFullWifiPower() {
  if (!setWifiTxProfile(WifiTxProfile::Full))
    wifiTxPowerRuntimeFault = true;
}

void disableWifiPowerSaveForConnection() {
  if (esp_wifi_set_ps(WIFI_PS_NONE) != ESP_OK) ++wifiModeErrors;
  wifiMinModemSleepActive = false;
}

bool applyWifiPowerSave() {
  if (accessPointMode || WiFi.status() != WL_CONNECTED ||
      !config.wifiPowerSave) {
    disableWifiPowerSaveForConnection();
    return false;
  }
  if (esp_wifi_set_ps(WIFI_PS_MIN_MODEM) == ESP_OK) {
    wifiMinModemSleepActive = true;
    return true;
  }
  ++wifiModeErrors;
  disableWifiPowerSaveForConnection();
  return false;
}

void manageAdaptiveWifiPower() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!config.ecoMode || !config.adaptiveWifiPower ||
      wifiTxPowerRuntimeFault) {
    forceFullWifiPower();
    return;
  }
  const uint32_t now = millis();
  if (!wifiConnectedSinceMs || now - wifiConnectedSinceMs < kWifiPowerStableMs ||
      now - lastWifiPowerEvaluateMs < kWifiPowerEvaluateMs)
    return;
  lastWifiPowerEvaluateMs = now;
  const int32_t rssi = WiFi.RSSI();
  WifiTxProfile target = wifiTxProfile;
  switch (wifiTxProfile) {
    case WifiTxProfile::Full:
      if (rssi >= -60) target = WifiTxProfile::Medium;
      break;
    case WifiTxProfile::Medium:
      if (rssi >= -53)
        target = WifiTxProfile::Reduced;
      else if (rssi <= -67)
        target = WifiTxProfile::Full;
      break;
    case WifiTxProfile::Reduced:
      if (rssi <= -67)
        target = WifiTxProfile::Full;
      else if (rssi <= -60)
        target = WifiTxProfile::Medium;
      break;
  }
  if (target != wifiTxProfile && !setWifiTxProfile(target)) {
    wifiTxPowerRuntimeFault = true;
    forceFullWifiPower();
  }
}

bool trackerFaultActive() {
  const bool meterFresh = meter.powerUpdatedMs &&
                          millis() - meter.powerUpdatedMs < kReadingStaleMs;
  return !meterFresh || !networkConnected() || !history.ready() ||
         heapWarningActive || cpuEcoRuntimeFault ||
         (meter.telegrams && !meter.lastCrcValid);
}

bool ecoLedSuppressed() {
  return config.ecoMode && config.ecoLedOff && !trackerFaultActive();
}

bool cpuBoostActive() {
  return config.ecoMode && !cpuEcoRuntimeFault && cpuBoostUntilMs &&
         static_cast<int32_t>(cpuBoostUntilMs - millis()) > 0;
}

uint32_t cpuBoostRemainingSeconds() {
  if (!cpuBoostActive()) return 0;
  return (static_cast<uint32_t>(cpuBoostUntilMs - millis()) + 999U) / 1000U;
}

bool switchCpuFrequency(uint32_t targetMhz) {
  if (getCpuFrequencyMhz() == targetMhz) return true;
  if (!setCpuFrequencyMhz(targetMhz)) {
    ++cpuFrequencyErrors;
    return false;
  }
  ++cpuFrequencySwitches;
  return true;
}

void disableCpuEcoForRuntime(const char *reason) {
  cpuEcoRuntimeFault = true;
  cpuBoostUntilMs = 0;
  cpuNormalUntilMs = 0;
  strlcpy(cpuBoostReason, reason, sizeof(cpuBoostReason));
  switchCpuFrequency(kPerformanceCpuMhz);
}

void requestCpuBoost(const char *reason) {
  if (!config.ecoMode || cpuEcoRuntimeFault) return;
  cpuBoostUntilMs = millis() + kCpuBoostHoldMs;
  cpuNormalUntilMs = 0;
  strlcpy(cpuBoostReason, reason, sizeof(cpuBoostReason));
  if (!switchCpuFrequency(kPerformanceCpuMhz))
    disableCpuEcoForRuntime("frequency_error");
}

void startCpuPowerMode() {
  cpuBoostUntilMs = 0;
  cpuNormalUntilMs = 0;
  if (!config.ecoMode) {
    strlcpy(cpuBoostReason, "eco_disabled", sizeof(cpuBoostReason));
    switchCpuFrequency(kPerformanceCpuMhz);
    return;
  }
  cpuNormalUntilMs = millis() + kCpuNormalHoldMs;
  strlcpy(cpuBoostReason, "eco_startup", sizeof(cpuBoostReason));
  if (!switchCpuFrequency(kEcoCpuMhz))
    disableCpuEcoForRuntime("frequency_error");
}

void manageCpuPowerMode() {
  if (!config.ecoMode || cpuEcoRuntimeFault) return;
  const uint32_t now = millis();
  if (cpuBoostUntilMs) {
    if (static_cast<int32_t>(now - cpuBoostUntilMs) < 0) return;
    cpuBoostUntilMs = 0;
    cpuNormalUntilMs = now + kCpuNormalHoldMs;
    strlcpy(cpuBoostReason, "eco_cooldown", sizeof(cpuBoostReason));
    if (!switchCpuFrequency(kEcoCpuMhz))
      disableCpuEcoForRuntime("frequency_error");
    return;
  }
  if (cpuNormalUntilMs) {
    if (static_cast<int32_t>(now - cpuNormalUntilMs) < 0) return;
    cpuNormalUntilMs = 0;
  }
  strlcpy(cpuBoostReason, "eco_idle", sizeof(cpuBoostReason));
  if (!switchCpuFrequency(kEcoCpuMhz))
    disableCpuEcoForRuntime("frequency_error");
}
