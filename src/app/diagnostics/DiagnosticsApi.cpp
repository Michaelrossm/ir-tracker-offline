// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

enum class MeterDiagnosisCode : uint8_t {
  NoSignal,
  NoTelegram,
  Stale,
  ParseUnstable,
  IntegrityUnstable,
  PartialValues,
  MissingEnergy,
  Complete
};

struct MeterDiagnosis {
  MeterDiagnosisCode code;
  const char *state;
  const char *summary;
  const char *hints[3];
  uint8_t hintCount;
};

constexpr bool diagnosticErrorRateHigh(uint32_t errors, uint64_t attempts) {
  return attempts >= 20U &&
         static_cast<uint64_t>(errors) * 100U >=
             static_cast<uint64_t>(attempts) * 15U;
}

constexpr MeterDiagnosisCode evaluateMeterDiagnosisCode(
    uint32_t bytes, uint32_t telegrams, uint32_t parseErrors,
    uint32_t /*crcErrors*/, bool fresh, bool hasPower, bool hasImport,
    bool hasExport) {
  return !bytes
             ? MeterDiagnosisCode::NoSignal
             : !telegrams
                   ? MeterDiagnosisCode::NoTelegram
                   : !fresh
                         ? MeterDiagnosisCode::Stale
                         : (!hasPower &&
                            diagnosticErrorRateHigh(
                                parseErrors,
                                static_cast<uint64_t>(telegrams) + parseErrors))
                               ? MeterDiagnosisCode::ParseUnstable
                               : !hasPower
                                     ? MeterDiagnosisCode::PartialValues
                                     : (!hasImport || !hasExport)
                                           ? MeterDiagnosisCode::MissingEnergy
                                           : MeterDiagnosisCode::Complete;
}

// Compile-time regression cases for the user-facing diagnosis states.
static_assert(evaluateMeterDiagnosisCode(0, 0, 0, 0, false, false, false,
                                         false) ==
                  MeterDiagnosisCode::NoSignal,
              "no bytes must report no IR signal");
static_assert(evaluateMeterDiagnosisCode(400, 0, 0, 0, false, false, false,
                                         false) ==
                  MeterDiagnosisCode::NoTelegram,
              "bytes without telegrams must be distinguished");
static_assert(evaluateMeterDiagnosisCode(4000, 10, 0, 0, true, true, false,
                                         false) ==
                  MeterDiagnosisCode::MissingEnergy,
              "power with missing energy values is a partial result");
static_assert(evaluateMeterDiagnosisCode(4000, 30, 0, 0, true, true, false,
                                         true) ==
                  MeterDiagnosisCode::MissingEnergy,
              "one missing energy counter must not be a communication error");
static_assert(evaluateMeterDiagnosisCode(4000, 30, 0, 0, true, true, true,
                                         true) ==
                  MeterDiagnosisCode::Complete,
              "complete stable readings must report OK");
static_assert(evaluateMeterDiagnosisCode(286026, 437, 0, 197, true, true,
                                         true, true) ==
                  MeterDiagnosisCode::Complete,
              "CRC synchronization events must not imply lost telegrams");
static_assert(evaluateMeterDiagnosisCode(29012, 45, 0, 12, true, true, true,
                                         true) ==
                  MeterDiagnosisCode::Complete,
              "rising CRC events with fresh valid data stay healthy");
static_assert(evaluateMeterDiagnosisCode(286026, 437, 0, 197, false, true,
                                         true, true) ==
                  MeterDiagnosisCode::Stale,
              "stale valid data must remain a warning");
static_assert(evaluateMeterDiagnosisCode(4096, 0, 0, 80, false, false, false,
                                         false) ==
                  MeterDiagnosisCode::NoTelegram,
              "RX data without a valid telegram must be explicit");
static_assert(evaluateMeterDiagnosisCode(4096, 10, 20, 0, true, false,
                                         false, false) ==
                  MeterDiagnosisCode::ParseUnstable,
              "parse failures plus missing power must warn");

