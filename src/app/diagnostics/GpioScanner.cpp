// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void restoreMeterSerialAfterScan() {
  restoreConfiguredMeterSerial();
  if (config.ledPin >= 0) {
    pinMode(config.ledPin, OUTPUT);
    digitalWrite(config.ledPin, config.ledInverted);
  }
}

void finishGpioScan(bool found, const String &error = "") {
  gpioScan.active = false;
  gpioScan.complete = true;
  gpioScan.found = found;
  gpioScan.error = error;
  restoreMeterSerialAfterScan();
  eventLog.add(found ? "INFO" : "WARN", "GPIO_SCAN",
               found ? "IR-Eingang durch CRC-gueltiges SML-Telegramm bestaetigt"
                     : "GPIO-Suche ohne gueltiges SML-Telegramm beendet");
}

void beginGpioScanCandidate() {
  if (gpioScan.pinIndex >= gpioScan.pinCount) {
    finishGpioScan(false, "no_valid_meter_telegram");
    return;
  }
  gpioScan.currentPin = gpioScan.pins[gpioScan.pinIndex];
  gpioScan.currentBaud = gpioScan.bauds[gpioScan.baudIndex];
  meterSerial.end();
  resetSmlCapture();
  pinMode(gpioScan.currentPin, INPUT);
  meterSerial.begin(gpioScan.currentBaud, SERIAL_8N1,
                    gpioScan.currentPin, -1);
  gpioScan.baselineTelegrams = meter.telegrams;
  gpioScan.candidateStartedMs = millis();
}

void startGpioScan() {
  if (gpioScan.active) return;
  if (activeD0.active) finishActiveD0Attempt();
  irPulse.active = false;
  apatorUnlock.active = false;
  gpioScan = GpioScanState{};
  gpioScan.active = true;

  // DE: Den aktuellen Wert zuerst wirklich pruefen, danach alle anderen
  // zulaessigen C3-Trackerpins. | EN: Really test the current value first,
  // followed by every other allowed C3 tracker pin.
  if (trackerGpioAvailable(config.rxPin))
    gpioScan.pins[gpioScan.pinCount++] = config.rxPin;
  for (uint8_t pin = 0; pin <= 10; ++pin)
    if (pin != config.rxPin && trackerGpioAvailable(pin))
      gpioScan.pins[gpioScan.pinCount++] = pin;

  gpioScan.bauds[gpioScan.baudCount++] = config.baud;
  const uint32_t commonBauds[] = {300, 600, 1200, 2400, 4800,
                                  9600, 19200, 38400, 115200};
  for (uint32_t baud : commonBauds)
    if (baud != config.baud) gpioScan.bauds[gpioScan.baudCount++] = baud;
  gpioScan.total = gpioScan.pinCount * gpioScan.baudCount;
  requestCpuBoost("gpio_scan");
  beginGpioScanCandidate();
}

void updateGpioScan() {
  if (!gpioScan.active) return;
  if (meter.telegrams > gpioScan.baselineTelegrams &&
      meter.lastCrcValid &&
      meter.lastTelegramMs >= gpioScan.candidateStartedMs) {
    gpioScan.foundPin = gpioScan.currentPin;
    gpioScan.foundBaud = gpioScan.currentBaud;
    ++gpioScan.tested;
    finishGpioScan(true);
    return;
  }
  if (millis() - gpioScan.candidateStartedMs < kGpioScanWindowMs) return;
  ++gpioScan.tested;
  if (++gpioScan.baudIndex >= gpioScan.baudCount) {
    gpioScan.baudIndex = 0;
    ++gpioScan.pinIndex;
  }
  beginGpioScanCandidate();
}

String gpioScanJson() {
  String json = "{\"supported\":true,\"active\":";
  json += gpioScan.active ? "true" : "false";
  json += ",\"complete\":";
  json += gpioScan.complete ? "true" : "false";
  json += ",\"found\":";
  json += gpioScan.found ? "true" : "false";
  json += ",\"tested\":" + String(gpioScan.tested) +
          ",\"total\":" + String(gpioScan.total) +
          ",\"current_pin\":" + String(gpioScan.currentPin) +
          ",\"current_baud\":" + String(gpioScan.currentBaud) +
          ",\"found_pin\":" + String(gpioScan.foundPin) +
          ",\"found_baud\":" + String(gpioScan.foundBaud) +
          ",\"error\":\"" + jsonEscape(gpioScan.error) + "\"}";
  return json;
}
