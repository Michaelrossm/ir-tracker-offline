// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void handleSafeShutdown() {
  if (!requireAdmin()) return;
  if (server.arg("confirm") != "SHUTDOWN") {
    server.send(400, "application/json",
                "{\"error\":\"shutdown_confirmation_required\"}");
    return;
  }
  if (!history.flushPending(HistoryStore::Tier::Minute)) {
    eventLog.add("ERROR", "SHUTDOWN_ABORT",
                 "Herunterfahren wegen Speicherfehler abgebrochen");
    server.send(500, "application/json",
                "{\"error\":\"history_flush_failed\"}");
    return;
  }
  irPulse.active = false;
  apatorUnlock.active = false;
  eventLog.add("INFO", "SAFE_SHUTDOWN",
               "Minutenpuffer gespeichert, Tiefschlaf wird gestartet");
  server.send(200, "application/json",
              "{\"ok\":true,\"state\":\"deep_sleep\","
              "\"wake\":\"power_cycle_or_reset\"}");
  delay(600);
  if (mqtt.connected()) mqtt.disconnect();
  dns.stop();
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
  meterSerial.end();
  if (config.txPin >= 0) {
    pinMode(config.txPin, OUTPUT);
    digitalWrite(config.txPin, config.pinInverted);
  }
  if (config.ledPin >= 0)
    digitalWrite(config.ledPin, config.ledInverted ? HIGH : LOW);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_deep_sleep_start();
}

void handleMaintenancePage() {
  if (!requireAdmin()) return;
  String body = maintenanceTabs(false);
  body += F(
      "<div class='grid'><div class='card'><h2>Einstellungen</h2>"
      "<p>Enthält WLAN-, MQTT-, PIN- und Geräteprofil-Daten. Die Datei enthält Geheimnisse und muss sicher aufbewahrt werden.</p>"
      "<button id='fullBackup'>Vollständiges Backup erstellen</button>"
      "<button class='secondary' id='settingsExport'>Nur Einstellungen herunterladen</button>"
      "<label>Backup wiederherstellen</label><input id='settingsFile' type='file' accept='.json,application/json'>"
      "<button id='settingsImport'>Einstellungen prüfen und wiederherstellen</button></div>"
      "<div class='card'><h2>Historie</h2><p>Alle vier Ringpuffer werden als eine JSON-Datei im Browser zusammengeführt.</p>"
      "<button id='historyExport'>Historie herunterladen</button>"
      "<label>Historienbackup</label><input id='historyFile' type='file' accept='.json,application/json'>"
      "<button id='historyImport'>Historie gestaffelt wiederherstellen</button>"
      "<button class='danger' id='historyClear'>Gesamte Historie löschen</button></div></div>"
      "<div class='card'><h2>Custom-Firmware aktualisieren</h2>"
      "<p>Es werden ausschließlich kryptografisch signierte IRFW-Pakete von Michael Roßmann akzeptiert. "
      "Signatur und ESP32-Image werden vor der Aktivierung geprüft.</p>"
      "<form method='post' action='/system/update' enctype='multipart/form-data' "
      "onsubmit=\"return confirm('Firmware installieren und Tracker neu starten?')\">"
      "<label>Signiertes Firmwarepaket (.irfw)</label><input type='file' name='firmware' "
      "accept='.irfw,application/octet-stream' required>"
      "<button type='submit'>WLAN-Update installieren</button></form></div>"
      "<div class='card' id='firmware-update'><h2>GitHub-Firmwareupdate</h2>"
      "<p>Prüft das offizielle Projekt auf eine neuere Version. Installiert werden "
      "ausschließlich passend signierte IRFW-Pakete; ein Downgrade ist gesperrt.</p>"
      "<div class='grid'><div><span class='muted'>Installierte Version</span><br><strong id='updateCurrent'>–</strong></div>"
      "<div><span class='muted'>Verfügbare Version</span><br><strong id='updateAvailable'>–</strong></div>"
      "<div><span class='muted'>Letzte erfolgreiche Prüfung</span><br><strong id='updateLast'>Noch nicht geprüft</strong></div></div>"
      "<p id='updateState' class='muted'>Status wird geladen …</p>"
      "<details id='updateError' hidden><summary>Technischer Fehlercode</summary><code id='updateErrorCode'></code></details>"
      "<div class='actions'><form method='post' action='/api/v1/update/check'><button type='submit'>Jetzt prüfen</button></form>"
      "<form id='updateInstall' method='post' action='/api/v1/update/install' hidden "
      "onsubmit=\"return confirm('Signiertes GitHub-Update installieren und neu starten?')\">"
      "<button class='secondary' type='submit'>Gefundenes Update installieren</button></form></div></div>"
      "<div class='card'><h2>Tracker sicher ausschalten</h2>"
      "<p>Speichert den offenen Minutenblock und versetzt den ESP32 danach in Tiefschlaf. "
      "Zum Wiedereinschalten Strom kurz aus- und einschalten oder Reset betätigen.</p>"
      "<button class='danger' id='safeShutdown' type='button'>Sicher herunterfahren</button></div>"
      "<div class='card'><h2>Ereignis- und Fehlerprotokoll</h2>"
      "<button id='eventsReload'>Protokoll laden</button><button class='danger' id='eventsClear'>Protokoll löschen</button>"
      "<pre id='events' style='white-space:pre-wrap;max-height:420px;overflow:auto'></pre></div>"
      "<p id='maintenanceStatus' class='muted'></p>");
  server.send(200, "text/html; charset=utf-8",
              page("Backup und Wartung", body, "",
                   String("/assets/maintenance.js?v=") + kFirmwareVersion));
}

