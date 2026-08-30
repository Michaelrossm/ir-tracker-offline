// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

String selfTestJson() {
  const bool meterFresh =
      meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
  String json = "{\"tests\":[";
  auto add = [&](const char *id, const char *label, const char *state,
                 const String &detail, bool &first) {
    if (!first) json += ",";
    first = false;
    json += "{\"id\":\"" + String(id) + "\",\"label\":\"" +
            jsonEscape(label) + "\",\"state\":\"" + state +
            "\",\"detail\":\"" + jsonEscape(detail) + "\"}";
  };
  bool first = true;
  add("ethernet", "LAN",
      ethernet.connected() ? "ok"
                           : (ethernet.hardwareDetected() ? "warn" : "off"),
      ethernet.connected()
          ? String("W5500 verbunden: ") + ethernet.localIP().toString()
          : (ethernet.hardwareDetected()
                 ? "W5500 erkannt, aber kein DHCP-Netzwerk verfügbar"
                 : "Kein W5500 erkannt; WLAN-Betrieb bleibt aktiv"),
      first);
  add("wifi", "WLAN", WiFi.status() == WL_CONNECTED ? "ok" : "warn",
      WiFi.status() == WL_CONNECTED
          ? WiFi.SSID() + " (" + String(WiFi.RSSI()) +
                " dBm), TX " + String(wifiTxPowerDbm(), 1) + " dBm"
          : "Nicht mit dem Heim-WLAN verbunden",
      first);
  add("wifi_eco", "WLAN-Energiesparen",
      wifiTxPowerRuntimeFault || wifiModeErrors ? "warn" : "ok",
      accessPointMode
          ? "Setup-Hotspot aktiv; volle Sendeleistung"
          : (String("STA, ") +
             (wifiMinModemSleepActive ? "MIN_MODEM" : "Modem-Sleep Fehler") +
             ", Profil " + wifiTxProfileName()),
      first);
  add("time", "Uhrzeit", time(nullptr) >= 1700000000 ? "ok" : "warn",
      time(nullptr) >= 1700000000
          ? String("Synchronisiert, TZ: ") + config.timezone
          : "Noch keine gültige Uhrzeit; Browser oder NTP erforderlich",
      first);
  add("meter", "Zählerempfang", meterFresh ? "ok" : "error",
      meterFresh ? String(meter.telegrams) + " Telegramme empfangen"
                 : "Kein aktuelles Zählertelegramm",
      first);
  add("values", "Messwerte",
      std::isfinite(meter.powerW) ? "ok" : "error",
      std::isfinite(meter.powerW)
          ? String(meter.powerW, 1) + " W"
          : "Leistungs-OBIS fehlt; PIN und Zählerfreigabe prüfen",
      first);
  uint8_t phaseCount = 0;
  for (uint8_t phase = 0; phase < 3; ++phase)
    if (std::isfinite(meter.phasePowerW[phase]) ||
        std::isfinite(meter.phaseVoltageV[phase]) ||
        std::isfinite(meter.phaseCurrentA[phase]))
      ++phaseCount;
  add("phases", "Phasenwerte", phaseCount ? "ok" : "off",
      phaseCount ? String(phaseCount) + " Phasen vom Zähler geliefert"
                 : "Zähler liefert keine einzelnen Phasenwerte",
      first);
  add("history", "Lokale Historie", history.ready() ? "ok" : "error",
      history.ready()
          ? String(history.usedBytes()) + " von " +
                String(history.totalBytes()) + " Bytes verwendet"
          : "Historien-Dateisystem nicht verfügbar",
      first);
  add("memory", "Arbeitsspeicher",
      ESP.getMinFreeHeap() >= 30000 ? "ok" : "warn",
      String(ESP.getFreeHeap()) + " Bytes frei, Minimum " +
          String(ESP.getMinFreeHeap()),
      first);
  add("cpu", "CPU-Energiesparmodus",
      cpuEcoRuntimeFault ? "warn" : (config.ecoMode ? "ok" : "off"),
      cpuEcoRuntimeFault
          ? "Eco-Laufzeitsicherung aktiv; Betrieb mit " +
                String(getCpuFrequencyMhz()) + " MHz"
          : (config.ecoMode
                 ? String(getCpuFrequencyMhz()) + " MHz, " +
                       (cpuBoostActive()
                            ? "Leistungsmodus noch " +
                                  String(cpuBoostRemainingSeconds()) + " s"
                            : "Eco-Betrieb")
                 : "Deaktiviert; dauerhaft 160 MHz"),
      first);
  add("eco_led", "Eco-Status-LED",
      config.ledPin < 0
          ? "off"
          : (trackerFaultActive() ? "warn" : "ok"),
      config.ledPin < 0
          ? "Kein LED-GPIO konfiguriert"
          : (trackerFaultActive()
                 ? "Fehleranzeige aktiv; Eco-Abschaltung wird ueberbrueckt"
                 : (ecoLedSuppressed()
                        ? "Im fehlerfreien Eco-Betrieb ausgeschaltet"
                        : "Normale Statusanzeige aktiv")),
      first);
  add("mqtt", "MQTT",
      !config.mqttHost.length() ? "off" : (mqtt.connected() ? "ok" : "warn"),
      !config.mqttHost.length()
          ? "Nicht konfiguriert"
          : (mqtt.connected() ? "Verbunden" : "Broker nicht erreichbar"),
      first);
  json += "]}";
  return json;
}

