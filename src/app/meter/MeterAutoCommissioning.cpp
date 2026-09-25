// MeterAutoCommissioning.cpp
// Conservative auto-commissioning that REUSES existing SML/D0 parsers.
// No duplicate OBIS parser, GPIO scanner or meter-value model.

#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

struct AutoSerialCandidate {
  uint32_t baud;
  uint32_t mode;
};

constexpr AutoSerialCandidate kAutoSerialCandidates[] = {
    {9600, SERIAL_8N1},
    {9600, SERIAL_7E1},
    {300, SERIAL_7E1},
    {2400, SERIAL_7E1},
    {115200, SERIAL_8N1},
};

constexpr uint32_t kAutoSerialStartMs = 18UL * 1000UL;
constexpr uint32_t kAutoSerialWindowMs = 4800;
constexpr uint8_t kAutoSerialConfirmFrames = 2;

struct MeterCommissioningState {
  bool started = false;
  bool active = false;
  bool finished = false;
  bool found = false;
  bool suggestGpioScan = false;
  uint8_t candidate = 0;
  uint32_t candidateStartedMs = 0;
  uint32_t baselineTelegrams = 0;
  MeterProtocol foundProtocol = MeterProtocol::Auto;
  uint32_t foundBaud = 0;
  String message;
} meterCommissioning;

bool meterCommissioningOwnsSerial() { return meterCommissioning.active; }

MeterProtocol meterCommissioningProtocol() {
  return kAutoSerialCandidates[meterCommissioning.candidate].mode == SERIAL_8N1
             ? MeterProtocol::Sml : MeterProtocol::Iec62056;
}

void beginMeterCommissioningCandidate() {
  const AutoSerialCandidate &candidate =
      kAutoSerialCandidates[meterCommissioning.candidate];
  meterSerial.end();
  resetMeterParsers();
  meterSerial.begin(candidate.baud, candidate.mode, config.rxPin, config.txPin);
  meterCommissioning.baselineTelegrams = meter.telegrams;
  meterCommissioning.candidateStartedMs = millis();
  requestCpuBoost("meter_auto");
}

void finishMeterCommissioning(bool found) {
  meterCommissioning.active = false;
  meterCommissioning.finished = true;
  meterCommissioning.found = found;

  if (!found) {
    restoreConfiguredMeterSerial();
    meterCommissioning.suggestGpioScan = true;
    meterCommissioning.message =
        "Zaehler am konfigurierten IR-Eingang nicht automatisch erkannt";
    eventLog.add("WARN", "METER_AUTO_NONE", meterCommissioning.message);
    return;
  }

  const AutoSerialCandidate &candidate =
      kAutoSerialCandidates[meterCommissioning.candidate];
  meterCommissioning.foundProtocol = meter.detectedProtocol;
  meterCommissioning.foundBaud = candidate.baud;

  if (meter.detectedProtocol == MeterProtocol::Sml &&
      candidate.mode == SERIAL_8N1) {
    config.meterProtocol = MeterProtocol::Sml;
    config.baud = candidate.baud;
  } else if (meter.detectedProtocol == MeterProtocol::Iec62056 &&
             candidate.mode == SERIAL_7E1) {
    config.meterProtocol = MeterProtocol::Iec62056;
    config.baud = candidate.baud;
  } else {
    restoreConfiguredMeterSerial();
    meterCommissioning.found = false;
    meterCommissioning.suggestGpioScan = true;
    meterCommissioning.message =
        "Telegramm erkannt, automatische Konfiguration aber nicht eindeutig";
    eventLog.add("WARN", "METER_AUTO_AMBIGUOUS", meterCommissioning.message);
    return;
  }

  saveConfig(); // exactly one persistent write after verified detection
  restoreConfiguredMeterSerial();

  meterCommissioning.message =
      "Zaehler automatisch erkannt: " +
      String(meterProtocolName(config.meterProtocol)) + ", " +
      String(config.baud) + " Baud";
  eventLog.add("INFO", "METER_AUTO_FOUND", meterCommissioning.message);
}

void startMeterCommissioning() {
#if IR_TRACKER_ENABLE_FACTORY_TEST
  if (factoryTest.running) return;
#endif
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  if (config.bridgeEnabled) return;
#endif
  if (meterCommissioning.started || productSafeRecoveryActive() ||
      config.meterProtocol != MeterProtocol::Auto || gpioScan.active ||
      irPulse.active || apatorUnlock.active || activeD0.active)
    return;

  meterCommissioning = MeterCommissioningState{};
  meterCommissioning.started = true;
  meterCommissioning.active = true;
  beginMeterCommissioningCandidate();
  eventLog.add("INFO", "METER_AUTO_START",
               "Automatische Zaehlererkennung gestartet");
}

void manageMeterCommissioning() {
  if (!meterCommissioning.started) {
    // Existing Auto parser always gets first chance.
    if (meter.lastTelegramMs &&
        millis() - meter.lastTelegramMs < kReadingStaleMs)
      return;
    if (millis() >= kAutoSerialStartMs) startMeterCommissioning();
    return;
  }

  if (!meterCommissioning.active) return;

  const uint32_t accepted =
      meter.telegrams - meterCommissioning.baselineTelegrams;

  if (accepted >= kAutoSerialConfirmFrames &&
      meter.lastTelegramMs >= meterCommissioning.candidateStartedMs &&
      meter.lastCrcValid) {
    finishMeterCommissioning(true);
    return;
  }

  if (millis() - meterCommissioning.candidateStartedMs <
      kAutoSerialWindowMs)
    return;

  ++meterCommissioning.candidate;
  if (meterCommissioning.candidate >=
      sizeof(kAutoSerialCandidates) / sizeof(kAutoSerialCandidates[0])) {
    finishMeterCommissioning(false);
    return;
  }

  beginMeterCommissioningCandidate();
}

String meterCommissioningJson() {
  String json;
  json.reserve(320);
  json = "{\"started\":";
  json += meterCommissioning.started ? "true" : "false";
  json += ",\"active\":";
  json += meterCommissioning.active ? "true" : "false";
  json += ",\"finished\":";
  json += meterCommissioning.finished ? "true" : "false";
  json += ",\"found\":";
  json += meterCommissioning.found ? "true" : "false";
  json += ",\"suggest_gpio_scan\":";
  json += meterCommissioning.suggestGpioScan ? "true" : "false";
  json += ",\"protocol\":\"" +
          String(meterProtocolName(meterCommissioning.foundProtocol)) + "\"";
  json += ",\"baud\":" + String(meterCommissioning.foundBaud);
  json += ",\"message\":\"" + jsonEscape(meterCommissioning.message) + "\"}";
  return json;
}