void handleSetup() {
  if (!requireAdmin()) return;
  String setupConfig;
  setupConfig.reserve(1800);
  setupConfig = F("window.IR_TRACKER_SETUP={\"ssids\":[");
  for (uint8_t index = 0; index < kWifiSlots; ++index) {
    if (index) setupConfig += ',';
    setupConfig += "\"" + jsonEscape(config.ssid[index]) + "\"";
  }
  setupConfig += F("],\"gpios\":[");
  bool firstPin = true;
  for (int pin = 0; pin <= 10; ++pin) {
    if (!trackerGpioAvailable(pin)) continue;
    if (!firstPin) setupConfig += ',';
    setupConfig += String(pin);
    firstPin = false;
  }
  setupConfig += F("],\"hostname\":\"");
  setupConfig += jsonEscape(config.hostname);
  setupConfig += F("\",\"timezone\":\"");
  setupConfig += jsonEscape(config.timezone);
  setupConfig += F("\",\"ap_minutes\":");
  setupConfig += String(config.setupApMinutes);
  setupConfig += F(",\"rx_pin\":");
  setupConfig += String(config.rxPin);
  setupConfig += F(",\"tx_pin\":");
  setupConfig += String(config.txPin);
  setupConfig += F(",\"led_pin\":");
  setupConfig += String(config.ledPin);
  setupConfig += F(",\"led_inv\":");
  setupConfig += config.ledInverted ? "true" : "false";
  setupConfig += F(",\"meter_protocol\":");
  setupConfig += String(static_cast<uint8_t>(config.meterProtocol));
  setupConfig += F(",\"baud\":");
  setupConfig += String(config.baud);
  setupConfig += F(",\"api_access\":");
  setupConfig += String(config.apiAccess);
  setupConfig += F(",\"storage_compat\":");
  setupConfig += config.storageCompatibilityMode ? "true" : "false";
  setupConfig += F(",\"modbus_tcp\":");
  setupConfig += config.modbusTcp ? "true" : "false";
  setupConfig += F(",\"event_flash\":");
  setupConfig += config.persistEventLog ? "true" : "false";
  setupConfig += F(",\"mqtt_host\":\"");
  setupConfig += jsonEscape(config.mqttHost);
  setupConfig += F("\",\"mqtt_port\":");
  setupConfig += String(config.mqttPort);
  setupConfig += F(",\"mqtt_user\":\"");
  setupConfig += jsonEscape(config.mqttUser);
  setupConfig += F("\",\"mqtt_password_saved\":");
  setupConfig += config.mqttPassword.length() ? "true" : "false";
  setupConfig += F(",\"ha_disc\":");
  setupConfig += config.homeAssistantDiscovery ? "true" : "false";
  setupConfig += F(",\"eco_mode\":");
  setupConfig += config.ecoMode ? "true" : "false";
  setupConfig += F(",\"eco_led_off\":");
  setupConfig += config.ecoLedOff ? "true" : "false";
  setupConfig += F(",\"wifi_power_auto\":");
  setupConfig += config.adaptiveWifiPower ? "true" : "false";
  setupConfig += F(",\"wifi_ps\":");
  setupConfig += config.wifiPowerSave ? "true" : "false";
  setupConfig += F(",\"gh_check\":");
  setupConfig += config.githubUpdateCheck ? "true" : "false";
  setupConfig += F(",\"gh_auto\":");
  setupConfig += config.githubAutoInstall ? "true" : "false";
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  setupConfig += F(",\"developer_io\":true,\"sniffer\":");
  setupConfig += config.snifferEnabled ? "true" : "false";
  setupConfig += F(",\"bridge\":");
  setupConfig += config.bridgeEnabled ? "true" : "false";
#else
  setupConfig += F(",\"developer_io\":false");
#endif
  setupConfig += F("};");
  const String body =
      F("<div id='setupRoot' class='card'><p class='muted'>"
        "Einstellungen werden geladen …</p></div>");
  server.send(200, "text/html; charset=utf-8",
              page("Einstellungen", body, setupConfig,
                   String("/assets/setup.js?v=") + kFirmwareVersion));
}

