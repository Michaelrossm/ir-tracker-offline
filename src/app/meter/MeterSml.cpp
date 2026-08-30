// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

bool matchAt(const std::vector<uint8_t> &data, size_t pos, const uint8_t *needle, size_t length) {
  return pos + length <= data.size() && memcmp(data.data() + pos, needle, length) == 0;
}

struct SmlNumber {
  bool valid = false;
  double value = 0;
  size_t next = 0;
};

SmlNumber readSmlNumber(const std::vector<uint8_t> &data, size_t pos) {
  if (pos >= data.size()) return {};
  const uint8_t tl = data[pos];
  const uint8_t type = tl & 0x70;
  const uint8_t len = tl & 0x0f;
  if ((type != 0x50 && type != 0x60) || len < 2 || pos + len > data.size()) return {};
  uint64_t raw = 0;
  for (size_t i = pos + 1; i < pos + len; ++i) raw = (raw << 8) | data[i];
  int64_t signedValue = static_cast<int64_t>(raw);
  if (type == 0x50) {
    const uint8_t bits = (len - 1) * 8;
    if (bits < 64 && (raw & (uint64_t(1) << (bits - 1)))) {
      signedValue =
          static_cast<int64_t>(raw | (~uint64_t(0) << bits));
    }
  }
  SmlNumber result;
  result.valid = true;
  result.value = type == 0x50 ? static_cast<double>(signedValue) : static_cast<double>(raw);
  result.next = pos + len;
  return result;
}

bool extractObisFrom(const std::vector<uint8_t> &data, const uint8_t obis[6], double &target) {
  for (size_t i = 0; i + 6 < data.size(); ++i) {
    if (memcmp(data.data() + i, obis, 6) != 0) continue;
    std::vector<SmlNumber> numbers;
    const size_t limit = std::min(data.size(), i + 52);
    for (size_t p = i + 6; p < limit;) {
      if (p > i + 8 && data[p] == 0x77) break;
      SmlNumber number = readSmlNumber(data, p);
      if (number.valid) {
        numbers.push_back(number);
        p = number.next;
      } else {
        ++p;
      }
    }
    if (numbers.empty()) continue;
    double value = numbers.back().value;
    if (numbers.size() >= 2) {
      const int scaler = static_cast<int>(numbers[numbers.size() - 2].value);
      if (scaler >= -9 && scaler <= 9) value *= pow(10.0, scaler);
    }
    target = value;
    return true;
  }
  return false;
}

bool extractObis(const uint8_t obis[6], double &target) {
  return extractObisFrom(telegram, obis, target);
}

uint16_t smlCrc16(const uint8_t *data, size_t length) {
  uint16_t crc = 0xffff;
  while (length--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : crc >> 1;
    }
  }
  return crc ^ 0xffff;
}

