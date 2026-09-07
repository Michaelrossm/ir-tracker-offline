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
            "<p>Sendet Messwerte an einen eigenen MQTT-Broker. Mit Discovery legt Home Assistant die Sensoren automatisch an.</p>"
            "<p class='muted'>Optional: Ohne MQTT bleibt der Tracker vollständig lokal funktionsfähig.</p></div>"
            "<div class='card'><h2>Modbus TCP</h2>"
            "<p>Herstellerneutrale, ausschlie&szlig;lich lesende Messwertregister f&uuml;r lokale Energiemanagementsysteme.</p>"
            "<code>Port 502</code><br><a href='https://github.com/Michaelrossm/ir-tracker-offline/blob/main/docs/MODBUS.md' "
            "target='_blank' rel='noopener'>Registerschema &ouml;ffnen</a></div>"
            "<div class='card'><h2>Monitoring und Export</h2>"
            "<code>/metrics</code><br><code>/openmetrics</code><br>"
            "<code>/api/v1/influx</code><br><code>/api/v1/values.csv</code></div></div>"
            "<div class='card'><h2>Sicherheitsprinzip</h2>"
            "<p>Alle hier aufgeführten Schnittstellen geben Messwerte aus. Es werden keine Register am Speicher beschrieben "
            "und keine Lade- oder Entladebefehle verschickt.</p></div>");
  body += F("<form method='post' action='/interfaces/save'><input type='hidden' name='csrf_token' value='");
  body += csrfToken;
  body += F("'><fieldset><legend>Schnittstellen konfigurieren</legend>"
            "<h2>Lokale API und Kompatibilität</h2>"
            "<p class='muted'>Diese Optionen betreffen ausschließlich Messwerte im lokalen Netzwerk. Einstellungen, Wartung und Firmwareupdates bleiben immer durch die Admin-Anmeldung geschützt.</p>"
            "<label>Zugriffsmodus</label><select name='api_access'><option value='0'");
  body += config.apiAccess == 0 ? " selected" : "";
  body += F(">Lokal offen – für lokale Integrationen ohne Anmeldung</option><option value='1'");
  body += config.apiAccess == 1 ? " selected" : "";
  body += F(">Admin-Anmeldung für die eigene Messwert-API erforderlich</option><option value='2'");
  body += config.apiAccess == 2 ? " selected" : "";
  body += F(">Eigene API und Kompatibilitätsendpunkte deaktivieren</option></select>"
            "<p class='muted'>Freigegeben werden nur Messwert-API, Prometheus, Influx und CSV. Über diese Schnittstellen können keine Einstellungen verändert werden.</p>"
            "<label><input class='fit' type='checkbox' name='storage_compat' value='1'");
  body += config.storageCompatibilityMode ? " checked" : "";
  body += F("> Speicher-Kompatibilitätsmodus aktivieren</label>"
            "<p class='muted'>Ermöglicht lokalen Speichern und Wechselrichtern die ausschließlich lesenden Shelly- und EcoTracker-kompatiblen Endpunkte ohne Anmeldung. Der Tracker verwendet dabei immer seine eigene, neutrale Geräteidentität.</p>"
            "<label><input class='fit' type='checkbox' name='modbus_tcp' value='1'");
  body += config.modbusTcp ? " checked" : "";
  body += F("> Herstellerneutrales Modbus TCP aktivieren</label>"
            "<p class='muted'>Stellt das dokumentierte, nur lesende IR-Tracker-Registerschema auf Port 502 bereit. Geeignet für lokale EMS- und Automatisierungssysteme.</p>"
            "<details class='compact-details'><summary>JSON-API für Experten</summary>"
            "<p class='muted'>Stabile, herstellerneutrale Messwerte für Home Assistant, ioBroker, Node-RED, openHAB und eigene Anwendungen.</p>"
            "<code>/api/v1/meter</code><br><code>/api/v1/status</code><br><code>/api/v1/obis</code><br><code>/api/v1/history</code><br><code>/api/v1/values.csv</code></details>"
            "<h2>Home Assistant / MQTT</h2><p class='muted'>Optional. Der Broker erhält aktuelle Messwerte; mit aktivierter Discovery erscheinen die Sensoren automatisch in Home Assistant.</p>"
            "<div class='inline'><div><label>MQTT-Server</label><input name='mqtt_host' maxlength='64' placeholder='192.168.178.10' value='");
  body += htmlEscape(config.mqttHost);
  body += F("'></div><div><label>Port</label><input name='mqtt_port' type='number' min='1' max='65535' value='");
  body += String(config.mqttPort);
  body += F("'></div></div><div class='inline'><div><label>Benutzer</label><input name='mqtt_user' maxlength='64' value='");
  body += htmlEscape(config.mqttUser);
  body += F("'></div><div><label>Passwort</label><input name='mqtt_pass' type='password' maxlength='64' autocomplete='new-password' placeholder='");
  body += config.mqttPassword.length() ? "gespeichert" : "optional";
  body += F("'></div></div><label><input class='fit' type='checkbox' name='ha_disc' value='1'");
  body += config.homeAssistantDiscovery ? " checked" : "";
  body += F("> Home-Assistant-Discovery aktivieren</label>"
            "<h2>Ereignisprotokoll</h2><label><input class='fit' type='checkbox' name='event_flash' value='1'");
  body += config.persistEventLog ? " checked" : "";
  body += F("> Ereignis- und Fehlerprotokoll dauerhaft im Flash speichern</label>"
            "<p class='muted'>Standard: maximal 256 Einträge im RAM; ein Neustart leert sie. Dauerhaftes Speichern ist für Fehlersuche sinnvoll, erzeugt aber zusätzliche Flash-Schreibvorgänge.</p>"
            "</fieldset><button type='submit'>Schnittstellen speichern</button></form>");
  server.send(200, "text/html; charset=utf-8",
              page("Schnittstellen", body));
}

