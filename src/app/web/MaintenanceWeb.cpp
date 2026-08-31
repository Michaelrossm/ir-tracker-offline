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
  String script = F(
      "const el=id=>document.getElementById(id),status=t=>el('maintenanceStatus').textContent=t;"
      "const updateText=e=>{if(e==='network_not_connected')return'Keine LAN- oder WLAN-Verbindung. Netzwerkverbindung prüfen.';if(e==='system_time_not_synchronized')return'Die Gerätezeit ist noch nicht synchronisiert.';if(e==='github_json_invalid')return'Die Antwort der Updatequelle konnte nicht verarbeitet werden.';if(e.startsWith('github_http_'))return'Die Updatequelle ist momentan nicht erreichbar.';return'Die Updateprüfung konnte nicht abgeschlossen werden.'};"
      "async function loadUpdate(){try{const r=await fetch('/api/v1/update/status'),u=await r.json();el('updateCurrent').textContent=u.current_version;el('updateAvailable').textContent=u.available?u.latest_version:'–';el('updateLast').textContent=u.last_success?new Date(u.last_success*1000).toLocaleString():'Noch nicht geprüft';el('updateInstall').hidden=!u.available;const s=el('updateState'),d=el('updateError');d.hidden=!u.error;el('updateErrorCode').textContent=u.error||'';s.className=u.error?'error':u.checked?'status-pill':'muted';s.textContent=u.error?'Updateprüfung fehlgeschlagen: '+updateText(u.error):u.available?'Eine neuere signierte Firmware ist verfügbar.':u.checked?'Die installierte Firmware ist aktuell.':'Es wurde in dieser Laufzeit noch keine manuelle Prüfung durchgeführt.'}catch(e){el('updateState').className='error';el('updateState').textContent='Update-Status konnte nicht geladen werden.'}}loadUpdate();"
      "const download=(name,data)=>{const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([data],{type:'application/json'}));a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000)};"
      "el('fullBackup').onclick=()=>{status('Vollständiges Backup wird erstellt …');el('settingsExport').click();setTimeout(()=>el('historyExport').click(),800)};"
      "el('settingsExport').onclick=async()=>{status('Einstellungen werden geladen ...');const r=await fetch('/api/v1/backup/settings');download('irtracker-settings.json',await r.text());status('Einstellungsbackup erstellt')};"
      "el('settingsImport').onclick=async()=>{const f=el('settingsFile').files[0];if(!f)return status('Bitte Einstellungsbackup auswählen');"
      "const text=await f.text();let j;try{j=JSON.parse(text)}catch(e){return status('Ungültige JSON-Datei')}if(j.format!=='irtracker-settings'||j.version!==1)return status('Falsches Backupformat');"
      "if(!confirm('Einstellungen ersetzen und Tracker neu starten?'))return;const r=await fetch('/api/v1/backup/settings/restore',{method:'POST',headers:{'Content-Type':'application/json'},body:text});status(r.ok?'Wiederhergestellt, Tracker startet neu':'Wiederherstellung fehlgeschlagen')};"
      "el('historyExport').onclick=async()=>{status('Historie wird gestreamt ...');const out={format:'irtracker-history',version:1,created:new Date().toISOString(),tiers:{}};"
      "for(const [name,range] of Object.entries({minute:'minute_all',quarter:'quarter_all',hour:'hour_all',day:'day_all'})){const r=await fetch('/api/v1/history?range='+range);if(!r.ok)return status('Fehler bei '+name);out.tiers[name]=(await r.json()).values}"
      "download('irtracker-history.json',JSON.stringify(out));status('Historienbackup erstellt')};"
      "el('historyImport').onclick=async()=>{const f=el('historyFile').files[0];if(!f)return status('Bitte Historienbackup auswählen');let b;try{b=JSON.parse(await f.text())}catch(e){return status('Ungültige JSON-Datei')}"
      "if(b.format!=='irtracker-history'||b.version!==1||!b.tiers)return status('Falsches Backupformat');for(const n of ['minute','quarter','hour','day']){if(!Array.isArray(b.tiers[n]))return status('Historienstufe fehlt: '+n);"
      "if(!b.tiers[n].every(v=>Number.isInteger(v.ts)&&v.ts>=1700000000&&Number.isFinite(v.avg)&&Number.isFinite(v.min)&&Number.isFinite(v.max)&&(v.import==null||Number.isFinite(v.import))&&(v.export==null||Number.isFinite(v.export))))return status('Ungültiger Datensatz in '+n)}"
      "if(!confirm('Vorhandene Historie durch dieses Backup ersetzen?'))return;for(const n of ['minute','quarter','hour','day']){status('Importiere '+n+' ...');let r=await fetch('/api/v1/history/import/start',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tier:n})});if(!r.ok)return status('Start fehlgeschlagen: '+n);"
      "for(let i=0;i<b.tiers[n].length;i+=50){r=await fetch('/api/v1/history/import/batch',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tier:n,values:b.tiers[n].slice(i,i+50)})});if(!r.ok)return status('Import fehlgeschlagen: '+n+' '+i);status('Importiere '+n+': '+Math.min(i+50,b.tiers[n].length)+' / '+b.tiers[n].length)}}status('Historie vollständig wiederhergestellt')};"
      "el('historyClear').onclick=async()=>{if(!confirm('Wirklich ALLE lokalen Messwerte dauerhaft löschen? Vorher Backup erstellen!'))return;"
      "const r=await fetch('/api/v1/history/clear',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'confirm=DELETE'});status(r.ok?'Historie vollständig gelöscht':'Historie konnte nicht gelöscht werden')};"
      "async function loadEvents(){const r=await fetch('/api/v1/events');const j=await r.json();el('events').textContent=j.events.map(x=>`${x.ts>1700000000?new Date(x.ts*1000).toLocaleString('de-DE'):'Uptime '+x.uptime_s+'s'} [${x.level}] ${x.code}: ${x.message}`).join('\\n')||'Keine Ereignisse'}"
      "el('eventsReload').onclick=loadEvents;el('eventsClear').onclick=async()=>{if(confirm('Protokoll wirklich löschen?')){await fetch('/api/v1/events/clear',{method:'POST'});loadEvents()}};loadEvents();");
  script += F(
      "el('safeShutdown').onclick=async()=>{if(!confirm('Tracker wirklich sicher herunterfahren? Zum Starten ist danach Strom Aus/Ein oder Reset nötig.'))return;"
      "status('Minutenpuffer wird gespeichert ...');const r=await fetch('/system/shutdown',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'confirm=SHUTDOWN'});"
      "status(r.ok?'Tracker wurde sicher heruntergefahren. Zum Starten Strom Aus/Ein oder Reset.':'Herunterfahren fehlgeschlagen; Tracker bleibt aktiv.')};");
  server.send(200, "text/html; charset=utf-8",
              page("Backup und Wartung", body, script));
}

