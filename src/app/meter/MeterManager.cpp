// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve shared
// application state while protocol parsers remain normal compiled modules.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

bool commitMeterCandidate(MeterData &candidate, MeterProtocol protocol,
                          bool integrityPresent, bool integrityValid) {
  if (!std::isfinite(candidate.powerW) &&
      std::isfinite(candidate.phasePowerW[0]) &&
      std::isfinite(candidate.phasePowerW[1]) &&
      std::isfinite(candidate.phasePowerW[2])) {
    candidate.powerW = candidate.phasePowerW[0] +
                       candidate.phasePowerW[1] +
                       candidate.phasePowerW[2];
  }
  const auto plausible = [](double value, double minimum, double maximum) {
    return !std::isfinite(value) || (value >= minimum && value <= maximum);
  };
  bool found = std::isfinite(candidate.powerW) ||
               std::isfinite(candidate.importKwh) ||
               std::isfinite(candidate.exportKwh);
  bool plausibleValues =
      plausible(candidate.powerW, -100000.0, 100000.0) &&
      plausible(candidate.importKwh, 0.0, 1.0e9) &&
      plausible(candidate.exportKwh, 0.0, 1.0e9);
  for (uint8_t phase = 0; phase < 3; ++phase) {
    found |= std::isfinite(candidate.phasePowerW[phase]) ||
             std::isfinite(candidate.phaseVoltageV[phase]) ||
             std::isfinite(candidate.phaseCurrentA[phase]);
    plausibleValues &=
        plausible(candidate.phasePowerW[phase], -100000.0, 100000.0) &&
        plausible(candidate.phaseVoltageV[phase], 0.0, 500.0) &&
        plausible(candidate.phaseCurrentA[phase], 0.0, 200.0);
  }
  if (!found || !plausibleValues || !integrityValid) {
    ++meter.parseErrors;
    return false;
  }

  // DE: Erst nach Integritaet, Parsing und Plausibilitaet atomar uebernehmen.
  // EN: Commit atomically only after CRC, parsing and plausibility succeed.
  const bool firstValidTelegram = meter.lastTelegramMs == 0;
  const uint32_t now = millis();
  auto commit = [&](double value, double &target, uint32_t &updatedMs) {
    if (!std::isfinite(value)) return;
    target = value;
    updatedMs = now;
  };
  commit(candidate.powerW, meter.powerW, meter.powerUpdatedMs);
  commit(candidate.importKwh, meter.importKwh, meter.importUpdatedMs);
  commit(candidate.exportKwh, meter.exportKwh, meter.exportUpdatedMs);
  for (uint8_t phase = 0; phase < 3; ++phase) {
    commit(candidate.phasePowerW[phase], meter.phasePowerW[phase],
           meter.phasePowerUpdatedMs[phase]);
    commit(candidate.phaseVoltageV[phase], meter.phaseVoltageV[phase],
           meter.phaseVoltageUpdatedMs[phase]);
    commit(candidate.phaseCurrentA[phase], meter.phaseCurrentA[phase],
           meter.phaseCurrentUpdatedMs[phase]);
  }
  ++meter.telegrams;
  meter.lastCrcValid = integrityValid;
  meter.lastIntegrityPresent = integrityPresent;
  meter.detectedProtocol = protocol;
  meter.lastTelegramMs = now;
  if (firstValidTelegram) {
    eventLog.add("INFO", "METER_FIRST",
                 "Erstes gueltiges Zaehlertelegramm empfangen (" +
                     String(meterProtocolName(protocol)) + ")");
  }
  return true;
}

constexpr bool isD0Protocol(MeterProtocol protocol) {
  return protocol == MeterProtocol::Iec62056 ||
         protocol == MeterProtocol::Iec62056Active;
}
static_assert(!isD0Protocol(MeterProtocol::Sml),
              "SML integrity events must use the SML CRC counter");
static_assert(isD0Protocol(MeterProtocol::Iec62056) &&
                  isD0Protocol(MeterProtocol::Iec62056Active),
              "IEC/D0 integrity events must use the D0 BCC counter");

