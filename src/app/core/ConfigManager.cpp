// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void createCsrfToken() {
  uint8_t randomBytes[32];
  esp_fill_random(randomBytes, sizeof(randomBytes));
  static const char hex[] = "0123456789abcdef";
  csrfToken = "";
  csrfToken.reserve(64);
  for (uint8_t value : randomBytes) {
    csrfToken += hex[value >> 4];
    csrfToken += hex[value & 0x0f];
  }
  memset(randomBytes, 0, sizeof(randomBytes));
}

bool trackerGpioAvailable(const int pin) {
  return HardwareProfile::trackerGpioAvailable(
      pin, ethernet.hardwareDetected());
}

void normalizeHardwarePins() {
  if (!trackerGpioAvailable(config.rxPin)) config.rxPin = kDefaultRxPin;
  if (config.txPin >= 0 && !trackerGpioAvailable(config.txPin))
    config.txPin = kDefaultTxPin;
  if (config.ledPin >= 0 && !trackerGpioAvailable(config.ledPin))
    config.ledPin = 5;
}

void loadConfig() {
  prefs.begin("offline", true);
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    config.ssid[i] = prefs.getString(("ssid" + String(i)).c_str(), "");
    config.password[i] = prefs.getString(("pass" + String(i)).c_str(), "");
  }
  // DE: Migration von Firmware 0.1/0.2. | EN: Migration from firmware 0.1/0.2.
  if (!config.ssid[0].length()) {
    config.ssid[0] = prefs.getString("ssid", "");
    config.password[0] = prefs.getString("password", "");
  }
  config.hostname = prefs.getString("hostname", deviceIdentity.hostname);
  if (config.hostname == "ir-tracker") config.hostname = deviceIdentity.hostname;
  config.rxPin = prefs.getUChar("rx_pin", kDefaultRxPin);
  config.txPin = prefs.getChar("tx_pin", kDefaultTxPin);
  config.ledPin = prefs.getChar("led_pin", 5);
  config.ledInverted = prefs.getBool("led_inv", true);
  config.baud = prefs.getULong("baud", kDefaultBaud);
  config.meterProtocol = static_cast<MeterProtocol>(
      prefs.getUChar("meter_proto", static_cast<uint8_t>(MeterProtocol::Auto)));
  config.mqttHost = prefs.getString("mqtt_host", "");
  config.mqttPort = prefs.getUShort("mqtt_port", 1883);
  config.mqttUser = prefs.getString("mqtt_user", "");
  config.mqttPassword = prefs.getString("mqtt_pass", "");
  config.homeAssistantDiscovery = prefs.getBool("ha_disc", true);
  config.apiAccess = prefs.getUChar("api_access", 0);
  // Fresh installations are secure by default. Existing installations that
  // deliberately used the old locally-open API retain their read-only storage
  // integration until the new dedicated switch is changed once.
  config.storageCompatibilityMode =
      prefs.isKey("storage_compat")
          ? prefs.getBool("storage_compat", false)
          : (prefs.isKey("api_access") && config.apiAccess == 0);
  config.modbusTcp = prefs.getBool("modbus_tcp", false);
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  config.snifferEnabled = prefs.getBool("sniffer", false);
  config.bridgeEnabled = prefs.getBool("bridge", false);