void handleSetup() {
  if (!requireAdmin()) return;
  String body = F("<form method='post' action='/setup/save'><fieldset><legend>WLAN-Verbindungen</legend>"
                  "<p class='muted'>Bis zu drei Netze. Der Tracker probiert sie der Reihe nach. Ist keines erreichbar, startet automatisch der Setup-Hotspot.</p>"
                  "<div class='error'>HTTP ist nicht transportverschlüsselt. Nur in einem vertrauenswürdigen Heim- oder getrennten IoT-Netz betreiben, keine Router-Portfreigabe einrichten und Fernzugriff ausschließlich per VPN verwenden.</div>");
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    body += "<div class='inline'><div><label>WLAN " + String(i + 1) + "</label><input name='ssid" +
            String(i) + "' value=\"" + htmlEscape(config.ssid[i]) +
            "\" maxlength='32' placeholder='Netzwerkname'></div><div><label>Passwort</label><input type='password' name='pass" +
            String(i) + "' placeholder='" + String(config.password[i].length() ? "gespeichert" : "offenes WLAN") +
            "' maxlength='64' autocomplete='off' data-lpignore='true'></div></div>";
  }
  body += F("<label>Hostname</label><input name='hostname' value='");
  body += htmlEscape(config.hostname);
  body += F("'><label>Zeitzone (POSIX-TZ)</label><input name='timezone' value='");
  body += htmlEscape(config.timezone);
  body += F("'><p class='muted'>Deutschland: CET-1CEST,M3.5.0,M10.5.0/3</p>"
            "<label>Setup-Hotspot Laufzeit (Minuten)</label><input type='number' name='ap_minutes' min='5' max='60' value='");
  body += String(config.setupApMinutes);
  body += F("'><p class='muted'>Nach Ablauf wird der Hotspot abgeschaltet. Ein Neustart öffnet ihn erneut.</p>"
            "<label>Neues Admin-Passwort</label><input type='password' name='admin_pass' "
            "minlength='4' maxlength='64' autocomplete='new-password' placeholder='unverändert lassen'>"
            "<label>Neues Admin-Passwort wiederholen</label><input type='password' name='admin_pass_confirm' "
            "minlength='4' maxlength='64' autocomplete='new-password' placeholder='unverändert lassen'>"
            "<p class='muted'>Erlaubt sind 4 bis 64 Zeichen; für gute Sicherheit werden mindestens 12 Zeichen empfohlen. "
            "Angemeldete Browser werden 60 Tage über ein signiertes "
            "HttpOnly-Cookie wiedererkannt. Eine Passwortänderung macht alte Sitzungen ungültig.</p>"
            "</fieldset><fieldset><legend>Stromzähler</legend>"
            "<label>IR-Eingang (GPIO)</label><select name='rx_pin'>");
  for (int pin = 0; pin <= 10; ++pin) {
    if (!trackerGpioAvailable(pin)) continue;
    body += "<option value='" + String(pin) + "'" + (pin == config.rxPin ? " selected" : "") + ">" + String(pin) + "</option>";
  }
  body += F("</select><label>IR-Sendeausgang (GPIO, -1 = aus)</label><select name='tx_pin'>"
            "<option value='-1'>Aus</option>");
  for (int pin = 0; pin <= 10; ++pin) {
    if (!trackerGpioAvailable(pin)) continue;
    body += "<option value='" + String(pin) + "'" + (pin == config.txPin ? " selected" : "") + ">" + String(pin) + "</option>";
  }
  body += "</select>";
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  body += "<label><input style='width:auto' type='checkbox' name='sniffer' value='1'" +
          String(config.snifferEnabled ? " checked" : "") +
          "> IR-Sniffer auf Port 81 aktivieren</label>"
          "<label><input style='width:auto' type='checkbox' name='bridge' value='1'" +
          String(config.bridgeEnabled ? " checked" : "") +
          "> Schreibende IR-Bridge aktivieren</label>"
          "<p class='muted'>Nur im Entwickler-Build verfuegbar.</p>";