void handleInterfacesSave() {
  if (!requireAdmin()) return;
  config.apiAccess = constrain(server.arg("api_access").toInt(), 0, 2);
  config.storageCompatibilityMode = server.hasArg("storage_compat");
  config.modbusTcp = server.hasArg("modbus_tcp");
  const bool previousEventPersistence = config.persistEventLog;
  config.persistEventLog = server.hasArg("event_flash");
  config.mqttHost = server.arg("mqtt_host");
  config.mqttHost.trim();
  config.mqttPort = constrain(server.arg("mqtt_port").toInt(), 1, 65535);
  config.mqttUser = server.arg("mqtt_user");
  const String newMqttPassword = server.arg("mqtt_pass");
  if (newMqttPassword.length() || !config.mqttHost.length())
    config.mqttPassword = newMqttPassword;
  config.homeAssistantDiscovery = server.hasArg("ha_disc");
  saveConfig();
  if (previousEventPersistence != config.persistEventLog &&
      !eventLog.setPersistence(config.persistEventLog)) {
    config.persistEventLog = previousEventPersistence;
    saveConfig();
    server.send(500, "application/json",
                "{\"error\":\"event_log_persistence_change_failed\"}");
    return;
  }
  eventLog.add("INFO", "INTERFACES_SAVE", "Schnittstellen gespeichert");
  server.send(200, "text/html; charset=utf-8",
              page("Gespeichert", "<p>Schnittstellen gespeichert. Der Tracker startet jetzt neu.</p>"));
  delay(750);
  ESP.restart();
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
  device["storage_compatibility"] = config.storageCompatibilityMode;
  device["modbus_tcp"] = config.modbusTcp;
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
  device["wifi_power_save"] = config.wifiPowerSave;
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
      String(restoredDevice["hostname"] | deviceIdentity.hostname);
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
  config.hostname = String(device["hostname"] | deviceIdentity.hostname);
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
  config.storageCompatibilityMode = device.containsKey("storage_compatibility")
                                        ? device["storage_compatibility"].as<bool>()
                                        : config.apiAccess == 0;
  config.modbusTcp = device["modbus_tcp"] | false;
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
  config.wifiPowerSave = device["wifi_power_save"] | false;
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
