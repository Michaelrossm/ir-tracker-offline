// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void startAccessPoint() {
  if (accessPointMode || !accessPointAllowed) return;
  forceFullWifiPower();
  wifiMinModemSleepActive = false;
  WiFi.persistent(false);
  if (!WiFi.mode(WIFI_AP_STA)) {
    ++wifiModeErrors;
    return;
  }
  const uint64_t chip = ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X", static_cast<unsigned>(chip & 0xffff));
  String apName = "IR-Tracker-Setup-" + String(suffix);
  String apPassword = localAdminPassword();
  if (!WiFi.softAP(apName.c_str(), apPassword.c_str())) {
    ++wifiModeErrors;
    return;
  }
  accessPointMode = true;
  accessPointStartedMs = millis();
  Serial.printf("Setup AP started: %s\n", apName.c_str());
  dns.start(53, "*", WiFi.softAPIP());
}

void stopAccessPoint() {
  if (!accessPointMode) return;
  dns.stop();
  if (!WiFi.softAPdisconnect(true)) ++wifiModeErrors;
  if (!WiFi.mode(WIFI_STA)) ++wifiModeErrors;
  wifiMinModemSleepActive =
      esp_wifi_set_ps(WIFI_PS_MIN_MODEM) == ESP_OK;
  if (!wifiMinModemSleepActive) ++wifiModeErrors;
  accessPointMode = false;
  accessPointStartedMs = 0;
}

bool beginNextKnownWifi() {
  while (wifiTried < kWifiSlots) {
    const uint8_t slot = wifiCandidate;
    wifiCandidate = (wifiCandidate + 1) % kWifiSlots;
    ++wifiTried;
    if (!config.ssid[slot].length()) continue;
    // Association, DHCP and TLS setup are deliberately performed at the
    // performance clock. Eco mode resumes two minutes after the last attempt.
    requestCpuBoost("wifi_connect");
    WiFi.begin(config.ssid[slot].c_str(), config.password[slot].c_str());
    wifiCandidateStartedMs = millis();
    Serial.printf("Trying Wi-Fi slot %u: %s\n", slot + 1, config.ssid[slot].c_str());
    return true;
  }
  wifiCandidateStartedMs = 0;
  lastWifiAttemptMs = millis();
  return false;
}

void manageWifi() {
  static bool wasConnected = false;
  static bool wasEthernetConnected = false;
  static bool previousPrimaryEthernet = false;
  const bool connected = WiFi.status() == WL_CONNECTED;
  const bool ethernetConnected = ethernet.connected();
  const bool anyNetworkConnected = connected || ethernetConnected;

  if (ethernetConnected != wasEthernetConnected) {
    if (ethernetConnected) {
      eventLog.add("INFO", "ETHERNET_CONNECTED",
                   "LAN verbunden: " + ethernet.localIP().toString());
      stopAccessPoint();
    } else if (wasEthernetConnected) {
      requestCpuBoost("lan_fallback");
      eventLog.add("WARN", "ETHERNET_LOST",
                   "LAN-Verbindung verloren, WLAN übernimmt");
    }
  }

  // Existing TCP sessions cannot migrate between interfaces. Reconnect MQTT
  // once when the preferred route changes; all other requests are short-lived.
  if (ethernetConnected != previousPrimaryEthernet) {
    if (mqtt.connected()) mqtt.disconnect();
    mqttNetwork.stop();
    lastMqttAttemptMs = 0;
    previousPrimaryEthernet = ethernetConnected;
  }

  if (anyNetworkConnected && !ntpConfigured) {
    configTzTime(config.timezone.c_str(), "fritz.box", "pool.ntp.org",
                 "time.cloudflare.com");
    ntpConfigured = true;
  }
#if IR_TRACKER_ENABLE_MDNS
  if (anyNetworkConnected && !mdnsRunning) {
    if (MDNS.begin(config.hostname.c_str())) {
      MDNS.addService("http", "tcp", 80);
      mdnsRunning = true;
      eventLog.add("INFO", "MDNS_STARTED",
                   "Tracker erreichbar als " + config.hostname + ".local");
    } else {
      eventLog.add("WARN", "MDNS_FAILED",
                   "mDNS konnte nicht gestartet werden");
    }
  }
#endif
  if (connected) {
    if (!wasConnected) {
      stopAccessPoint();
      wifiConnectedSinceMs = millis();
      lastWifiPowerEvaluateMs = millis();
      forceFullWifiPower();
      accessPointAllowed = true;
      Serial.printf("Wi-Fi connected: %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      eventLog.add("INFO", "WIFI_CONNECTED",
                   WiFi.SSID() + " " + WiFi.localIP().toString());
    }
    wifiCandidateStartedMs = 0;
  } else {
    if (wasConnected) {
      forceFullWifiPower();
      wifiConnectedSinceMs = 0;
      lastWifiPowerEvaluateMs = 0;
      wifiMinModemSleepActive = false;
      eventLog.add("WARN", "WIFI_LOST", "WLAN-Verbindung verloren");
#if IR_TRACKER_ENABLE_MDNS
      if (mdnsRunning && !ethernetConnected) {
        MDNS.end();
        mdnsRunning = false;
      }
#endif
      wifiTried = 0;
      wifiCandidate = 0;
      wifiCandidateStartedMs = 0;
    }
    if (!ethernetConnected) startAccessPoint();
    else stopAccessPoint();
    if (accessPointMode && accessPointStartedMs &&
        millis() - accessPointStartedMs >=
            static_cast<uint32_t>(config.setupApMinutes) * 60000UL) {
      eventLog.add("INFO", "SETUP_AP_TIMEOUT",
                   "Setup-Hotspot nach Zeitlimit abgeschaltet");
      accessPointAllowed = false;
      stopAccessPoint();
    }
    if (wifiCandidateStartedMs && millis() - wifiCandidateStartedMs >= kWifiPerNetworkMs) {
      beginNextKnownWifi();
    } else if (!wifiCandidateStartedMs &&
               (!lastWifiAttemptMs || millis() - lastWifiAttemptMs >= kWifiRetryMs)) {
      wifiTried = 0;
      beginNextKnownWifi();
    }
  }
  wasConnected = connected;
  wasEthernetConnected = ethernetConnected;
}