bool parserIntegrityIsActive(MeterProtocol protocol) {
  if (config.meterProtocol == MeterProtocol::Sml)
    return protocol == MeterProtocol::Sml;
  if (isD0Protocol(config.meterProtocol)) return isD0Protocol(protocol);
  const bool activeFresh =
      meter.detectedProtocol != MeterProtocol::Auto && meter.lastTelegramMs &&
      millis() - meter.lastTelegramMs < kReadingStaleMs;
  if (!activeFresh) return true;
  return meter.detectedProtocol == MeterProtocol::Sml
             ? protocol == MeterProtocol::Sml
             : isD0Protocol(protocol);
}

uint32_t d0BccErrors() { return d0Parser.checksumErrors(); }

bool autoProtocolFresh(MeterProtocol protocol) {
  return config.meterProtocol == MeterProtocol::Auto &&
         meter.detectedProtocol == protocol && meter.lastTelegramMs &&
         millis() - meter.lastTelegramMs < kReadingStaleMs;
}

constexpr bool shouldFeedD0ParserFor(MeterProtocol configured,
                                     MeterProtocol detected,
                                     bool detectedFresh) {
  return configured == MeterProtocol::Sml
             ? false
             : (isD0Protocol(configured)
                    ? true
                    : !(detectedFresh && detected == MeterProtocol::Sml));
}

constexpr bool shouldFeedSmlParserFor(MeterProtocol configured,
                                      MeterProtocol detected,
                                      bool detectedFresh) {
  return isD0Protocol(configured)
             ? false
             : (configured == MeterProtocol::Sml
                    ? true
                    : !(detectedFresh &&
                        detected == MeterProtocol::Iec62056));
}

static_assert(!shouldFeedD0ParserFor(MeterProtocol::Auto, MeterProtocol::Sml,
                                    true) &&
                  shouldFeedSmlParserFor(MeterProtocol::Auto,
                                         MeterProtocol::Sml, true),
              "fresh Auto/SML must pause only D0");
static_assert(shouldFeedD0ParserFor(MeterProtocol::Auto,
                                   MeterProtocol::Iec62056, true) &&
                  !shouldFeedSmlParserFor(MeterProtocol::Auto,
                                          MeterProtocol::Iec62056, true),
              "fresh Auto/D0 must pause only SML");
static_assert(shouldFeedD0ParserFor(MeterProtocol::Auto, MeterProtocol::Sml,
                                   false) &&
                  shouldFeedSmlParserFor(MeterProtocol::Auto,
                                         MeterProtocol::Sml, false),
              "stale Auto lock must release both parsers");

bool shouldFeedD0Parser() {
  // In Auto mode, binary SML bytes must no longer create false D0/BCC
  // candidates after SML has been identified. Staleness releases this soft
  // lock automatically, allowing both protocol families to be discovered.
  return shouldFeedD0ParserFor(
      config.meterProtocol, meter.detectedProtocol,
      autoProtocolFresh(meter.detectedProtocol));
}

bool shouldFeedSmlParser() {
  // Passive D0 locks out SML only while the last valid D0 telegram is fresh.
  // Active-D0 retry/ACK handling remains owned by updateActiveD0().
  return shouldFeedSmlParserFor(
      config.meterProtocol, meter.detectedProtocol,
      autoProtocolFresh(meter.detectedProtocol));
}

void acceptParserEvent(MeterParseStatus status,
                       const MeterParseResult &result) {
  if (status == MeterParseStatus::None) return;
  if (status == MeterParseStatus::IntegrityError) {
    ++meter.crcErrors;
    if (result.protocol == MeterProtocol::Sml) ++meter.smlCrcErrors;
    if (parserIntegrityIsActive(result.protocol)) meter.lastCrcValid = false;
    return;
  }
  if (status == MeterParseStatus::InvalidFrame) {
    ++meter.parseErrors;
    return;
  }
  if (result.frameData && result.frameSize)
    lastTelegram.assign(result.frameData,
                        result.frameData + result.frameSize);
  MeterData candidate = result.values;
  commitMeterCandidate(candidate, result.protocol, result.integrityPresent,
                       result.integrityValid);
}

void consumeMeterBytes(const uint8_t *data, size_t length) {
  // Initialize the complete result model once per UART batch, not once per
  // byte. This keeps parser dispatch inexpensive in 80 MHz Eco mode.
  MeterParseResult result;
  for (size_t i = 0; i < length; ++i) {
    const uint8_t value = data[i];
    ++meter.bytes;
    if (shouldFeedD0Parser()) {
      const MeterParseStatus status = d0Parser.feed(&value, 1, result);
      acceptParserEvent(status, result);
    }
    if (shouldFeedSmlParser()) {
      const MeterParseStatus status = smlParser.feed(&value, 1, result);
      acceptParserEvent(status, result);
    }
  }
}