String meterReportJson() {
  const bool fresh =
      meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
  String json = "{\"manufacturer\":\"unknown\",\"model\":\"electricity meter\",";
  json += "\"protocol\":\"" +
          String(meterProtocolName(meter.detectedProtocol)) + "\",";
  json += "\"configured_protocol\":\"" +
          String(meterProtocolName(config.meterProtocol)) + "\",";
  json += "\"telegram_age_s\":" + ageOrNull(meter.lastTelegramMs) + ",";
  json += "\"telegram_fresh\":" + String(fresh ? "true" : "false") + ",";
  json += "\"telegram_count\":" + String(meter.telegrams) + ",";
  json += "\"last_crc_valid\":" +
          String(meter.lastCrcValid ? "true" : "false") + ",";
  json += "\"received\":{\"1.8.0\":" +
          String(std::isfinite(meter.importKwh) ? "true" : "false") +
          ",\"2.8.0\":" +
          String(std::isfinite(meter.exportKwh) ? "true" : "false") +
          ",\"16.7.0\":" +
          String(std::isfinite(meter.powerW) ? "true" : "false") + "},";
  json += "\"phases\":[";
  const uint8_t powerCodes[3] = {36, 56, 76};
  const uint8_t voltageCodes[3] = {32, 52, 72};
  const uint8_t currentCodes[3] = {31, 51, 71};
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (phase) json += ",";
    json += "{\"phase\":\"L" + String(phase + 1) + "\",";
    json += "\"power_obis\":\"" + String(powerCodes[phase]) +
            ".7.0\",\"power_received\":" +
            String(std::isfinite(meter.phasePowerW[phase]) ? "true" : "false") +
            ",\"voltage_obis\":\"" + String(voltageCodes[phase]) +
            ".7.0\",\"voltage_received\":" +
            String(std::isfinite(meter.phaseVoltageV[phase]) ? "true"
                                                             : "false") +
            ",\"current_obis\":\"" + String(currentCodes[phase]) +
            ".7.0\",\"current_received\":" +
            String(std::isfinite(meter.phaseCurrentA[phase]) ? "true"
                                                             : "false") +
            "}";
  }
  json += "],\"ir_tx\":{\"gpio\":" + String(config.txPin) +
          ",\"inverted\":" + String(config.pinInverted ? "true" : "false") +
          ",\"last_sequence_result\":\"";
  json += std::isfinite(meter.powerW)
              ? "extended_dataset_received"
              : "no_extended_dataset_no_meter_ack_available";
  json += "\"},\"note\":\"Spannung und Strom werden nur live im RAM gehalten und nicht historisch gespeichert.\"}";
  return json;
}
