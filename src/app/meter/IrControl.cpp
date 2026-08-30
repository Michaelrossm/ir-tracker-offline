// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void setIrPulseOutput(bool active) {
  if (irPulse.pin < 0) return;
  digitalWrite(irPulse.pin, active ^ irPulse.inverted);
  irPulse.outputActive = active;
}

void finishIrPulseJob() {
  const int8_t pulsePin = irPulse.pin;
  if (pulsePin >= 0) {
    setIrPulseOutput(false);
    delay(2);
    if (pulsePin != config.txPin) pinMode(pulsePin, INPUT);
  }
  irPulse.active = false;
  irPulse.outputActive = false;
  restoreConfiguredMeterSerial();
  if (config.ledPin >= 0) {
    pinMode(config.ledPin, OUTPUT);
    digitalWrite(config.ledPin, config.ledInverted);
  }
}

bool beginIrPulseJob(const uint8_t digits[4], uint16_t pulseMs,
                     uint16_t digitGapMs, bool inverted,
                     int8_t outputPin = -1) {
  const int8_t selectedPin = outputPin >= 0 ? outputPin : config.txPin;
  if (selectedPin < 0 || selectedPin > 10 || irPulse.active ||
      gpioScan.active || selectedPin == config.rxPin)
    return false;
  if (activeD0.active) finishActiveD0Attempt();
  meterSerial.end();
  pinMode(selectedPin, OUTPUT);
  irPulse = {};
  irPulse.active = true;
  irPulse.pin = selectedPin;
  irPulse.inverted = inverted;
  irPulse.pulseMs = pulseMs;
  irPulse.pulseGapMs = pulseMs;
  irPulse.digitGapMs = digitGapMs;
  memcpy(irPulse.digits, digits, sizeof(irPulse.digits));
  irPulse.pulsesRemaining = irPulse.digits[0] ? irPulse.digits[0] : 10;
  setIrPulseOutput(false);
  irPulse.nextChangeMs = millis() + 250;
  return true;
}

bool beginIrPulseCount(uint8_t count, uint16_t pulseMs, bool inverted) {
  if (!count || config.txPin < 0 || irPulse.active) return false;
  meterSerial.end();
  pinMode(config.txPin, OUTPUT);
  irPulse = {};
  irPulse.active = true;
  irPulse.pin = config.txPin;
  irPulse.inverted = inverted;
  irPulse.pulseMs = pulseMs;
  irPulse.pulseGapMs = pulseMs >= 2000 ? 300 : pulseMs;
  irPulse.digitGapMs = 500;
  irPulse.digits[0] = count;
  irPulse.digits[1] = irPulse.digits[2] = irPulse.digits[3] = 0xff;
  irPulse.pulsesRemaining = count;
  setIrPulseOutput(false);
  irPulse.nextChangeMs = millis() + 250;
  return true;
}

void handleApatorUnlock() {
  if (!requireAdmin()) return;
  if (apatorUnlock.active || irPulse.active) {
    server.send(409, "application/json",
                "{\"error\":\"ir_sequence_already_running\"}");
    return;
  }
  if (config.meterPin.length() != 4 || config.txPin < 0) {
    server.send(400, "application/json",
                "{\"error\":\"saved_pin_or_ir_output_missing\"}");
    return;
  }
  apatorUnlock = {};
  apatorUnlock.active = true;
  apatorUnlock.phase = 1;
  apatorUnlock.nextMs = millis();
  eventLog.add("INFO", "METER_UNLOCK",
               "Optionale Zählerfreischaltung gestartet");
  server.send(202, "text/html; charset=utf-8",
              page("Zähler wird freigeschaltet",
                   "<div class='card'><h2>Automatische IR-Sequenz läuft</h2>"
                   "<p>Dauer ungefähr eine Minute. Tracker und Zähler währenddessen nicht bewegen.</p>"
                   "<p>Anschließend unter Messwerte prüfen, ob die aktuelle Leistung erscheint.</p>"
                   "<a href='/maintenance/diagnostics'>Status anzeigen</a></div>"));
}

