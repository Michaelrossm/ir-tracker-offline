// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

#if IR_TRACKER_ENABLE_FACTORY_TEST
void finishFactoryLoopback(const bool passed) {
  factoryTest.running = false;
  factoryTest.finished = true;
  factoryTest.loopbackPassed = passed;
  restoreConfiguredMeterSerial();
  eventLog.add(passed ? "INFO" : "ERROR", "FCT_IR_LOOPBACK",
               passed ? "IR-Sender und IR-Empfaenger per Pruefreflektor bestaetigt"
                      : "Kein passendes IR-Loopbackmuster empfangen");
}

bool startFactoryTest() {
  if (factoryTest.running || gpioScan.active || irPulse.active ||
      apatorUnlock.active)
    return false;
  if (activeD0.active) finishActiveD0Attempt();
  requestCpuBoost("factory_test");
  factoryTest = {};
  factoryTest.running = true;
  factoryTest.startedMs = millis();
  meterSerial.end();
  resetSmlCapture();
  meterSerial.begin(9600, SERIAL_8N1, config.rxPin, config.txPin);
  while (meterSerial.available()) meterSerial.read();
  if (config.ledPin >= 0) {
    pinMode(config.ledPin, OUTPUT);
    digitalWrite(config.ledPin, !config.ledInverted);
  }
  for (uint8_t repeat = 0; repeat < 3; ++repeat)
    meterSerial.write(kFactoryLoopbackPattern, sizeof(kFactoryLoopbackPattern));
  meterSerial.flush();
  eventLog.add("INFO", "FCT_START",
               "Werkspruefung gestartet; IR-Pruefreflektor erforderlich");
  return true;
}

void updateFactoryTest() {
  if (!factoryTest.running) return;
  while (meterSerial.available()) {
    const uint8_t value = meterSerial.read();
    if (value == kFactoryLoopbackPattern[factoryTest.matched]) {
      if (++factoryTest.matched == sizeof(kFactoryLoopbackPattern)) {
        finishFactoryLoopback(true);
        return;
      }
    } else {
      factoryTest.matched =
          value == kFactoryLoopbackPattern[0] ? 1 : 0;
    }
  }
  if (millis() - factoryTest.startedMs >= kFactoryLoopbackTimeoutMs)
    finishFactoryLoopback(false);
}

bool factoryAutomatedChecksPass() {
  return String(ESP.getChipModel()) == "ESP32-C3" &&
         ESP.getFlashChipSize() >= 4UL * 1024UL * 1024UL &&
         ESP.getFreeHeap() >= kHeapWarningBytes && history.ready() &&
         wifiConnected() && ethernet.hardwareDetected() &&
         ethernet.connected() && factoryTest.finished &&
         factoryTest.loopbackPassed && factoryTest.poeConfirmed;
}

String factoryTestJson() {
  const bool automated = factoryAutomatedChecksPass();
  const bool passed = automated && factoryTest.ledConfirmed;
  String json = "{\"state\":\"";
  json += factoryTest.running ? "running"
          : passed ? "pass"
          : factoryTest.finished && factoryTest.loopbackPassed &&
                    (!factoryTest.ledConfirmed || !factoryTest.poeConfirmed)
              ? "waiting"
          : factoryTest.finished ? "fail" : "idle";
  json += "\",\"progress\":";
  json += factoryTest.running
              ? String((100U * factoryTest.matched) /
                       sizeof(kFactoryLoopbackPattern))
              : String(factoryTest.finished ? 100 : 0);
  json += ",\"tests\":[";
  auto test = [&](const char *id, const char *state, const String &detail,
                  bool &first) {
    if (!first) json += ',';
    first = false;
    json += "{\"id\":\"" + String(id) + "\",\"state\":\"" + state +
            "\",\"detail\":\"" + jsonEscape(detail) + "\"}";
  };
  bool first = true;
  test("chip", String(ESP.getChipModel()) == "ESP32-C3" ? "pass" : "fail",
       String(ESP.getChipModel()) + ", Revision " + ESP.getChipRevision(), first);
  test("flash", ESP.getFlashChipSize() >= 4UL * 1024UL * 1024UL ? "pass" : "fail",
       String(ESP.getFlashChipSize() / (1024UL * 1024UL)) + " MiB", first);
  test("ram", ESP.getFreeHeap() >= kHeapWarningBytes ? "pass" : "fail",
       String(ESP.getFreeHeap()) + " Byte frei", first);
  test("storage", history.ready() ? "pass" : "fail",
       history.ready() ? "Historienpartition les- und schreibbereit"
                       : "Historienpartition nicht bereit", first);
  test("wifi", wifiConnected() ? "pass" : "fail",
       wifiConnected() ? WiFi.SSID() + " (" + String(WiFi.RSSI()) + " dBm)"
                       : "Keine WLAN-Verbindung", first);
  test("w5500", ethernet.hardwareDetected() ? "pass" : "fail",
       ethernet.hardwareDetected() ? "W5500 VERSIONR erkannt"
                                   : "W5500 nicht erkannt", first);
  test("ethernet", ethernet.connected() ? "pass" : "fail",
       ethernet.connected() ? ethernet.localIP().toString()
                            : "Keine LAN-Verbindung/DHCP-Adresse", first);
  test("ir_loopback",
       factoryTest.running ? "running"
       : factoryTest.loopbackPassed ? "pass" : "fail",
       factoryTest.loopbackPassed
           ? "TX GPIO " + String(config.txPin) + " -> RX GPIO " +
                 String(config.rxPin) + ": Testmuster fehlerfrei empfangen"
           : "Optischen Pruefreflektor zwischen Sender und Empfaenger einsetzen",
       first);
  test("led", factoryTest.ledConfirmed ? "pass" : "manual",
       factoryTest.ledConfirmed ? "Leuchtfunktion vom Bediener bestaetigt"
                                : "Sichtpruefung ausstehend", first);
  test("poe", factoryTest.poeConfirmed ? "pass" : "manual",
       factoryTest.poeConfirmed
           ? "Tracker blieb nach Trennen von USB/5 V ueber LAN erreichbar"
           : "Seite ueber LAN oeffnen, USB/5 V trennen und danach bestaetigen",
       first);
  test("ble", "skip",
       "Nicht Bestandteil der Produktfirmware; kein BLE-Stack eingebaut", first);
  json += "],\"automated_pass\":" + String(automated ? "true" : "false") +
          ",\"pass\":" + String(passed ? "true" : "false") + "}";
  return json;
}
#endif