MeterDiagnosis meterDiagnosis() {
  const bool fresh =
      meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
  const MeterDiagnosisCode code = evaluateMeterDiagnosisCode(
      meter.bytes, meter.telegrams, meter.parseErrors, meter.crcErrors, fresh,
      std::isfinite(meter.powerW), std::isfinite(meter.importKwh),
      std::isfinite(meter.exportKwh));
  switch (code) {
    case MeterDiagnosisCode::NoSignal:
      return {code, "error", "Kein IR-Signal erkannt.",
              {"Lesekopfposition prüfen.",
               "Optische Schnittstelle und Freischaltung prüfen.",
               "RX-Konfiguration prüfen."},
              3};
    case MeterDiagnosisCode::NoTelegram:
      return {code, meter.bytes >= 512U || millis() >= 60000U ? "error" : "warn",
              "IR-Daten werden empfangen, aber kein vollständiges Zählertelegramm erkannt.",
              {"Protokoll oder Auto-Modus prüfen.", "Baudrate prüfen.",
               "Supportbericht senden, falls der Zustand bestehen bleibt."},
              3};
    case MeterDiagnosisCode::Stale:
      return {code, "warn", "Der Zähler wurde erkannt, liefert momentan aber keine aktuellen Daten.",
              {"Lesekopfposition und optische Schnittstelle prüfen.",
               "Prüfen, ob der Zähler weiterhin Telegramme sendet.", nullptr},
              2};
    case MeterDiagnosisCode::ParseUnstable:
      return {code, "warn", "Zählertelegramme werden empfangen, können aber nicht vollständig ausgewertet werden.",
              {"Protokoll und Baudrate prüfen.",
               "Für unbekannte Zähler einen technischen Supportbericht senden.",
               nullptr},
              2};
    case MeterDiagnosisCode::IntegrityUnstable:
      return {code, "warn", "Die optische Übertragung ist möglicherweise fehlerhaft oder instabil.",
              {"Lesekopfposition und Fremdlicht prüfen.",
               "Einzelne sporadische Fehler sind unkritisch; bewertet wird erst ab 20 Versuchen und 15 Prozent Fehlerquote.",
               nullptr},
              2};
    case MeterDiagnosisCode::PartialValues:
      return {code, "warn", "Das Zählertelegramm wird erkannt, aber die Momentanleistung ist nicht verfügbar.",
              {"Freischaltung der erweiterten Zählerwerte prüfen.",
               "Verfügbare OBIS-Werte im technischen Bericht prüfen.", nullptr},
              2};
    case MeterDiagnosisCode::MissingEnergy:
      if (!std::isfinite(meter.importKwh) &&
          !std::isfinite(meter.exportKwh))
        return {code, "warn", "Der Zähler ist erkannt, liefert aber nicht alle Energiezählerstände.",
                {"Bezug-Zählerstand nicht verfügbar.",
                 "Einspeisezählerstand nicht verfügbar.",
                 "Fehlende Werte sind möglicherweise am Zähler noch nicht freigeschaltet."},
                3};
      return {code, "warn", "Der Zähler ist erkannt, liefert aber nicht alle Energiezählerstände.",
              {std::isfinite(meter.importKwh)
                   ? "Einspeisezählerstand nicht verfügbar."
                   : "Bezug-Zählerstand nicht verfügbar.",
               "Fehlende Werte sind möglicherweise am Zähler noch nicht freigeschaltet.",
               nullptr},
              2};
    case MeterDiagnosisCode::Complete:
    default:
      return {code, "ok", "Zählerdaten werden stabil empfangen.",
              {"Momentanleistung, Bezug und Einspeisung sind verfügbar.",
               nullptr, nullptr},
              1};
  }
}

const char *meterDiagnosisCodeName(MeterDiagnosisCode code) {
  switch (code) {
    case MeterDiagnosisCode::NoSignal: return "no_signal";
    case MeterDiagnosisCode::NoTelegram: return "no_telegram";
    case MeterDiagnosisCode::Stale: return "stale";
    case MeterDiagnosisCode::ParseUnstable: return "parse_unstable";
    case MeterDiagnosisCode::IntegrityUnstable: return "integrity_unstable";
    case MeterDiagnosisCode::PartialValues: return "partial_values";
    case MeterDiagnosisCode::MissingEnergy: return "missing_energy";
    case MeterDiagnosisCode::Complete: return "complete";
    default: return "unknown";
  }
}

const char *diagnosisStateLabel(const char *state) {
  if (!strcmp(state, "ok")) return "OK";
  if (!strcmp(state, "error")) return "Fehler";
  return "Hinweis";
}

String diagnosticDuration(uint32_t totalSeconds) {
  const uint32_t days = totalSeconds / 86400U;
  const uint32_t hours = (totalSeconds % 86400U) / 3600U;
  const uint32_t minutes = (totalSeconds % 3600U) / 60U;
  const uint32_t seconds = totalSeconds % 60U;
  String value;
  value.reserve(40);
  if (days) value += String(days) + " d ";
  if (days || hours) value += String(hours) + " h ";
  if (days || hours || minutes) value += String(minutes) + " min ";
  value += String(seconds) + " s";
  return value;
}