void updateApatorUnlock() {
  if (!apatorUnlock.active || irPulse.active ||
      static_cast<int32_t>(millis() - apatorUnlock.nextMs) < 0)
    return;
  if (apatorUnlock.phase == 1) {
    // DE: Apator: Zwei kurze Impulse öffnen PIN-Menü/LCD-Test. | EN: Apator: two short flashes open the PIN menu/LCD test.
    if (beginIrPulseCount(2, 300, config.pinInverted)) {
      eventLog.add("INFO", "APATOR_STEP",
                   "1/4 Initialisierung: 2 kurze Impulse gesendet");
      apatorUnlock.phase = 2;
      apatorUnlock.nextMs = 0;
    }
    return;
  }
  if (apatorUnlock.phase == 2) {
    apatorUnlock.phase = 3;
    apatorUnlock.nextMs = millis() + 5000;
    return;
  }
  if (apatorUnlock.phase == 3) {
    uint8_t digits[4];
    for (uint8_t i = 0; i < 4; ++i) digits[i] = config.meterPin[i] - '0';
    if (beginIrPulseJob(digits, config.pinPulseMs, config.pinDigitGapMs,
                        config.pinInverted)) {
      eventLog.add("INFO", "APATOR_STEP",
                   "2/4 Gespeicherte PIN als Impulsfolge gesendet");
      apatorUnlock.phase = 4;
      apatorUnlock.nextMs = 0;
    }
    return;
  }
  if (apatorUnlock.phase == 4) {
    apatorUnlock.phase = 5;
    apatorUnlock.nextMs = millis() + 3500;
    return;
  }
  if (apatorUnlock.phase == 5) {
    // DE: Apator-Zweirichtungszähler: vom PIN-Ergebnis zu Inf OFF/ON. | EN: Two-way Apator meter: advance from PIN result to Inf OFF/ON.
    if (beginIrPulseCount(13, 300, config.pinInverted)) {
      eventLog.add("INFO", "APATOR_STEP",
                   "3/4 Navigation: 13 kurze Impulse gesendet");
      apatorUnlock.phase = 6;
      apatorUnlock.nextMs = 0;
    }
    return;
  }
  if (apatorUnlock.phase == 6) {
    apatorUnlock.phase = 7;
    apatorUnlock.nextMs = millis() + 800;
    return;
  }
  if (apatorUnlock.phase == 7) {
    // DE: Langer Impuls schaltet Inf von OFF auf ON. | EN: A long flash toggles Inf from OFF to ON.
    if (beginIrPulseCount(1, 5500, config.pinInverted)) {
      eventLog.add("INFO", "APATOR_STEP",
                   "4/4 Inf-Umschaltung: langer Impuls gesendet");
      apatorUnlock.phase = 8;
      apatorUnlock.nextMs = 0;
    }
    return;
  }
  if (apatorUnlock.phase == 8) {
    if (!apatorUnlock.verifyUntilMs) {
      apatorUnlock.verifyUntilMs = millis() + 90000;
      apatorUnlock.nextMs = millis() + 2000;
      return;
    }
    if (std::isfinite(meter.powerW)) {
      eventLog.add("INFO", "APATOR_UNLOCK_OK",
                   "Inf ON bestätigt: Momentanleistung empfangen");
      apatorUnlock.active = false;
    } else if (static_cast<int32_t>(millis() -
                                    apatorUnlock.verifyUntilMs) >= 0) {
      eventLog.add("ERROR", "APATOR_UNLOCK_FAIL",
                   "Kein 16.7.0: Zähler bestätigt Lichtimpulse nicht über SML; Display/IR-TX prüfen");
      apatorUnlock.active = false;
    } else {
      apatorUnlock.nextMs = millis() + 2000;
    }
  }
}

void handleIrPin() {
  if (!requireAdmin()) return;
  String pin = server.arg("pin");
  if (!pin.length()) pin = config.meterPin;
  if (pin.length() != 4) {
    server.send(400, "application/json", "{\"error\":\"pin_must_have_4_digits\"}");
    return;
  }
  uint8_t digits[4];
  for (uint8_t i = 0; i < 4; ++i) {
    if (!isDigit(pin[i])) {
      server.send(400, "application/json", "{\"error\":\"pin_must_be_numeric\"}");
      return;
    }
    digits[i] = pin[i] - '0';
  }
  const uint16_t pulseMs = constrain(server.arg("pulse_ms").toInt(), 50, 1000);
  const uint16_t digitGapMs = constrain(server.arg("digit_gap_ms").toInt(), 1000, 10000);
  const bool inverted = server.hasArg("invert");
  if (server.hasArg("save_pin")) config.meterPin = pin;
  config.autoPin = false;
  config.pinInverted = inverted;
  config.pinPulseMs = pulseMs;
  config.pinDigitGapMs = digitGapMs;
  saveConfig();
  eventLog.add("INFO", "PIN_SEND",
               server.hasArg("save_pin") ? "PIN gesendet und gespeichert"
                                         : "PIN gesendet");
  if (!beginIrPulseJob(digits, pulseMs, digitGapMs, inverted)) {
    server.send(409, "application/json", "{\"error\":\"ir_busy_or_disabled\"}");
    return;
  }
  server.send(202, "text/html; charset=utf-8",
              page("IR-PIN wird gesendet",
                   "<p>Die vier Ziffern werden jetzt nicht blockierend gesendet.</p>"
                   "<p><a href='/maintenance/diagnostics'>Zur Diagnose</a></p>"));
}