void handleSetupSave() {
  if (!requireAdmin()) return;
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    if (!safeSingleLine(server.arg("ssid" + String(i)), 32) ||
        !validWifiPassword(server.arg("pass" + String(i)))) {
      server.send(400, "application/json",
                  "{\"error\":\"invalid_wifi_credentials\"}");
      return;
    }
  }
  String requestedHostname = server.arg("hostname");
  requestedHostname.trim();
  if (!validHostname(requestedHostname) ||
      !safeSingleLine(server.arg("timezone"), 80) ||
      !safeSingleLine(server.arg("mqtt_host"), 253) ||
      !safeSingleLine(server.arg("mqtt_user"), 128) ||
      !safeSingleLine(server.arg("mqtt_pass"), 256)) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_settings_text\"}");
    return;
  }
  const String requestedAdminPassword = server.arg("admin_pass");
  const String requestedAdminPasswordConfirm =
      server.arg("admin_pass_confirm");
  if (requestedAdminPassword.length() > 64) {
    server.send(400, "application/json",
                "{\"error\":\"admin_password_too_long\"}");
    return;
  }
  if (requestedAdminPassword != requestedAdminPasswordConfirm) {
    server.send(400, "application/json",
                "{\"error\":\"admin_password_confirmation_mismatch\"}");
    return;
  }
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    String newSsid = server.arg("ssid" + String(i));
    String newPassword = server.arg("pass" + String(i));
    newSsid.trim();
    if (newPassword.length() || newSsid != config.ssid[i]) config.password[i] = newPassword;
    config.ssid[i] = newSsid;
  }
  config.hostname = requestedHostname;
  String timezone = server.arg("timezone");
  timezone.trim();
  if (timezone.length() && timezone.length() <= 80)
    config.timezone = timezone;
  config.setupApMinutes =
      constrain(server.arg("ap_minutes").toInt(), 5, 60);
  const String newAdminPassword = requestedAdminPassword;
  if (newAdminPassword.length() && newAdminPassword.length() < 4) {
    server.send(400, "application/json",
                "{\"error\":\"admin_password_too_short\"}");
    return;
  }
  if (newAdminPassword.length() >= 4 && newAdminPassword.length() <= 64)
    config.adminPassword = newAdminPassword;
  config.rxPin = constrain(server.arg("rx_pin").toInt(), 0, 10);
  config.txPin = constrain(server.arg("tx_pin").toInt(), -1, 10);
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  config.snifferEnabled = server.hasArg("sniffer");
  config.bridgeEnabled = server.hasArg("bridge");