String diagnosticValue(double value, uint8_t decimals, const char *unit) {
  if (!std::isfinite(value)) return "nicht verfügbar";
  return String(value, static_cast<unsigned int>(decimals)) + " " + unit;
}

String diagnosticIntegrityText() {
  if (!meter.lastIntegrityPresent)
    return "vom Protokoll nicht geliefert";
  return meter.lastCrcValid ? "gültig" : "fehlerhaft";
}

MeterProtocol activeDiagnosisProtocol() {
  return meter.detectedProtocol != MeterProtocol::Auto
             ? meter.detectedProtocol
             : config.meterProtocol;
}

uint32_t activeIntegrityErrors() {
  return activeDiagnosisProtocol() == MeterProtocol::Sml
             ? meter.smlCrcErrors
             : (isD0Protocol(activeDiagnosisProtocol()) ? d0BccErrors()
                                                        : meter.crcErrors);
}

const char *activeIntegrityLabel() {
  return activeDiagnosisProtocol() == MeterProtocol::Sml
             ? "SML CRC-Fehler"
             : (isD0Protocol(activeDiagnosisProtocol())
                    ? "D0 BCC-Fehler"
                    : "Integritätsereignisse gesamt");
}

String selfTestJson() {
  const bool meterFresh =
      meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
  const MeterDiagnosis diagnosis = meterDiagnosis();
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
  add("meter", "Zählerempfang", diagnosis.state, diagnosis.summary, first);
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
  const MeterDiagnosis diagnosis = meterDiagnosis();
  String json;
  json.reserve(2600);
  json = "{\"manufacturer\":\"unknown\",\"model\":\"electricity meter\",";
  json += "\"protocol\":\"" +
          String(meterProtocolName(meter.detectedProtocol)) + "\",";
  json += "\"configured_protocol\":\"" +
          String(meterProtocolName(config.meterProtocol)) + "\",";
  json += "\"telegram_age_s\":" + ageOrNull(meter.lastTelegramMs) + ",";
  json += "\"telegram_fresh\":" + String(fresh ? "true" : "false") + ",";
  json += "\"telegram_count\":" + String(meter.telegrams) + ",";
  json += "\"rx_bytes\":" + String(meter.bytes) + ",";
  json += "\"parse_errors\":" + String(meter.parseErrors) + ",";
  json += "\"crc_errors\":" + String(meter.crcErrors) + ",";
  json += "\"sml_crc_errors\":" + String(meter.smlCrcErrors) + ",";
  json += "\"d0_bcc_errors\":" + String(d0BccErrors()) + ",";
  json += "\"integrity_present\":" +
          String(meter.lastIntegrityPresent ? "true" : "false") + ",";
  json += "\"last_crc_valid\":" +
          String(meter.lastCrcValid ? "true" : "false") + ",";
  json += "\"diagnosis\":{\"state\":\"" + String(diagnosis.state) +
          "\",\"code\":\"" + meterDiagnosisCodeName(diagnosis.code) +
          "\",\"summary\":\"" + jsonEscape(diagnosis.summary) +
          "\",\"hints\":[";
  for (uint8_t i = 0; i < diagnosis.hintCount; ++i) {
    if (i) json += ",";
    json += "\"" + jsonEscape(diagnosis.hints[i]) + "\"";
  }
  json += "]},";
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
            ",\"power_w\":" + numberOrNull(meter.phasePowerW[phase]) +
            ",\"voltage_obis\":\"" + String(voltageCodes[phase]) +
            ".7.0\",\"voltage_received\":" +
            String(std::isfinite(meter.phaseVoltageV[phase]) ? "true"
                                                             : "false") +
            ",\"voltage_v\":" + numberOrNull(meter.phaseVoltageV[phase]) +
            ",\"current_obis\":\"" + String(currentCodes[phase]) +
            ".7.0\",\"current_received\":" +
            String(std::isfinite(meter.phaseCurrentA[phase]) ? "true"
                                                             : "false") +
            ",\"current_a\":" + numberOrNull(meter.phaseCurrentA[phase]) +
            "}";
  }
  json += "],\"values\":{\"power_w\":" + numberOrNull(meter.powerW) +
          ",\"import_kwh\":" + numberOrNull(meter.importKwh, 6) +
          ",\"export_kwh\":" + numberOrNull(meter.exportKwh, 6) + "},";
  const SmlParser::Diagnostics parserDiagnostics = smlParser.diagnostics();
  json += "\"sml_parser\":{\"qualification_matches\":" +
          String(parserDiagnostics.qualificationMatches) +
          ",\"qualification_target\":" +
          String(parserDiagnostics.qualificationTarget) +
          ",\"sentinel_interval\":" +
          String(parserDiagnostics.sentinelInterval) +
          ",\"qualification_complete\":" +
          String(parserDiagnostics.qualificationComplete ? "true" : "false") +
          ",\"one_pass_active\":" +
          String(parserDiagnostics.onePassActive ? "true" : "false") +
          ",\"legacy_fallback_latched\":" +
          String(parserDiagnostics.legacyFallbackLatched ? "true" : "false") +
          ",\"comparison_mismatches\":" +
          String(parserDiagnostics.comparisonMismatches) +
          ",\"sentinel_comparisons\":" +
          String(parserDiagnostics.sentinelComparisons) + "},";
  json += "\"system\":{\"transport\":\"" +
          String(primaryTransportName()) + "\",\"wifi_rssi\":";
  json += (wifiConnected() ? String(WiFi.RSSI()) : "null");
  json += ",\"free_heap\":" + String(ESP.getFreeHeap()) +
          ",\"minimum_free_heap\":" + String(ESP.getMinFreeHeap()) +
          ",\"history_ready\":" + String(history.ready() ? "true" : "false") +
          ",\"mqtt_state\":\"";
  json += !config.mqttHost.length()
              ? "not_configured"
              : (mqtt.connected() ? "connected" : "disconnected");
  json += "\"},\"ir_tx\":{\"gpio\":" + String(config.txPin) +
          ",\"inverted\":" + String(config.pinInverted ? "true" : "false") +
          ",\"last_sequence_result\":\"";
  json += std::isfinite(meter.powerW)
              ? "extended_dataset_received"
              : "no_extended_dataset_no_meter_ack_available";
  json += "\"},\"note\":\"Spannung und Strom werden nur live im RAM gehalten und nicht historisch gespeichert.\"}";
  return json;
}