void handleForgetPin() {
  if (!requireAdmin()) return;
  config.meterPin = "";
  config.autoPin = false;
  saveConfig();
  eventLog.add("INFO", "PIN_FORGET", "Gespeicherte PIN gelöscht");
  server.sendHeader("Location", "/maintenance/diagnostics", true);
  server.send(303, "text/plain", "");
}

void handleIrPulse() {
  if (!requireAdmin()) return;
  uint8_t digits[4] = {1, 0, 0, 0};
  if (!beginIrPulseJob(digits, 300, 1000, server.hasArg("invert"))) {
    server.send(409, "application/json", "{\"error\":\"ir_busy_or_disabled\"}");
    return;
  }
  // DE: Einzelimpulstest endet nach der ersten Ziffer. | EN: A single-pulse test ends after its first digit.
  irPulse.digits[1] = irPulse.digits[2] = irPulse.digits[3] = 0xff;
  server.send(202, "application/json", "{\"accepted\":true,\"pulses\":1}");
}

void handleGpioOutputTest() {
  if (!requireAdmin()) return;
  const int pin = server.arg("pin").toInt();
  if (!server.hasArg("pin") || !trackerGpioAvailable(pin) ||
      pin == config.rxPin) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_or_rx_gpio\"}");
    return;
  }
  const uint8_t digits[4] = {1, 0xff, 0xff, 0xff};
  if (!beginIrPulseJob(digits, 350, 700, server.arg("inverted") == "1",
                       static_cast<int8_t>(pin))) {
    server.send(409, "application/json",
                "{\"error\":\"gpio_output_test_busy\"}");
    return;
  }
  eventLog.add("INFO", "GPIO_TX_TEST",
               "Kurzer Ausgangstest auf GPIO " + String(pin));
  server.send(202, "application/json",
              "{\"accepted\":true,\"pin\":" + String(pin) +
                  ",\"duration_ms\":350}");
}

struct DigitalSample {
  uint16_t highPermille = 0;
  uint16_t transitions = 0;
};

DigitalSample sampleDigitalPin(int8_t pin, uint32_t durationUs = 50000) {
  DigitalSample result;
  uint32_t high = 0;
  uint32_t total = 0;
  int previous = digitalRead(pin);
  const uint32_t started = micros();
  while (static_cast<int32_t>(micros() - started) <
         static_cast<int32_t>(durationUs)) {
    const int current = digitalRead(pin);
    high += current == HIGH;
    ++total;
    if (current != previous) {
      ++result.transitions;
      previous = current;
    }
    delayMicroseconds(80);
    if ((total & 63U) == 0) {
      esp_task_wdt_reset();
      yield();
    }
  }
  if (total) result.highPermille = high * 1000U / total;
  return result;
}