#endif
  config.apiAccess = constrain(server.arg("api_access").toInt(), 0, 2);
  config.storageCompatibilityMode = server.hasArg("storage_compat");
  config.modbusTcp = server.hasArg("modbus_tcp");
  const bool previousEventPersistence = config.persistEventLog;
  config.persistEventLog = server.hasArg("event_flash");
  config.ecoMode = server.hasArg("eco_mode");
  config.ecoLedOff = server.hasArg("eco_led_off");
  config.adaptiveWifiPower = server.hasArg("wifi_power_auto");
  config.wifiPowerSave = server.hasArg("wifi_ps");
  config.githubUpdateCheck = server.hasArg("gh_check");
  config.githubAutoInstall = server.hasArg("gh_auto");
  if (config.githubAutoInstall) config.githubUpdateCheck = true;
  config.ledPin = constrain(server.arg("led_pin").toInt(), -1, 10);
  normalizeHardwarePins();
  config.ledInverted = server.hasArg("led_inv");
  config.meterProtocol = static_cast<MeterProtocol>(
      constrain(server.arg("meter_protocol").toInt(), 0, 3));
  config.baud = server.arg("baud").toInt();
  config.mqttHost = server.arg("mqtt_host");
  config.mqttHost.trim();
  config.mqttPort = constrain(server.arg("mqtt_port").toInt(), 1, 65535);
  config.mqttUser = server.arg("mqtt_user");
  String newMqttPassword = server.arg("mqtt_pass");
  if (newMqttPassword.length() || !config.mqttHost.length()) config.mqttPassword = newMqttPassword;
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
  eventLog.add("INFO", "SETTINGS_SAVE", "Einstellungen gespeichert");
  server.send(200, "text/html; charset=utf-8",
              page("Gespeichert", "<p>Der Tracker startet jetzt neu.</p>"));
  delay(750);
  ESP.restart();
}

void handleLogout() {
  if (!requireAdmin()) return;
  server.sendHeader(
      "Set-Cookie",
      "ir_session=deleted; Max-Age=0; Path=/; HttpOnly; SameSite=Strict",
      false);
  server.sendHeader("Clear-Site-Data", "\"cookies\"");
  server.send(
      200, "text/html; charset=utf-8",
      page("Abgemeldet",
           "<div class='card'><p>Die 60-Tage-Sitzung dieses Browsers wurde "
           "gelöscht.</p><p>Falls der Browser die HTTP-Basisanmeldung selbst "
           "gespeichert hat, kann sie zusätzlich in dessen Passwortverwaltung "
           "entfernt werden.</p><a href='/'>Erneut anmelden</a></div>"));
}

