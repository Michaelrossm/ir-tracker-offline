// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void startAccessPoint() {
  if (accessPointMode || !accessPointAllowed) return;
  forceFullWifiPower();
  WiFi.persistent(false);
  if (!WiFi.mode(WIFI_AP_STA)) {
    ++wifiModeErrors;
    return;
  }
  disableWifiPowerSaveForConnection();
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
  accessPointMode = false;
  accessPointStartedMs = 0;
  applyWifiPowerSave();
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
    disableWifiPowerSaveForConnection();
    WiFi.begin(config.ssid[slot].c_str(), config.password[slot].c_str());
    wifiCandidateStartedMs = millis();
    Serial.printf("Trying Wi-Fi slot %u: %s\n", slot + 1, config.ssid[slot].c_str());
    return true;
  }
  wifiCandidateStartedMs = 0;
  lastWifiAttemptMs = millis();
  return false;
}

#if IR_TRACKER_ENABLE_MDNS
void stopMdnsDiscovery() {
  if (!mdnsRunning) return;
  MDNS.end();
  mdnsRunning = false;
  mdnsAdvertisedIp = "";
  mdnsAdvertisedTransport = "";
}

bool startMdnsDiscovery() {
  if (!networkConnected()) return false;
  const String activeIp = primaryNetworkIp();
  if (!MDNS.begin(config.hostname.c_str())) {
    eventLog.add("WARN", "MDNS_FAILED", "mDNS konnte nicht gestartet werden");
    return false;
  }
  MDNS.setInstanceName(deviceIdentity.instance);
  MDNS.addService("http", "tcp", 80);
  MDNS.addServiceTxt("http", "tcp", "model", DeviceIdentity::kModel);
  MDNS.addServiceTxt("http", "tcp", "serial",
                     static_cast<const char *>(deviceIdentity.serial));
  MDNS.addService("irtracker", "tcp", 80);
  MDNS.addServiceTxt("irtracker", "tcp", "model", DeviceIdentity::kModel);
  MDNS.addServiceTxt("irtracker", "tcp", "serial",
                     static_cast<const char *>(deviceIdentity.serial));
  MDNS.addServiceTxt("irtracker", "tcp", "api", "/api/v1/meter");
  if (config.modbusTcp) {
    MDNS.addService("modbus", "tcp", 502);
    MDNS.addServiceTxt("modbus", "tcp", "schema", "irtracker.meter.v1");
    MDNS.addServiceTxt("modbus", "tcp", "model", DeviceIdentity::kModel);
  }

  // Protocol-compatible discovery with the tracker's own neutral identity.
  MDNS.addService("shelly", "tcp", 80);
  MDNS.addServiceTxt("shelly", "tcp", "id",
                     static_cast<const char *>(deviceIdentity.hostname));
  MDNS.addServiceTxt("shelly", "tcp", "model",
                     DeviceIdentity::kShellyApiModel);
  MDNS.addServiceTxt("shelly", "tcp", "gen", "2");
  MDNS.addService("everhome", "tcp", 80);
  MDNS.addServiceTxt("everhome", "tcp", "serial-number",
                     static_cast<const char *>(deviceIdentity.serial));
  MDNS.addServiceTxt("everhome", "tcp", "product", DeviceIdentity::kModel);
  MDNS.addServiceTxt("everhome", "tcp", "product-id", "IRT1000");
  MDNS.addServiceTxt("everhome", "tcp", "ip", activeIp.c_str());

  mdnsRunning = true;
  mdnsAdvertisedIp = activeIp;
  mdnsAdvertisedTransport = primaryTransportName();
  eventLog.add("INFO", "MDNS_STARTED",
               config.hostname + ".local auf " + mdnsAdvertisedTransport +
                   " (" + activeIp + ")");
  return true;
}

void syncMdnsDiscovery() {
  static uint32_t lastCheckMs = 0;
  static uint32_t lastStartAttemptMs = 0;
  if (millis() - lastCheckMs < 1000U) return;
  lastCheckMs = millis();
  if (!networkConnected()) {
    stopMdnsDiscovery();
    return;
  }
  const String activeIp = primaryNetworkIp();
  const String activeTransport = primaryTransportName();
  if (mdnsRunning && activeIp == mdnsAdvertisedIp &&
      activeTransport == mdnsAdvertisedTransport)
    return;
  const bool replacingActiveDiscovery = mdnsRunning;
  stopMdnsDiscovery();
  if (replacingActiveDiscovery || !lastStartAttemptMs ||
      millis() - lastStartAttemptMs >= 30000U) {
    lastStartAttemptMs = millis();
    if (startMdnsDiscovery()) lastStartAttemptMs = 0;
  }
}
#endif

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
  if (connected) {
    if (!wasConnected) {
      stopAccessPoint();
      wifiConnectedSinceMs = millis();
      lastWifiPowerEvaluateMs = millis();
      forceFullWifiPower();
      applyWifiPowerSave();
      accessPointAllowed = true;
      Serial.printf("Wi-Fi connected: %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      eventLog.add("INFO", "WIFI_CONNECTED",
                   WiFi.SSID() + " " + WiFi.localIP().toString());
    }
    wifiCandidateStartedMs = 0;
  } else {
    if (wasConnected) {
      forceFullWifiPower();
      disableWifiPowerSaveForConnection();
      wifiConnectedSinceMs = 0;
      lastWifiPowerEvaluateMs = 0;
      eventLog.add("WARN", "WIFI_LOST", "WLAN-Verbindung verloren");
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
#if IR_TRACKER_ENABLE_MDNS
  syncMdnsDiscovery();
#endif
}