// Keep the meter's UART ahead of synchronous network work. This is the single
// shared receive path used before and after network/Web processing; parsing and
// validation remain in consumeMeterBytes().
void serviceMeterInput() {
#if IR_TRACKER_ENABLE_FACTORY_TEST
  if (factoryTest.running) return;
#endif
  uint8_t incoming[128];
  size_t count = 0;
  while (!irPulse.active && meterSerial.available() &&
         count < sizeof(incoming)) {
    incoming[count++] = meterSerial.read();
  }
  if (!count) return;
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  if (config.snifferEnabled) snifferSocket.broadcastBIN(incoming, count);
#endif
  consumeMeterBytes(incoming, count);
}

void resetMeterParsers() {
  smlParser.reset();
  d0Parser.reset();
  d0Parser.clearIdentificationReady();
}

// Compatibility name used by the GPIO and TX diagnostic workflows.
void resetSmlCapture() { resetMeterParsers(); }

void restoreConfiguredMeterSerial() {
  meterSerial.end();
  resetMeterParsers();
  meterSerial.begin(meterBaud(), meterSerialMode(), config.rxPin,
                    config.txPin);
}

void finishActiveD0Attempt() {
  activeD0.active = false;
  activeD0.acknowledgementSent = false;
  if (config.meterProtocol == MeterProtocol::Auto)
    restoreConfiguredMeterSerial();
}

void beginActiveD0Attempt() {
  if (activeD0.active || config.txPin < 0 || irPulse.active ||
      gpioScan.active || apatorUnlock.active)
    return;
  meterSerial.end();
  resetMeterParsers();
  meterSerial.begin(300, SERIAL_7E1, config.rxPin, config.txPin);
  static const uint8_t request[] = {'/', '?', '!', '\r', '\n'};
  meterSerial.write(request, sizeof(request));
  meterSerial.flush();
  activeD0.active = true;
  activeD0.acknowledgementSent = false;
  activeD0.startedMs = millis();
  activeD0.lastAttemptMs = activeD0.startedMs;
}

void updateActiveD0() {
  if (activeD0.active) {
    if (!activeD0.acknowledgementSent && d0Parser.identificationReady()) {
      static const uint8_t acknowledgement[] =
          {0x06, '0', '0', '0', '\r', '\n'};
      meterSerial.write(acknowledgement, sizeof(acknowledgement));
      meterSerial.flush();
      d0Parser.clearIdentificationReady();
      activeD0.acknowledgementSent = true;
    }
    if ((meter.detectedProtocol == MeterProtocol::Iec62056 &&
         meter.lastTelegramMs >= activeD0.startedMs) ||
        millis() - activeD0.startedMs >= kActiveD0TimeoutMs)
      finishActiveD0Attempt();
    return;
  }
  const bool explicitActive =
      config.meterProtocol == MeterProtocol::Iec62056Active;
  const bool noUsableTelegram =
      !meter.lastTelegramMs ||
      millis() - meter.lastTelegramMs >= kMeterRecoveryMs;
  const bool automaticDiscovery =
      config.meterProtocol == MeterProtocol::Auto && noUsableTelegram &&
      millis() >= kActiveD0InitialDelayMs;
  if ((explicitActive || automaticDiscovery) &&
      (!activeD0.lastAttemptMs ||
       millis() - activeD0.lastAttemptMs >= kActiveD0RetryMs))
    beginActiveD0Attempt();
}

void updateMeterRecovery() {
  if (activeD0.active || gpioScan.active || irPulse.active ||
      apatorUnlock.active)
    return;
  const uint32_t now = millis();
  const uint32_t lastValid = meter.lastTelegramMs;
  if ((lastValid && now - lastValid < kMeterRecoveryMs) ||
      (!lastValid && now < kMeterRecoveryMs) ||
      (lastMeterRecoveryMs && now - lastMeterRecoveryMs < kMeterRecoveryMs))
    return;
  restoreConfiguredMeterSerial();
  lastMeterRecoveryMs = now;
  ++meterReinitializations;
  eventLog.add("WARN", "METER_UART_RECOVERY",
               "Zaehlerempfang nach Datenverlust neu initialisiert");
}