void handleDiagnostics() {
  if (!requireAdmin()) return;
  String body = maintenanceTabs(true);
  body += F("<div class='card'><h2>Zählerdiagnose</h2>"
                  "<p><span id='diagnosisState' class='status-pill'>Wird geprüft …</span></p>"
                  "<p id='diagnosisSummary'>Zählerdaten werden ausgewertet.</p>"
                  "<ul id='diagnosisHints'></ul>"
                  "<div class='actions'><button id='copySupport' type='button'>Diagnose für Support kopieren</button>"
                  "<button id='copyTechnical' class='secondary' type='button'>Technische Diagnose kopieren</button></div>"
                  "<p id='copyStatus' class='muted'></p>"
                  "<div class='grid'>"
                  "<div class='stat'><strong>IR-Verbindung</strong><small id='diagnosisIr'>–</small></div>"
                  "<div class='stat'><strong>Protokoll</strong><small id='diagnosisProtocol'>–</small></div>"
                  "<div class='stat'><strong>Messwerte</strong><small id='diagnosisValues'>–</small></div>"
                  "<div class='stat'><strong>System</strong><small id='diagnosisSystem'>–</small></div>"
                  "</div></div>"
                  "<div class='grid'><div class='card'><h2>Geführter Selbsttest</h2>"
                  "<button id='runSelftest' type='button'>Selbsttest ausführen</button>"
                  "<div id='selftest' class='stats'></div></div>"
                  "<div class='card'><h2>Zählerbericht</h2>"
                  "<p><a href='/api/v1/meter-report'>Detailliert anzeigen: empfangene und fehlende OBIS-Werte</a></p>"
                  "<p class='muted'>Spannung und Strom werden nur live im RAM gehalten, niemals in der Flash-Historie.</p>"
                  "<details class='compact-details'><summary>Technische Details</summary>"
                  "<p><a href='/api/v1/memory-info'>Speicherinformationen</a></p>"
                  "<p><a href='/api/v1/obis'>Alle erkannten OBIS-Werte</a></p>"
                  "<p><a href='/api/v1/raw'>Letztes Zählertelegramm (Hex)</a></p>"
                  "<p>IR-Sniffer WebSocket: <code>ws://GERAET:81/</code></p>"
                  "<p>IR-Bridge WebSocket: <code>ws://GERAET:82/</code></p>"
                  "<p class='muted'>Die Bridge ist nur aktiv, wenn ein TX-GPIO eingestellt wurde.</p>"
                  "</details></div></div>"
                  "<details class='card compact-details'><summary>Optionale IR-Freischaltung (experimentell)</summary>"
                  "<p>Nur für Zähler, die diese optische Impulsfolge unterstützen. Sendet automatisch Initialisierung, "
                  "gespeicherte PIN, Navigation zu Inf und den langen Impuls für Inf ON. "
                  "Danach prüft der Tracker bis zu 90 Sekunden auf Momentanleistung.</p>"
                  "<form method='post' action='/ir/meter-unlock' "
                  "onsubmit=\"return confirm('Optionale IR-Freischaltung jetzt starten? Der Tracker darf dabei nicht bewegt werden.')\">"
                  "<button type='submit'>Zähler mit gespeicherter PIN freischalten</button></form>"
                  "<p class='muted'>Vorher unten die PIN einmal lokal speichern. Dauer ungefähr eine Minute.</p></details>"
                  "<details class='card compact-details'><summary>Stromzähler-PIN über IR</summary>"
                  "<p>Die vierstellige PIN wird nur im Arbeitsspeicher verarbeitet und nicht gespeichert. "
                  "Am Zähler zuerst die PIN-Anzeige aktivieren. Eine Ziffer 0 wird als zehn Lichtimpulse gesendet.</p>"
                  "<form method='post' action='/ir/pin'>"
                  "<label>Vierstellige PIN</label><input name='pin' type='password' inputmode='numeric' "
                  "pattern='[0-9]{4}' minlength='4' maxlength='4' autocomplete='off' placeholder='");
  body += config.meterPin.length() ? "gespeichert - leer lassen zum Verwenden" : "vier Ziffern";
  body += F("'>"
                  "<div class='inline'><div><label>Impulsdauer (ms)</label>"
                  "<input name='pulse_ms' type='number' min='50' max='1000' value='");
  body += String(config.pinPulseMs);
  body += F("'></div>"
                  "<div><label>Ziffernpause (ms)</label>"
                  "<input name='digit_gap_ms' type='number' min='1000' max='10000' value='");
  body += String(config.pinDigitGapMs);
  body += F("'></div></div><label><input style='width:auto' type='checkbox' name='invert' value='1'");
  if (config.pinInverted) body += " checked";
  body += F("> IR-Ausgang invertieren</label>"
                  "<label><input style='width:auto' type='checkbox' name='save_pin' value='1'> PIN lokal speichern</label>"
                  "<p class='muted'>Unterstützt der Zähler keine optische PIN-Steuerung, erfolgt die Freischaltung mit Zählertaste oder Taschenlampe.</p>"
                  "<p class='muted'>Die automatische PIN-Eingabe wird nicht von jedem Zähler oder optischen Lesekopf unterstützt. "
                  "Bei fehlender Displayreaktion PIN und Inf-Freigabe mit Zählertaste oder Taschenlampe durchführen.<br><br>"
                  "Das Speichern ist optional. Die PIN liegt lokal im Gerätespeicher; "
                  "ohne aktivierte ESP32-Flash-Verschlüsselung ist sie nicht kryptografisch gegen "
                  "einen direkten Hardwarezugriff geschützt. Die Automatik sendet höchstens einmal "
                  "pro Neustart und bleibt aus, sobald der Leistungswert vorhanden ist.</p>"
                  "<button type='submit'>PIN-Impulsfolge starten</button></form>"
                  "<form method='post' action='/ir/pin/forget'><button class='danger' type='submit'>"
                  "Gespeicherte PIN und Automatik löschen</button></form>"
                  "<form method='post' action='/ir/pulse'><input type='hidden' name='count' value='1'>"
                  "<button type='submit'>Einzelnen Testimpuls senden</button></form>"
                  "<form method='post' action='/ir/stop'><button class='danger' type='submit'>IR-Sendung stoppen</button></form>"
                  "<p class='muted'>Die genaue Bedienfolge ist vom Zählermodell abhängig. Während der Impulsfolge "
                  "pausiert der SML-Empfang kurzzeitig.</p></details>");
  server.send(200, "text/html; charset=utf-8",
              page("Wartung – Diagnose", body, "",
                   String("/assets/diagnostics.js?v=") + kFirmwareVersion));
}