void appendDiagnosticPhase(String &report, uint8_t phase) {
  report += "L" + String(phase + 1) + ": ";
  bool present = false;
  if (std::isfinite(meter.phasePowerW[phase])) {
    report += String(meter.phasePowerW[phase], 1) + " W";
    present = true;
  }
  if (std::isfinite(meter.phaseVoltageV[phase])) {
    if (present) report += " | ";
    report += String(meter.phaseVoltageV[phase], 1) + " V";
    present = true;
  }
  if (std::isfinite(meter.phaseCurrentA[phase])) {
    if (present) report += " | ";
    report += String(meter.phaseCurrentA[phase], 3) + " A";
    present = true;
  }
  if (!present) report += "nicht verfügbar";
  report += "\n";
}

String supportReportText(bool technical) {
  const MeterDiagnosis diagnosis = meterDiagnosis();
  String report;
  report.reserve(technical ? 3200 : 1800);
  report = "IR Tracker Offline – Diagnosebericht\n";
  report += "Firmware: " + String(kFirmwareVersion) + "\n";
  report += "Laufzeit: " + diagnosticDuration(millis() / 1000U) + "\n\n";

  report += "=== ZÄHLER ===\n";
  report += "Status: " + String(diagnosisStateLabel(diagnosis.state)) + "\n";
  report += "Konfiguriertes Protokoll: " +
            String(meterProtocolName(config.meterProtocol)) + "\n";
  report += "Erkanntes Protokoll: " +
            String(meterProtocolName(meter.detectedProtocol)) + "\n";
  report += "RX-Bytes: " + String(meter.bytes) + "\n";
  report += "Telegramme: " + String(meter.telegrams) + "\n";
  report += "Letztes Telegramm: ";
  report += meter.lastTelegramMs ? String(valueAgeSeconds(meter.lastTelegramMs)) +
                                      " Sekunden"
                                : "nicht verfügbar";
  report += "\nParserfehler: " + String(meter.parseErrors) + "\n";
  report += String(activeIntegrityLabel()) + ": " +
            String(activeIntegrityErrors()) + "\n";
  report += "Letzte Integritätsprüfung: " + diagnosticIntegrityText() +
            "\n\n";

  report += "=== MESSWERTE ===\n";
  report += "Momentanleistung: " + diagnosticValue(meter.powerW, 1, "W") +
            "\n";
  report += "Bezug gesamt: " + diagnosticValue(meter.importKwh, 6, "kWh") +
            "\n";
  report += "Einspeisung gesamt: " +
            diagnosticValue(meter.exportKwh, 6, "kWh") + "\n";
  for (uint8_t phase = 0; phase < 3; ++phase)
    appendDiagnosticPhase(report, phase);

  report += "\n=== NETZWERK ===\n";
  report += "Verbindung: " + String(primaryTransportName()) + "\n";
  if (wifiConnected()) report += "Signal: " + String(WiFi.RSSI()) + " dBm\n";
  report += "MQTT: ";
  report += !config.mqttHost.length()
                ? "nicht konfiguriert"
                : (mqtt.connected() ? "verbunden" : "nicht verbunden");
  report += "\n\n=== SYSTEM ===\n";
  report += "Freier Heap: " + String(ESP.getFreeHeap()) + " Bytes\n";
  report += "Minimaler Heap: " + String(ESP.getMinFreeHeap()) + " Bytes\n";
  report += "Historie: " + String(history.ready() ? "bereit" : "Fehler") +
            "\n";

  report += "\n=== AUTOMATISCHE BEWERTUNG ===\n";
  report += diagnosis.summary;
  report += "\n";
  for (uint8_t i = 0; i < diagnosis.hintCount; ++i)
    report += "- " + String(diagnosis.hints[i]) + "\n";

  if (technical) {
    const SmlParser::Diagnostics parserDiagnostics = smlParser.diagnostics();
    const char *parserMode = parserDiagnostics.legacyFallbackLatched
                                 ? "legacy-fallback"
                                 : (parserDiagnostics.onePassActive
                                        ? "one-pass"
                                        : "legacy-qualifikation");
    report += "\n=== TECHNISCHE DETAILS ===\n";
    report += "RX-GPIO: " + String(config.rxPin) + "\n";
    report += "TX-GPIO: " + String(config.txPin) + "\n";
    report += "Baudrate: " + String(meterBaud()) + "\n";
    report += "Telegrammlänge: " + String(lastTelegram.size()) + " Bytes\n";
    report += "OBIS 1.8.0 Bezug: " +
              String(std::isfinite(meter.importKwh) ? "vorhanden" : "fehlt") +
              "\n";
    report += "OBIS 2.8.0 Einspeisung: " +
              String(std::isfinite(meter.exportKwh) ? "vorhanden" : "fehlt") +
              "\n";
    report += "OBIS 16.7.0 Leistung: " +
              String(std::isfinite(meter.powerW) ? "vorhanden" : "fehlt") +
              "\n";
    report += "Reset-Ursache: " + bootResetReason + "\n";
    report += "Largest Free Heap Block: " +
              String(largestFreeHeapBlockBytes()) + " Bytes\n";
    report += "Stack Reserve (High Water Mark): " +
              String(loopStackHighWaterMarkBytes()) + " Bytes\n";
    report += "SML-Parser-Modus: " + String(parserMode) + "\n";
    report += "Qualifikation: " +
              String(parserDiagnostics.qualificationMatches) + "/" +
              String(parserDiagnostics.qualificationTarget) + "\n";
    report += "One-Pass aktiv: " +
              String(parserDiagnostics.onePassActive ? "ja" : "nein") +
              "\n";
    report += "Legacy-Fallback: " +
              String(parserDiagnostics.legacyFallbackLatched ? "ja" : "nein") +
              "\n";
    report += "Vergleichsabweichungen: " +
              String(parserDiagnostics.comparisonMismatches) + "\n";
    report += "Kontrollvergleiche: " +
              String(parserDiagnostics.sentinelComparisons) + "\n";
    report += "SML CRC-Fehler: " + String(meter.smlCrcErrors) + "\n";
    report += "D0 BCC-Fehler: " + String(d0BccErrors()) + "\n";
    report += "Integritätsereignisse gesamt (Legacy): " +
              String(meter.crcErrors) + "\n";
    if (activeDiagnosisProtocol() == MeterProtocol::Sml && d0BccErrors())
      report += "D0-BCC-Ereignisse stammen aus der parallelen Auto-Erkennung und werden nicht als SML-Übertragungsfehler bewertet.\n";
    if (activeIntegrityErrors() && !strcmp(diagnosis.state, "ok"))
      report += "Integritätsereignisse erkannt; gültiger Datenstrom stabil.\n";
    report += "Bewertungscode: " +
              String(meterDiagnosisCodeName(diagnosis.code)) + "\n";
  }
  return report;
}
