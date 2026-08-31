// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void handleSetTime() {
  if (!requireAdmin()) return;
  const time_t epoch = server.arg("epoch").toInt();
  if (epoch < 1700000000) {
    server.send(400, "application/json", "{\"error\":\"invalid_time\"}");
    return;
  }
  timeval tv = {epoch, 0};
  settimeofday(&tv, nullptr);
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleInterfacesPage() {
  if (!requireAdmin()) return;
  const String host = primaryNetworkIp();
  String body = F(
      "<div class='card'><span class='status-pill'><i class='dot'></i>Nur lesende Messwertausgabe</span>"
      "<h2>Der Tracker sendet keine Sollwerte</h2>"
      "<p>Null-Einspeisung, Ladegrenzen und Zeitpläne werden ausschließlich im Speicher oder Wechselrichter eingestellt. "
      "Der IR-Tracker stellt dafür nur die gemessene Netzleistung bereit.</p></div>"
      "<div class='grid'><div class='card'><h2>Shelly-kompatibel</h2>"
      "<p>Für Speicher, die einen Shelly EM oder Shelly Pro EM als externen Zähler unterstützen.</p><code>http://");
  body += host;
  body += F("/status</code><br><code>/emeter/0</code><br><code>/rpc/EM.GetStatus?id=0</code></div>"
            "<div class='card'><h2>EcoTracker-kompatibel</h2>"
            "<p>Lokale, nur lesende EcoTracker-Messwertausgabe für kompatible Speicher und Anwendungen.</p><code>http://");
  body += host;
  body += F("/v1/json</code></div>"
            "<div class='card'><h2>Home Assistant / MQTT</h2>"
            "<p>MQTT und automatische Home-Assistant-Erkennung werden unter Einstellungen konfiguriert.</p>"
            "<a href='/setup'>MQTT konfigurieren</a></div>"
            "<div class='card'><h2>Monitoring und Export</h2>"
            "<code>/metrics</code><br><code>/openmetrics</code><br>"
            "<code>/api/v1/influx</code><br><code>/api/v1/values.csv</code></div></div>"
            "<div class='card'><h2>Sicherheitsprinzip</h2>"
            "<p>Alle hier aufgeführten Schnittstellen geben Messwerte aus. Es werden keine Register am Speicher beschrieben "
            "und keine Lade- oder Entladebefehle verschickt.</p></div>");
  server.send(200, "text/html; charset=utf-8",
              page("Schnittstellen", body));
}

String settingsBackupJson() {
  DynamicJsonDocument document(8192);
  document["format"] = "irtracker-settings";
  document["version"] = 1;
  document["firmware"] = kFirmwareVersion;
  JsonArray wifi = document.createNestedArray("wifi");
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    JsonObject network = wifi.createNestedObject();
    network["ssid"] = config.ssid[i];
    network["password"] = config.password[i];
  }
  JsonObject device = document.createNestedObject("device");
  device["hostname"] = config.hostname;
  device["rx_pin"] = config.rxPin;
  device["tx_pin"] = config.txPin;
  device["led_pin"] = config.ledPin;
  device["led_inverted"] = config.ledInverted;
  device["baud"] = config.baud;
  device["meter_protocol"] = static_cast<uint8_t>(config.meterProtocol);
  device["api_access"] = config.apiAccess;
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  device["sniffer"] = config.snifferEnabled;
  device["bridge"] = config.bridgeEnabled;
#endif
  device["timezone"] = config.timezone;
  device["setup_ap_minutes"] = config.setupApMinutes;
  device["persist_event_log"] = config.persistEventLog;
  device["eco_mode"] = config.ecoMode;
  device["eco_led_off"] = config.ecoLedOff;
  device["adaptive_wifi_power"] = config.adaptiveWifiPower;
  device["github_update_check"] = config.githubUpdateCheck;
  device["github_auto_install"] = config.githubAutoInstall;
  JsonObject mqttConfig = document.createNestedObject("mqtt");
  mqttConfig["host"] = config.mqttHost;
  mqttConfig["port"] = config.mqttPort;
  mqttConfig["user"] = config.mqttUser;
  mqttConfig["password"] = config.mqttPassword;
  mqttConfig["home_assistant_discovery"] = config.homeAssistantDiscovery;
  JsonObject pin = document.createNestedObject("meter_pin");
  pin["value"] = config.meterPin;
  pin["automatic"] = config.autoPin;
  pin["inverted"] = config.pinInverted;
  pin["pulse_ms"] = config.pinPulseMs;
  pin["digit_gap_ms"] = config.pinDigitGapMs;
  String output;
  // DE: Kompaktes JSON behaelt die Semantik und spart Uebertragung/Flash.
  // EN: Compact JSON preserves semantics and saves transfer/flash space.
  serializeJson(document, output);
  return output;
}