#if IR_TRACKER_ENABLE_FACTORY_TEST
void handleFactoryTestPage() {
  if (!requireAdmin()) return;
  String body = maintenanceTabs(true, true);
  body += F(
      "<div class='card'><span class='status-pill'>Nur Werksprüfungs-Build</span>"
      "<h2>Werksprüfung</h2>"
      "<p>Der Test prüft ESP32-C3, 4-MiB-Flash, RAM, Historienpartition, WLAN, "
      "W5500/LAN sowie IR-Sender und IR-Empfänger. Für den IR-Test muss ein "
      "optischer Prüfreflektor beide Bauteile koppeln.</p>"
      "<div class='actions'><button id='fctStart'>Prüfung starten</button>"
      "<button class='secondary' id='fctLed'>Leuchtende Status-LED bestätigen</button>"
      "<button class='secondary' id='fctPoe'>Betrieb nur über PoE bestätigen</button></div>"
      "<p><strong id='fctOverall'>Bereit</strong> <span id='fctProgress' class='muted'></span></p>"
      "<div id='fctTests' class='stats'></div></div>");
  const String script = F(
      "const o=document.getElementById('fctOverall'),p=document.getElementById('fctProgress'),x=document.getElementById('fctTests');"
      "const labels={chip:'ESP32-C3',flash:'Flash',ram:'RAM',storage:'Historie',wifi:'WLAN',w5500:'W5500',ethernet:'LAN',ir_loopback:'IR TX/RX',led:'Status-LED',poe:'PoE',ble:'Bluetooth'};"
      "async function load(){const r=await fetch('/api/v1/factory-test',{cache:'no-store'}),j=await r.json();"
      "o.textContent=j.state==='pass'?'PASS':j.state==='running'?'PRÜFUNG LÄUFT':j.state==='waiting'?'BESTÄTIGUNG AUSSTEHEND':j.state==='fail'?'FAIL':'BEREIT';o.className=j.state==='pass'?'ok':j.state==='fail'?'error':'status-pill';p.textContent=j.state==='running'?j.progress+' %':'';"
      "x.innerHTML=j.tests.map(t=>`<div class='stat'><strong>${labels[t.id]||t.id}: ${t.state.toUpperCase()}</strong><small>${t.detail}</small></div>`).join('');if(j.state==='running')setTimeout(load,250)}"
      "document.getElementById('fctStart').onclick=async()=>{await fetch('/api/v1/factory-test/start',{method:'POST'});load()};"
      "document.getElementById('fctLed').onclick=async()=>{await fetch('/api/v1/factory-test/led-confirm',{method:'POST'});load()};"
      "document.getElementById('fctPoe').onclick=async()=>{await fetch('/api/v1/factory-test/poe-confirm',{method:'POST'});load()};load();");
  server.send(200, "text/html; charset=utf-8",
              page("Werksprüfung", body, script));
}
#endif