bool commitMeterCandidate(MeterValues &candidate, MeterProtocol protocol,
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

void parseTelegram() {
  const uint8_t powerObis[] = {0x01, 0x00, 0x10, 0x07, 0x00, 0xff};
  const uint8_t importPowerObis[] = {0x01, 0x00, 0x01, 0x07, 0x00, 0xff};
  const uint8_t exportPowerObis[] = {0x01, 0x00, 0x02, 0x07, 0x00, 0xff};
  const uint8_t importObis[] = {0x01, 0x00, 0x01, 0x08, 0x00, 0xff};
  const uint8_t exportObis[] = {0x01, 0x00, 0x02, 0x08, 0x00, 0xff};
  const uint8_t importTariffObis[2][6] = {
      {0x01, 0x00, 0x01, 0x08, 0x01, 0xff},
      {0x01, 0x00, 0x01, 0x08, 0x02, 0xff}};
  const uint8_t exportTariffObis[2][6] = {
      {0x01, 0x00, 0x02, 0x08, 0x01, 0xff},
      {0x01, 0x00, 0x02, 0x08, 0x02, 0xff}};
  const uint8_t phasePowerObis[3][6] = {
      {0x01, 0x00, 0x24, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x38, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x4c, 0x07, 0x00, 0xff}};
  const uint8_t phaseImportPowerObis[3][6] = {
      {0x01, 0x00, 0x15, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x29, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x3d, 0x07, 0x00, 0xff}};
  const uint8_t phaseExportPowerObis[3][6] = {
      {0x01, 0x00, 0x16, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x2a, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x3e, 0x07, 0x00, 0xff}};
  const uint8_t phaseVoltageObis[3][6] = {
      {0x01, 0x00, 0x20, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x34, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x48, 0x07, 0x00, 0xff}};
  const uint8_t phaseCurrentObis[3][6] = {
      {0x01, 0x00, 0x1f, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x33, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x47, 0x07, 0x00, 0xff}};
  lastTelegram = telegram;
  meter.lastCrcValid = false;
  if (telegram.size() < 12) {
    ++meter.parseErrors;
    return;
  }
  const size_t crcLowIndex = telegram.size() - 2;
  const uint16_t expected =
      telegram[crcLowIndex] |
      (static_cast<uint16_t>(telegram[crcLowIndex + 1]) << 8);
  const uint16_t actual = smlCrc16(telegram.data(), crcLowIndex);
  meter.lastCrcValid = expected == actual;
  if (!meter.lastCrcValid) {
    // DE: Defekte Frames dürfen Livewerte/Historie nie ändern. | EN: Damaged frames must never alter live values/history.
    ++meter.crcErrors;
    return;
  }

  MeterValues candidate;
  bool found = false;
  found |= extractObis(powerObis, candidate.powerW);
  if (!std::isfinite(candidate.powerW)) {
    double importPowerW = NAN;
    double exportPowerW = NAN;
    const bool hasImportPower = extractObis(importPowerObis, importPowerW);
    const bool hasExportPower = extractObis(exportPowerObis, exportPowerW);
    if (hasImportPower || hasExportPower) {
      candidate.powerW = (hasImportPower ? importPowerW : 0.0) -
                         (hasExportPower ? exportPowerW : 0.0);
      found = true;
    }
  }
  double importWh = NAN;
  double exportWh = NAN;
  if (extractObis(importObis, importWh)) {
    candidate.importKwh = importWh / 1000.0;
    found = true;
  }
  if (extractObis(exportObis, exportWh)) {
    candidate.exportKwh = exportWh / 1000.0;
    found = true;
  }
  auto extractTariffTotal = [&](const uint8_t codes[2][6], double &target) {
    if (std::isfinite(target)) return false;
    double tariffs[2] = {NAN, NAN};
    const bool first = extractObis(codes[0], tariffs[0]);
    const bool second = extractObis(codes[1], tariffs[1]);
    if (!first && !second) return false;
    target = ((first ? tariffs[0] : 0.0) +
              (second ? tariffs[1] : 0.0)) /
             1000.0;
    return true;
  };
  found |= extractTariffTotal(importTariffObis, candidate.importKwh);
  found |= extractTariffTotal(exportTariffObis, candidate.exportKwh);
  for (uint8_t phase = 0; phase < 3; ++phase) {
    const bool netPhasePower = extractObis(
        phasePowerObis[phase], candidate.phasePowerW[phase]);
    found |= netPhasePower;
    if (!netPhasePower) {
      double phaseImportW = NAN;
      double phaseExportW = NAN;
      const bool hasImport = extractObis(phaseImportPowerObis[phase], phaseImportW);
      const bool hasExport = extractObis(phaseExportPowerObis[phase], phaseExportW);
      if (hasImport || hasExport) {
        candidate.phasePowerW[phase] =
            (hasImport ? phaseImportW : 0.0) -
            (hasExport ? phaseExportW : 0.0);
        found = true;
      }
    }
    found |= extractObis(phaseVoltageObis[phase],
                         candidate.phaseVoltageV[phase]);
    found |= extractObis(phaseCurrentObis[phase],
                         candidate.phaseCurrentA[phase]);
  }
  if (!found) {
    ++meter.parseErrors;
    return;
  }

  // DE: Atomar erst nach CRC, Parsing und Plausibilität übernehmen. | EN: Commit atomically only after CRC, parsing and plausibility succeed.
  commitMeterCandidate(candidate, MeterProtocol::Sml, true, true);
}

void consumeMeterByte(uint8_t value) {
  ++meter.bytes;
  if (config.meterProtocol != MeterProtocol::Sml) {
    LegacyMeterReading legacy;
    if (legacyMeterParser.consume(value, legacy)) {
      MeterValues candidate;
      candidate.powerW = legacy.powerW;
      candidate.importKwh = legacy.importKwh;
      candidate.exportKwh = legacy.exportKwh;
      for (uint8_t phase = 0; phase < 3; ++phase) {
        candidate.phasePowerW[phase] = legacy.phasePowerW[phase];
        candidate.phaseVoltageV[phase] = legacy.phaseVoltageV[phase];
        candidate.phaseCurrentA[phase] = legacy.phaseCurrentA[phase];
      }
      lastTelegram.assign(legacyMeterParser.lastFrameData(),
                          legacyMeterParser.lastFrameData() +
                              legacyMeterParser.lastFrameSize());
      commitMeterCandidate(candidate, MeterProtocol::Iec62056,
                           legacy.bccPresent,
                           !legacy.bccPresent || legacy.bccValid);
    }
    const uint32_t checksumErrors = legacyMeterParser.checksumErrors();
    if (checksumErrors > legacyChecksumErrorsSeen) {
      meter.crcErrors += checksumErrors - legacyChecksumErrorsSeen;
      legacyChecksumErrorsSeen = checksumErrors;
    }
  }
  if (config.meterProtocol == MeterProtocol::Iec62056 ||
      config.meterProtocol == MeterProtocol::Iec62056Active)
    return;
  if (!capturing) {
    if (value == kSmlStart[startMatched]) {
      ++startMatched;
      if (startMatched == sizeof(kSmlStart)) {
        telegram.assign(kSmlStart, kSmlStart + sizeof(kSmlStart));
        capturing = true;
        startMatched = 0;
      }
    } else {
      startMatched = value == kSmlStart[0] ? 1 : 0;
    }
    return;
  }
  telegram.push_back(value);
  if (smlTrailerRemaining) {
    if (--smlTrailerRemaining == 0) {
      parseTelegram();
      telegram.clear();
      capturing = false;
    }
    return;
  }
  if (telegram.size() > kTelegramMax) {
    telegram.clear();
    capturing = false;
    ++meter.parseErrors;
    return;
  }
  if (telegram.size() >= sizeof(kSmlEnd) &&
      matchAt(telegram, telegram.size() - sizeof(kSmlEnd), kSmlEnd, sizeof(kSmlEnd))) {
    // DE: SML-Trailer nach 1A: Füllbytes und CRC16 Little Endian. | EN: SML trailer after 1A: fill-byte count and CRC16 little-endian.
    smlTrailerRemaining = 3;
  }
}

void resetSmlCapture() {
  telegram.clear();
  startMatched = 0;
  capturing = false;
  smlTrailerRemaining = 0;
  legacyMeterParser.reset();
  legacyMeterParser.clearIdentificationReady();
}

void restoreConfiguredMeterSerial() {
  meterSerial.end();
  resetSmlCapture();
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
  resetSmlCapture();
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
    if (!activeD0.acknowledgementSent &&
        legacyMeterParser.identificationReady()) {
      // Original firmware uses IEC 62056-21 ACK 000 and remains at 300 baud.
      static const uint8_t acknowledgement[] = {0x06, '0', '0', '0', '\r', '\n'};
      meterSerial.write(acknowledgement, sizeof(acknowledgement));
      meterSerial.flush();
      legacyMeterParser.clearIdentificationReady();
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
  const bool automaticDiscovery = config.meterProtocol == MeterProtocol::Auto &&
                                  noUsableTelegram &&
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