void handleSettingsBackup() {
  if (!requireAdmin()) return;
  server.sendHeader("Content-Disposition",
                    "attachment; filename=irtracker-settings.json");
  server.send(200, "application/json; charset=utf-8", settingsBackupJson());
}

void handleSettingsRestore() {
  if (!requireAdmin()) return;
  requestCpuBoost("settings_restore");
  if (server.arg("plain").length() > 16384) {
    server.send(413, "application/json",
                "{\"error\":\"settings_backup_too_large\"}");
    return;
  }
  DynamicJsonDocument document(8192);
  const DeserializationError error =
      deserializeJson(document, server.arg("plain"));
  if (error || document["format"] != "irtracker-settings" ||
      document["version"].as<int>() != 1) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_settings_backup\"}");
    return;
  }
  JsonArray wifi = document["wifi"].as<JsonArray>();
  if (wifi.size() != kWifiSlots) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_wifi_slots\"}");
    return;
  }
  JsonObject restoredDevice = document["device"];
  JsonObject restoredMqtt = document["mqtt"];
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    const String ssid = wifi[i]["ssid"] | "";
    const String password = wifi[i]["password"] | "";
    if (!safeSingleLine(ssid, 32) || !validWifiPassword(password)) {
      server.send(400, "application/json",
                  "{\"error\":\"invalid_wifi_credentials\"}");
      return;
    }
  }
  const String restoredHostname =
      String(restoredDevice["hostname"] | "ir-tracker");
  const String restoredTimezone = String(
      restoredDevice["timezone"] | "CET-1CEST,M3.5.0,M10.5.0/3");
  if (!validHostname(restoredHostname) ||
      !safeSingleLine(restoredTimezone, 80) ||
      !safeSingleLine(String(restoredMqtt["host"] | ""), 253) ||
      !safeSingleLine(String(restoredMqtt["user"] | ""), 128) ||
      !safeSingleLine(String(restoredMqtt["password"] | ""), 256)) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_settings_text\"}");
    return;
  }
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    config.ssid[i] = wifi[i]["ssid"] | "";
    config.password[i] = wifi[i]["password"] | "";
  }
  JsonObject device = document["device"];
  config.hostname = String(device["hostname"] | "ir-tracker");
  config.rxPin = constrain(device["rx_pin"] | 3, 0, 10);
  config.txPin = constrain(device["tx_pin"] | 6, -1, 10);
  config.ledPin = constrain(device["led_pin"] | 5, -1, 10);
  normalizeHardwarePins();
  config.ledInverted = device["led_inverted"] | true;
  config.baud = constrain(device["baud"] | 9600, 300, 115200);
  config.meterProtocol = static_cast<MeterProtocol>(constrain(
      device["meter_protocol"] | static_cast<int>(MeterProtocol::Auto), 0,
      static_cast<int>(MeterProtocol::Iec62056Active)));
  config.apiAccess = constrain(device["api_access"] | 0, 0, 2);
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  config.snifferEnabled = device["sniffer"] | false;
  config.bridgeEnabled = device["bridge"] | false;
#endif
  config.setupApMinutes =
      constrain(device["setup_ap_minutes"] | 15, 5, 60);
  config.persistEventLog = device["persist_event_log"] | false;
  config.ecoMode = device["eco_mode"] | true;
  config.ecoLedOff = device["eco_led_off"] | true;
  config.adaptiveWifiPower = device["adaptive_wifi_power"] | true;
  config.githubUpdateCheck = device["github_update_check"] | true;
  config.githubAutoInstall = device["github_auto_install"] | false;
  config.timezone = String(
      device["timezone"] | "CET-1CEST,M3.5.0,M10.5.0/3");
  JsonObject mqttConfig = document["mqtt"];
  config.mqttHost = String(mqttConfig["host"] | "");
  config.mqttPort = constrain(mqttConfig["port"] | 1883, 1, 65535);
  config.mqttUser = String(mqttConfig["user"] | "");
  config.mqttPassword = String(mqttConfig["password"] | "");
  config.homeAssistantDiscovery =
      mqttConfig["home_assistant_discovery"] | true;
  JsonObject pin = document["meter_pin"];
  config.meterPin = String(pin["value"] | "");
  if (config.meterPin.length() != 4) config.meterPin = "";
  config.autoPin = false;
  config.pinInverted = pin["inverted"] | false;
  config.pinPulseMs = constrain(pin["pulse_ms"] | 300, 50, 1000);
  config.pinDigitGapMs =
      constrain(pin["digit_gap_ms"] | 3000, 1000, 10000);
  saveConfig();
  eventLog.add("INFO", "SETTINGS_RESTORE",
               "Einstellungen wiederhergestellt");
  server.send(200, "application/json", "{\"ok\":true,\"restarting\":true}");
  delay(500);
  ESP.restart();
}