#endif
  body += F("<label>Status-LED (GPIO, -1 = aus)</label><select name='led_pin'>"
            "<option value='-1'>Aus</option>");
  for (int pin = 0; pin <= 10; ++pin) {
    if (!trackerGpioAvailable(pin)) continue;
    body += "<option value='" + String(pin) + "'" + (pin == config.ledPin ? " selected" : "") + ">" + String(pin) + "</option>";
  }
  body += "</select><label><input style='width:auto' type='checkbox' name='led_inv' value='1'" +
          String(config.ledInverted ? " checked" : "") + "> LED invertieren</label>";
  body += F("<label>Zählerprotokoll</label><select name='meter_protocol'>"
            "<option value='0'");
  if (config.meterProtocol == MeterProtocol::Auto) body += " selected";
  body += F(">Automatisch (SML und älteres IEC 62056-21)</option>"
            "<option value='1'");
  if (config.meterProtocol == MeterProtocol::Sml) body += " selected";
  body += F(">SML</option><option value='2'");
  if (config.meterProtocol == MeterProtocol::Iec62056) body += " selected";
  body += F(">IEC 62056-21 / D0 passiv (ASCII)</option><option value='3'");
  if (config.meterProtocol == MeterProtocol::Iec62056Active) body += " selected";
  body += F(">IEC 62056-21 / D0 aktiv (300 Baud)</option></select>"
            "<p class='muted'>Automatisch liest SML und passive ASCII-Telegramme. "
            "Bleiben gültige Daten aus, versucht der Tracker zusätzlich eine aktive "
            "IEC-Abfrage. Der aktive Modus sendet /?! und ACK 000 an ältere Zähler.</p>"
            "<label>Baudrate</label><select name='baud'>");
  const uint32_t rates[] = {300, 600, 1200, 2400, 4800,
                            9600, 19200, 38400, 115200};
  for (uint32_t rate : rates) {
    body += "<option value='" + String(rate) + "'" + (rate == config.baud ? " selected" : "") + ">" + String(rate) + "</option>";
  }
  body += F("</select></fieldset><fieldset><legend>Lokale API und Kompatibilität</legend>"
            "<label>Zugriffsmodus</label><select name='api_access'><option value='0'");
  if (config.apiAccess == 0) body += " selected";
  body += F(">Lokal offen (Integrationen ohne Anmeldung)</option><option value='1'");
  if (config.apiAccess == 1) body += " selected";
  body += F(">Admin-Anmeldung erforderlich</option><option value='2'");
  if (config.apiAccess == 2) body += " selected";
  body += F(">API und Shelly-Kompatibilität deaktiviert</option></select>"
            "<p class='muted'>Betrifft Messwert-API, Prometheus, Influx, CSV und Shelly-Endpunkte. Einstellungen und Wartung bleiben immer geschützt.</p>"
            "<details class='compact-details'><summary>JSON-API für Experten</summary>"
            "<p class='muted'>Für Home Assistant, ioBroker, Node-RED, openHAB und eigene lokale Auswertungen. "
            "Die API ist keine eigene Bedienseite und bleibt deshalb aus der Hauptnavigation ausgeblendet.</p>"
            "<code>/api/v1/status</code><br><code>/api/v1/obis</code><br>"
            "<code>/api/v1/history</code><br><code>/api/v1/values.csv</code>"
            "</details>"
            "<label><input style='width:auto' type='checkbox' name='event_flash' value='1'");
  if (config.persistEventLog) body += " checked";
  body += F("> Ereignis- und Fehlerprotokoll dauerhaft im Flash speichern</label>"
            "<p class='muted'>Standardmäßig aus: Bis zu 256 Einträge bleiben nur im RAM. "
            "Die ältesten werden automatisch überschrieben; ein Neustart leert das Protokoll. "
            "Aktivieren schreibt neue Einträge zusätzlich dauerhaft in den Flash.</p>"
            "</fieldset><fieldset><legend>Home Assistant / MQTT</legend>"
            "<p class='muted'>Optional. Mit MQTT Discovery erscheinen die Sensoren automatisch in Home Assistant.</p>"
            "<div class='inline'><div><label>MQTT-Server</label><input name='mqtt_host' value='");
  body += htmlEscape(config.mqttHost);
  body += F("' placeholder='192.168.178.10'></div><div><label>Port</label><input type='number' name='mqtt_port' value='");
  body += String(config.mqttPort);
  body += F("'></div></div><div class='inline'><div><label>Benutzer</label><input name='mqtt_user' value='");
  body += htmlEscape(config.mqttUser);
  body += F("'></div><div><label>Passwort</label><input type='password' name='mqtt_pass' placeholder='");
  body += config.mqttPassword.length() ? "gespeichert" : "optional";
  body += F("'></div></div><label><input style='width:auto' type='checkbox' name='ha_disc' value='1'");
  if (config.homeAssistantDiscovery) body += " checked";
  body += F("> Home-Assistant-Discovery aktivieren</label></fieldset>"
            "<fieldset><legend>Energiesparen</legend>"
            "<label><input style='width:auto' type='checkbox' name='eco_mode' value='1'");
  if (config.ecoMode) body += " checked";
  body += F("> Eco-Modus aktivieren</label>"
            "<p class='muted'>Standardmäßig aktiv: 80 MHz im Messbetrieb. "
            "Vollständiger Export, Import und Firmwareupdate schalten automatisch "
            "auf 160 MHz. Zwei Minuten nach der letzten rechenintensiven Aufgabe wird "
            "wieder auf 80 MHz reduziert.</p>"
            "<label><input style='width:auto' type='checkbox' name='eco_led_off' value='1'");
  if (config.ecoLedOff) body += " checked";
  body += F("> Status-LED bei fehlerfreiem Eco-Betrieb ausschalten</label>"
            "<p class='muted'>Wirkt nur bei aktivem Eco-Modus. Bei fehlendem oder "
            "veraltertem Zählerwert, WLAN-Ausfall, Speicherwarnung oder internem "
            "Eco-Fehler bleibt die LED-Warnanzeige automatisch aktiv.</p>"
            "<label><input style='width:auto' type='checkbox' name='wifi_power_auto' value='1'");
  if (config.adaptiveWifiPower) body += " checked";
  body += F("> WLAN-Sendeleistung im Eco-Modus automatisch anpassen</label>"
            "<p class='muted'>Start, Setup-Hotspot und Wiederverbindung verwenden immer "
            "19,5 dBm. Nach drei stabilen Minuten wird bei gutem Signal vorsichtig auf "
            "15 oder 11 dBm reduziert. Bei schwachem Signal oder Abbruch wird automatisch "
            "volle Leistung verwendet.</p></fieldset>"
            "<fieldset><legend>Firmwareupdates</legend>"
            "<label><input style='width:auto' type='checkbox' name='gh_check' value='1'");
  if (config.githubUpdateCheck) body += " checked";
  body += F("> Täglich auf signierte GitHub-Firmware prüfen</label>"
            "<label><input style='width:auto' type='checkbox' name='gh_auto' value='1'");
  if (config.githubAutoInstall) body += " checked";
  body += F("> Neue signierte Firmware automatisch installieren</label>"
            "<p class='muted'>Die automatische Installation ist standardmäßig aus. "
            "Akzeptiert werden nur neuere, von Michael Roßmann kryptografisch signierte "
            "IRFW-Pakete aus dem offiziellen GitHub-Release.</p></fieldset>"
            "<button type='submit'>Alle Einstellungen speichern</button></form>"
            "<form method='post' action='/auth/logout'><button class='secondary' type='submit'>"
            "Diesen Browser abmelden</button></form>");
  server.send(200, "text/html; charset=utf-8", page("Einstellungen", body));
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
  const bool previousEventPersistence = config.persistEventLog;
  config.persistEventLog = server.hasArg("event_flash");
  config.ecoMode = server.hasArg("eco_mode");
  config.ecoLedOff = server.hasArg("eco_led_off");
  config.adaptiveWifiPower = server.hasArg("wifi_power_auto");
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
  const String script = F(
      "const el=id=>document.getElementById(id),out=el('selftest');"
      "const shown=(v,d=3,u='')=>v==null?'nicht verfügbar':Number(v).toLocaleString('de-DE',{maximumFractionDigits:d})+(u?' '+u:'');"
      "async function loadDiagnosis(){try{const r=await fetch('/api/v1/meter-report',{cache:'no-store'});if(!r.ok)throw Error(r.status);const m=await r.json(),d=m.diagnosis,v=m.values,s=m.system;"
      "const labels={ok:'OK',warn:'HINWEIS',error:'FEHLER'},colors={ok:'#63e68b',warn:'#ffb454',error:'#ff8d69'},badge=el('diagnosisState');badge.textContent=labels[d.state]||'HINWEIS';badge.style.borderColor=colors[d.state];badge.style.color=colors[d.state];"
      "el('diagnosisSummary').textContent=d.summary;el('diagnosisHints').replaceChildren(...d.hints.map(h=>{const li=document.createElement('li');li.textContent=h;return li}));"
      "const integrity=m.integrity_present?(m.last_crc_valid?'gültig':'fehlerhaft'):'nicht geliefert';"
      "el('diagnosisIr').textContent=`RX-Bytes: ${m.rx_bytes} · Telegramme: ${m.telegram_count} · Letztes: ${m.telegram_age_s==null?'nicht verfügbar':m.telegram_age_s+' s'} · CRC/Integrität: ${integrity} · Parsefehler: ${m.parse_errors} · CRC-Fehler: ${m.crc_errors}`;"
      "el('diagnosisProtocol').textContent=`Konfiguriert: ${m.configured_protocol} · Erkannt: ${m.protocol}`;"
      "const phases=m.phases.map(p=>`${p.phase}: ${shown(p.power_w,1,'W')} / ${shown(p.voltage_v,1,'V')} / ${shown(p.current_a,3,'A')}`).join(' · ');"
      "el('diagnosisValues').textContent=`Leistung: ${shown(v.power_w,1,'W')} · Bezug: ${shown(v.import_kwh,6,'kWh')} · Einspeisung: ${shown(v.export_kwh,6,'kWh')} · ${phases}`;"
      "const mqtt={connected:'verbunden',disconnected:'nicht verbunden',not_configured:'nicht konfiguriert'}[s.mqtt_state]||s.mqtt_state;"
      "el('diagnosisSystem').textContent=`Verbindung: ${s.transport} · WLAN-Signal: ${s.wifi_rssi==null?'nicht aktiv':s.wifi_rssi+' dBm'} · Heap: ${s.free_heap} Byte · Minimum: ${s.minimum_free_heap} Byte · Historie: ${s.history_ready?'bereit':'Fehler'} · MQTT: ${mqtt}`;}"
      "catch(e){el('diagnosisState').textContent='FEHLER';el('diagnosisSummary').textContent='Diagnosedaten konnten nicht geladen werden. Verbindung zum Tracker prüfen.'}}"
      "async function copyReport(technical){const status=el('copyStatus');try{const r=await fetch('/api/v1/support-report'+(technical?'?technical=1':''),{cache:'no-store'});if(!r.ok)throw Error(r.status);const text=await r.text();"
      "if(navigator.clipboard&&window.isSecureContext)await navigator.clipboard.writeText(text);else{const area=document.createElement('textarea');area.value=text;area.style.position='fixed';area.style.opacity='0';document.body.appendChild(area);area.select();if(!document.execCommand('copy'))throw Error('copy');area.remove()}status.textContent=technical?'Technische Diagnose kopiert.':'Diagnose für Support kopiert.'}"
      "catch(e){status.textContent='Kopieren nicht möglich. Bitte Browserberechtigung prüfen.'}}"
      "el('copySupport').onclick=()=>copyReport(false);el('copyTechnical').onclick=()=>copyReport(true);"
      "async function test(){out.innerHTML='<div class=\"loading\"><span class=\"spinner\"></span>Prüfung läuft …</div>';"
      "try{const r=await fetch('/api/v1/selftest',{cache:'no-store'});if(!r.ok)throw Error(r.status);"
      "const j=await r.json(),c={ok:'#63e68b',warn:'#ffb454',error:'#ff8d69',off:'#9bb3a4'};"
      "out.innerHTML=j.tests.map(t=>`<div class='stat' style='border-color:${c[t.state]}'><span style='color:${c[t.state]};font-weight:700'>${t.state==='ok'?'OK':t.state==='warn'?'HINWEIS':t.state==='error'?'FEHLER':'OPTIONAL'}</span><strong>${t.label}</strong><small>${t.detail}</small></div>`).join('')}"
      "catch(e){out.innerHTML='<div class=\"error\">Selbsttest konnte nicht geladen werden. Verbindung zum Tracker prüfen.</div>'}}"
      "el('runSelftest').onclick=test;loadDiagnosis();test();");
  server.send(200, "text/html; charset=utf-8",
              page("Wartung – Diagnose", body, script));
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