void handleGpioTxScan() {
  if (!requireAdmin()) return;
  const int rx = server.arg("rx").toInt();
  if (!server.hasArg("rx") || !trackerGpioAvailable(rx) || gpioScan.active ||
      irPulse.active) {
    server.send(400, "application/json", "{\"error\":\"invalid_rx_or_busy\"}");
    return;
  }
  requestCpuBoost("gpio_tx_scan");
  meterSerial.end();
  resetSmlCapture();
  pinMode(rx, INPUT);
  int8_t pins[10] = {};
  uint8_t pinCount = 0;
  if (config.txPin >= 0 && trackerGpioAvailable(config.txPin) &&
      config.txPin != rx)
    pins[pinCount++] = config.txPin;
  for (int8_t pin = 0; pin <= 10; ++pin)
    if (trackerGpioAvailable(pin) && pin != rx && pin != config.txPin)
      pins[pinCount++] = pin;

  int8_t foundPin = -1;
  bool foundInverted = false;
  uint8_t confidence = 0;
  uint8_t tested = 0;
  uint16_t foundActiveTransitions = 0;
  uint16_t foundIdleTransitions = 0;
  for (uint8_t index = 0; index < pinCount; ++index) {
    const int8_t pin = pins[index];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(20);
    const DigitalSample low1 = sampleDigitalPin(rx);
    digitalWrite(pin, HIGH);
    delay(20);
    const DigitalSample high1 = sampleDigitalPin(rx);
    digitalWrite(pin, LOW);
    delay(20);
    const DigitalSample low2 = sampleDigitalPin(rx);
    digitalWrite(pin, HIGH);
    delay(20);
    const DigitalSample high2 = sampleDigitalPin(rx);
    digitalWrite(pin, LOW);
    pinMode(pin, INPUT);
    ++tested;

    const uint16_t lowTransitions =
        (low1.transitions + low2.transitions) / 2U;
    const uint16_t highTransitions =
        (high1.transitions + high2.transitions) / 2U;
    const uint16_t lowDuty = (low1.highPermille + low2.highPermille) / 2U;
    const uint16_t highDuty =
        (high1.highPermille + high2.highPermille) / 2U;
    const bool activeLow = lowTransitions < highTransitions;
    const uint16_t activeTransitions =
        activeLow ? lowTransitions : highTransitions;
    const uint16_t idleTransitions =
        activeLow ? highTransitions : lowTransitions;
    const uint16_t activeDuty = activeLow ? lowDuty : highDuty;
    const bool saturated = activeDuty <= 120U || activeDuty >= 880U;
    const bool correlated = idleTransitions >= 12U &&
                            idleTransitions >= activeTransitions * 4U + 8U;
    if (saturated && correlated) {
      foundPin = pin;
      foundInverted = activeLow;
      foundActiveTransitions = activeTransitions;
      foundIdleTransitions = idleTransitions;
      const uint16_t transitionScore = std::min<uint16_t>(
          70U, (idleTransitions - activeTransitions) * 2U);
      const uint16_t saturationDistance =
          std::min<uint16_t>(activeDuty, 1000U - activeDuty);
      confidence = std::min<uint16_t>(
          100U, 30U + transitionScore +
                    (saturationDistance <= 50U ? 10U : 0U));
      break;
    }
  }
  restoreMeterSerialAfterScan();
  if (foundPin < 0) {
    eventLog.add("WARN", "GPIO_TX_SCAN",
                 "Kein eindeutiger optischer TX-Rueckkanal erkannt");
    server.send(200, "application/json",
                "{\"complete\":true,\"found\":false,\"tested\":" +
                    String(tested) +
                    ",\"error\":\"no_optical_loopback\"}");
    return;
  }
  eventLog.add("INFO", "GPIO_TX_SCAN",
               "IR-Sender durch wiederholte optische RX-Korrelation auf GPIO " +
                   String(foundPin) + " erkannt");
  server.send(200, "application/json",
              "{\"complete\":true,\"found\":true,\"pin\":" +
                  String(foundPin) + ",\"inverted\":" +
                  String(foundInverted ? "true" : "false") +
                  ",\"confidence\":" + String(confidence) +
                  ",\"tested\":" + String(tested) +
                  ",\"active_transitions\":" +
                  String(foundActiveTransitions) +
                  ",\"idle_transitions\":" +
                  String(foundIdleTransitions) + "}");
}

void handleIrStop() {
  if (!requireAdmin()) return;
  if (irPulse.active) finishIrPulseJob();
  server.sendHeader("Location", "/maintenance/diagnostics", true);
  server.send(303, "text/plain", "");
}

void updateIrPulseJob() {
  if (!irPulse.active || static_cast<int32_t>(millis() - irPulse.nextChangeMs) < 0) return;
  if (!irPulse.outputActive && irPulse.pulsesRemaining) {
    setIrPulseOutput(true);
    --irPulse.pulsesRemaining;
    irPulse.nextChangeMs = millis() + irPulse.pulseMs;
    return;
  }
  if (irPulse.outputActive) {
    setIrPulseOutput(false);
    irPulse.nextChangeMs = millis() + irPulse.pulseGapMs;
    return;
  }
  ++irPulse.digitIndex;
  if (irPulse.digitIndex >= 4 || irPulse.digits[irPulse.digitIndex] == 0xff) {
    finishIrPulseJob();
    return;
  }
  irPulse.pulsesRemaining =
      irPulse.digits[irPulse.digitIndex] ? irPulse.digits[irPulse.digitIndex] : 10;
  irPulse.nextChangeMs = millis() + irPulse.digitGapMs;
}

void manageAutoPin() {
  if (autoPinAttempted || !config.autoPin || config.meterPin.length() != 4 ||
      irPulse.active || apatorUnlock.active || millis() < 60000 ||
      meter.telegrams < 2 ||
      std::isfinite(meter.powerW)) {
    return;
  }
  uint8_t digits[4];
  for (uint8_t i = 0; i < 4; ++i) digits[i] = config.meterPin[i] - '0';
  autoPinAttempted = true;
  beginIrPulseJob(digits, config.pinPulseMs, config.pinDigitGapMs,
                  config.pinInverted);
}