bool historyTierFromName(const String &name, HistoryStore::Tier &tier) {
  if (name == "minute")
    tier = HistoryStore::Tier::Minute;
  else if (name == "quarter")
    tier = HistoryStore::Tier::QuarterHour;
  else if (name == "hour")
    tier = HistoryStore::Tier::Hour;
  else if (name == "day")
    tier = HistoryStore::Tier::Day;
  else
    return false;
  return true;
}

void handleHistoryImportStart() {
  if (!requireAdmin()) return;
  requestCpuBoost("history_import");
  if (server.arg("plain").length() > 256) {
    server.send(413, "application/json", "{\"error\":\"request_too_large\"}");
    return;
  }
  DynamicJsonDocument document(512);
  if (deserializeJson(document, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  HistoryStore::Tier tier;
  if (!historyTierFromName(String(document["tier"] | ""), tier) ||
      !history.clear(tier)) {
    server.send(400, "application/json", "{\"error\":\"invalid_tier\"}");
    return;
  }
  eventLog.add("WARN", "HISTORY_IMPORT",
               "Historienstufe für Wiederherstellung geleert");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleHistoryImportBatch() {
  if (!requireAdmin()) return;
  requestCpuBoost("history_import");
  if (server.arg("plain").length() > 16384) {
    server.send(413, "application/json", "{\"error\":\"batch_too_large\"}");
    return;
  }
  DynamicJsonDocument document(12288);
  if (deserializeJson(document, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  HistoryStore::Tier tier;
  JsonArray values = document["values"].as<JsonArray>();
  if (!historyTierFromName(String(document["tier"] | ""), tier) ||
      values.isNull() || values.size() > 50) {
    server.send(400, "application/json", "{\"error\":\"invalid_batch\"}");
    return;
  }
  size_t imported = 0;
  for (JsonObject value : values) {
    HistoryStore::Record record = {
        value["ts"].as<uint32_t>(),
        value["avg"] | NAN,
        value["min"] | NAN,
        value["max"] | NAN,
        value["import"] | NAN,
        value["export"] | NAN};
    if (!history.importRecord(tier, record)) {
      server.send(400, "application/json",
                  "{\"error\":\"invalid_history_record\"}");
      return;
    }
    ++imported;
  }
  server.send(200, "application/json",
              "{\"ok\":true,\"imported\":" + String(imported) + "}");
}

void handleEventsJson() {
  if (!requireAdmin()) return;
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("{\"events\":[");
  bool first = true;
  String chunk;
  eventLog.forEach([&](const EventLog::Record &record) {
    if (!first) chunk += ',';
    first = false;
    chunk += "{\"ts\":" + String(record.timestamp) +
             ",\"uptime_s\":" + String(record.uptimeSeconds) +
             ",\"level\":\"" + jsonEscape(record.level) +
             "\",\"code\":\"" + jsonEscape(record.code) +
             "\",\"message\":\"" + jsonEscape(record.message) + "\"}";
    if (chunk.length() > 900) {
      server.sendContent(chunk);
      chunk = "";
    }
    return true;
  });
  if (chunk.length()) server.sendContent(chunk);
  server.sendContent("]}");
}

void handleEventsClear() {
  if (!requireAdmin()) return;
  const bool ok = eventLog.clear();
  if (ok) eventLog.add("INFO", "LOG_CLEAR", "Ereignisprotokoll gelöscht");
  server.send(ok ? 200 : 500, "application/json",
              ok ? "{\"ok\":true}" : "{\"error\":\"clear_failed\"}");
}

void handleHistoryClearAll() {
  if (!requireAdmin()) return;
  requestCpuBoost("history_clear");
  if (server.arg("confirm") != "DELETE") {
    server.send(400, "application/json",
                "{\"error\":\"confirmation_required\"}");
    return;
  }
  bool ok = true;
  ok &= history.clear(HistoryStore::Tier::Minute);
  ok &= history.clear(HistoryStore::Tier::QuarterHour);
  ok &= history.clear(HistoryStore::Tier::Hour);
  ok &= history.clear(HistoryStore::Tier::Day);
  liveWriteIndex = 0;
  liveCount = 0;
  if (ok) eventLog.add("WARN", "HISTORY_CLEAR", "Gesamte Historie gelöscht");
  server.send(ok ? 200 : 500, "application/json",
              ok ? "{\"ok\":true}" : "{\"error\":\"history_clear_failed\"}");
}