#endif
  config.meterPin = prefs.getString("meter_pin", "");
  config.autoPin = prefs.getBool("auto_pin", false);
  config.pinInverted = prefs.getBool("pin_inv", false);
  config.pinPulseMs = prefs.getUShort("pin_pulse", 300);
  config.pinDigitGapMs = prefs.getUShort("pin_gap", 3000);
  config.adminPassword = prefs.getString("admin_pass", "");
  config.timezone = prefs.getString(
      "timezone", "CET-1CEST,M3.5.0,M10.5.0/3");
  config.setupApMinutes = prefs.getUShort("ap_minutes", 15);
  config.persistEventLog = prefs.getBool("event_flash", false);
  config.ecoMode = prefs.getBool("eco_mode", true);
  config.ecoLedOff = prefs.getBool("eco_led_off", true);
  config.adaptiveWifiPower = prefs.getBool("wifi_power_auto", true);
  config.wifiPowerSave = prefs.getBool("wifi_ps", false);
  config.githubUpdateCheck = prefs.getBool("gh_check", true);
  config.githubAutoInstall = prefs.getBool("gh_auto", false);
  prefs.end();
  if (config.rxPin > 10) config.rxPin = kDefaultRxPin;
  if (config.txPin > 10) config.txPin = -1;
  if (config.ledPin > 10) config.ledPin = -1;
  normalizeHardwarePins();
  if (static_cast<uint8_t>(config.meterProtocol) >
      static_cast<uint8_t>(MeterProtocol::Iec62056Active))
    config.meterProtocol = MeterProtocol::Auto;
  if (config.baud < 300 || config.baud > 115200) config.baud = kDefaultBaud;
  if (config.meterPin.length() != 4) {
    config.meterPin = "";
  }
  // DE: Beim LEPUS erfolgt die PIN-Eingabe per Taste/Taschenlampe; automatische
  // IR-Folgen bleiben wegen fehlender sicherer Bestätigung aus. | EN: LEPUS PIN
  // entry uses its button/flashlight; automatic IR sequences stay disabled
  // because the meter does not acknowledge them reliably.
  config.autoPin = false;
  config.apiAccess = constrain(config.apiAccess, 0, 2);
  config.setupApMinutes = constrain(config.setupApMinutes, 5, 60);
  config.pinPulseMs = constrain(config.pinPulseMs, 50, 1000);
  config.pinDigitGapMs = constrain(config.pinDigitGapMs, 1000, 10000);
  if (config.adminPassword.length() && config.adminPassword.length() < 4)
    config.adminPassword = "";
  if (!config.timezone.length() || config.timezone.length() > 80)
    config.timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
}

void saveConfig() {
  prefs.begin("offline", false);
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    prefs.putString(("ssid" + String(i)).c_str(), config.ssid[i]);
    prefs.putString(("pass" + String(i)).c_str(), config.password[i]);
  }
  prefs.putString("hostname", config.hostname);
  prefs.putUChar("rx_pin", config.rxPin);
  prefs.putChar("tx_pin", config.txPin);
  prefs.putChar("led_pin", config.ledPin);
  prefs.putBool("led_inv", config.ledInverted);
  prefs.putULong("baud", config.baud);
  prefs.putUChar("meter_proto", static_cast<uint8_t>(config.meterProtocol));
  prefs.putString("mqtt_host", config.mqttHost);
  prefs.putUShort("mqtt_port", config.mqttPort);
  prefs.putString("mqtt_user", config.mqttUser);
  prefs.putString("mqtt_pass", config.mqttPassword);
  prefs.putBool("ha_disc", config.homeAssistantDiscovery);
  prefs.putUChar("api_access", config.apiAccess);
  prefs.putBool("storage_compat", config.storageCompatibilityMode);
  prefs.putBool("modbus_tcp", config.modbusTcp);
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  prefs.putBool("sniffer", config.snifferEnabled);
  prefs.putBool("bridge", config.bridgeEnabled);
#endif
  prefs.putString("meter_pin", config.meterPin);
  prefs.putBool("auto_pin", config.autoPin);
  prefs.putBool("pin_inv", config.pinInverted);
  prefs.putUShort("pin_pulse", config.pinPulseMs);
  prefs.putUShort("pin_gap", config.pinDigitGapMs);
  prefs.putString("admin_pass", config.adminPassword);
  prefs.putString("timezone", config.timezone);
  prefs.putUShort("ap_minutes", config.setupApMinutes);
  prefs.putBool("event_flash", config.persistEventLog);
  prefs.putBool("eco_mode", config.ecoMode);
  prefs.putBool("eco_led_off", config.ecoLedOff);
  prefs.putBool("wifi_power_auto", config.adaptiveWifiPower);
  prefs.putBool("wifi_ps", config.wifiPowerSave);
  prefs.putBool("gh_check", config.githubUpdateCheck);
  prefs.putBool("gh_auto", config.githubAutoInstall);
  prefs.end();
}
