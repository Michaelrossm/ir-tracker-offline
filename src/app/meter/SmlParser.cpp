#include "SmlParser.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
constexpr uint8_t kSmlStart[] = {0x1b, 0x1b, 0x1b, 0x1b,
                                 0x01, 0x01, 0x01, 0x01};
constexpr uint8_t kSmlEnd[] = {0x1b, 0x1b, 0x1b, 0x1b, 0x1a};

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
  if ((type != 0x50 && type != 0x60) || len < 2 || pos + len > data.size()) {
    return {};
  }

  uint64_t raw = 0;
  for (size_t i = pos + 1; i < pos + len; ++i) {
    raw = (raw << 8) | data[i];
  }
  int64_t signedValue = static_cast<int64_t>(raw);
  if (type == 0x50) {
    const uint8_t bits = (len - 1) * 8;
    if (bits < 64 && (raw & (uint64_t(1) << (bits - 1)))) {
      signedValue = static_cast<int64_t>(raw | (~uint64_t(0) << bits));
    }
  }

  SmlNumber result;
  result.valid = true;
  result.value = type == 0x50 ? static_cast<double>(signedValue)
                              : static_cast<double>(raw);
  result.next = pos + len;
  return result;
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

bool matchAt(const std::vector<uint8_t> &data, size_t pos,
             const uint8_t *needle, size_t length) {
  return pos + length <= data.size() &&
         memcmp(data.data() + pos, needle, length) == 0;
}

bool decodeLegacyWindow(const std::vector<uint8_t> &data, size_t obisPos,
                        double &target) {
  SmlNumber previous;
  SmlNumber last;
  uint8_t count = 0;
  const size_t limit = std::min(data.size(), obisPos + 52);
  for (size_t pos = obisPos + 6; pos < limit;) {
    if (pos > obisPos + 8 && data[pos] == 0x77) break;
    const SmlNumber number = readSmlNumber(data, pos);
    if (number.valid) {
      previous = last;
      last = number;
      if (count < 2) ++count;
      pos = number.next;
    } else {
      ++pos;
    }
  }
  if (!count || !last.valid) return false;

  double value = last.value;
  if (count >= 2 && previous.valid) {
    const int scaler = static_cast<int>(previous.value);
    if (scaler >= -9 && scaler <= 9) value *= pow(10.0, scaler);
  }
  target = value;
  return true;
}

enum ObisSlot : uint8_t {
  kPower, kImportPower, kExportPower, kImportEnergy, kExportEnergy,
  kImportTariff1, kImportTariff2, kExportTariff1, kExportTariff2,
  kP1, kP2, kP3, kP1Imp, kP2Imp, kP3Imp, kP1Exp, kP2Exp, kP3Exp,
  kV1, kV2, kV3, kI1, kI2, kI3, kSlotCount
};

struct ObisSpec {
  uint8_t code[6];
  ObisSlot slot;
};

constexpr ObisSpec kSpecs[] = {
    {{0x01, 0x00, 0x10, 0x07, 0x00, 0xff}, kPower},
    {{0x01, 0x00, 0x01, 0x07, 0x00, 0xff}, kImportPower},
    {{0x01, 0x00, 0x02, 0x07, 0x00, 0xff}, kExportPower},
    {{0x01, 0x00, 0x01, 0x08, 0x00, 0xff}, kImportEnergy},
    {{0x01, 0x00, 0x02, 0x08, 0x00, 0xff}, kExportEnergy},
    {{0x01, 0x00, 0x01, 0x08, 0x01, 0xff}, kImportTariff1},
    {{0x01, 0x00, 0x01, 0x08, 0x02, 0xff}, kImportTariff2},
    {{0x01, 0x00, 0x02, 0x08, 0x01, 0xff}, kExportTariff1},
    {{0x01, 0x00, 0x02, 0x08, 0x02, 0xff}, kExportTariff2},
    {{0x01, 0x00, 0x24, 0x07, 0x00, 0xff}, kP1},
    {{0x01, 0x00, 0x38, 0x07, 0x00, 0xff}, kP2},
    {{0x01, 0x00, 0x4c, 0x07, 0x00, 0xff}, kP3},
    {{0x01, 0x00, 0x15, 0x07, 0x00, 0xff}, kP1Imp},
    {{0x01, 0x00, 0x29, 0x07, 0x00, 0xff}, kP2Imp},
    {{0x01, 0x00, 0x3d, 0x07, 0x00, 0xff}, kP3Imp},
    {{0x01, 0x00, 0x16, 0x07, 0x00, 0xff}, kP1Exp},
    {{0x01, 0x00, 0x2a, 0x07, 0x00, 0xff}, kP2Exp},
    {{0x01, 0x00, 0x3e, 0x07, 0x00, 0xff}, kP3Exp},
    {{0x01, 0x00, 0x20, 0x07, 0x00, 0xff}, kV1},
    {{0x01, 0x00, 0x34, 0x07, 0x00, 0xff}, kV2},
    {{0x01, 0x00, 0x48, 0x07, 0x00, 0xff}, kV3},
    {{0x01, 0x00, 0x1f, 0x07, 0x00, 0xff}, kI1},
    {{0x01, 0x00, 0x33, 0x07, 0x00, 0xff}, kI2},
    {{0x01, 0x00, 0x47, 0x07, 0x00, 0xff}, kI3},
};
static_assert(sizeof(kSpecs) / sizeof(kSpecs[0]) == kSlotCount,
              "OBIS table mismatch");

struct Captured {
  bool found = false;
  double value = NAN;
};

void captureOnePass(const std::vector<uint8_t> &data,
                    Captured output[kSlotCount]) {
  if (data.size() < 6) return;
  uint8_t unresolved = kSlotCount;
  for (size_t pos = 0; pos + 6 <= data.size() && unresolved; ++pos) {
    for (const auto &spec : kSpecs) {
      Captured &slot = output[spec.slot];
      if (slot.found) continue;
      if (memcmp(data.data() + pos, spec.code, 6) != 0) continue;
      double value = NAN;
      if (decodeLegacyWindow(data, pos, value)) {
        slot.found = true;
        slot.value = value;
        --unresolved;
      }
    }
  }
}

inline bool has(const Captured captured[kSlotCount], ObisSlot slot) {
  return captured[slot].found;
}
inline double valueOf(const Captured captured[kSlotCount], ObisSlot slot) {
  return captured[slot].value;
}

bool applyCaptured(const Captured captured[kSlotCount], MeterData &meter) {
  bool found = false;
  if (has(captured, kPower)) {
    meter.powerW = valueOf(captured, kPower);
    found = true;
  } else {
    const bool importFound = has(captured, kImportPower);
    const bool exportFound = has(captured, kExportPower);
    if (importFound || exportFound) {
      meter.powerW = (importFound ? valueOf(captured, kImportPower) : 0.0) -
                     (exportFound ? valueOf(captured, kExportPower) : 0.0);
      found = true;
    }
  }
  if (has(captured, kImportEnergy)) {
    meter.importKwh = valueOf(captured, kImportEnergy) / 1000.0;
    found = true;
  } else {
    const bool first = has(captured, kImportTariff1);
    const bool second = has(captured, kImportTariff2);
    if (first || second) {
      meter.importKwh = ((first ? valueOf(captured, kImportTariff1) : 0.0) +
                         (second ? valueOf(captured, kImportTariff2) : 0.0)) /
                        1000.0;
      found = true;
    }
  }
  if (has(captured, kExportEnergy)) {
    meter.exportKwh = valueOf(captured, kExportEnergy) / 1000.0;
    found = true;
  } else {
    const bool first = has(captured, kExportTariff1);
    const bool second = has(captured, kExportTariff2);
    if (first || second) {
      meter.exportKwh = ((first ? valueOf(captured, kExportTariff1) : 0.0) +
                         (second ? valueOf(captured, kExportTariff2) : 0.0)) /
                        1000.0;
      found = true;
    }
  }

  constexpr ObisSlot net[3] = {kP1, kP2, kP3};
  constexpr ObisSlot import[3] = {kP1Imp, kP2Imp, kP3Imp};
  constexpr ObisSlot exportSlots[3] = {kP1Exp, kP2Exp, kP3Exp};
  constexpr ObisSlot voltage[3] = {kV1, kV2, kV3};
  constexpr ObisSlot current[3] = {kI1, kI2, kI3};
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (has(captured, net[phase])) {
      meter.phasePowerW[phase] = valueOf(captured, net[phase]);
      found = true;
    } else {
      const bool importFound = has(captured, import[phase]);
      const bool exportFound = has(captured, exportSlots[phase]);
      if (importFound || exportFound) {
        meter.phasePowerW[phase] =
            (importFound ? valueOf(captured, import[phase]) : 0.0) -
            (exportFound ? valueOf(captured, exportSlots[phase]) : 0.0);
        found = true;
      }
    }
    if (has(captured, voltage[phase])) {
      meter.phaseVoltageV[phase] = valueOf(captured, voltage[phase]);
      found = true;
    }
    if (has(captured, current[phase])) {
      meter.phaseCurrentA[phase] = valueOf(captured, current[phase]);
      found = true;
    }
  }
  return found;
}

bool parseOnePass(const std::vector<uint8_t> &data, MeterData &meter) {
  Captured captured[kSlotCount] = {};
  captureOnePass(data, captured);
  return applyCaptured(captured, meter);
}

bool parseLegacy(const std::vector<uint8_t> &data, MeterData &meter) {
  const uint8_t power[] = {1, 0, 0x10, 7, 0, 0xff};
  const uint8_t importPowerObis[] = {1, 0, 1, 7, 0, 0xff};
  const uint8_t exportPowerObis[] = {1, 0, 2, 7, 0, 0xff};
  const uint8_t importEnergy[] = {1, 0, 1, 8, 0, 0xff};
  const uint8_t exportEnergy[] = {1, 0, 2, 8, 0, 0xff};
  const uint8_t importTariffObis[2][6] = {{1, 0, 1, 8, 1, 0xff},
                                         {1, 0, 1, 8, 2, 0xff}};
  const uint8_t exportTariffObis[2][6] = {{1, 0, 2, 8, 1, 0xff},
                                         {1, 0, 2, 8, 2, 0xff}};
  const uint8_t phasePower[3][6] = {
      {1, 0, 0x24, 7, 0, 0xff}, {1, 0, 0x38, 7, 0, 0xff},
      {1, 0, 0x4c, 7, 0, 0xff}};
  const uint8_t phaseImport[3][6] = {
      {1, 0, 0x15, 7, 0, 0xff}, {1, 0, 0x29, 7, 0, 0xff},
      {1, 0, 0x3d, 7, 0, 0xff}};
  const uint8_t phaseExport[3][6] = {
      {1, 0, 0x16, 7, 0, 0xff}, {1, 0, 0x2a, 7, 0, 0xff},
      {1, 0, 0x3e, 7, 0, 0xff}};
  const uint8_t phaseVoltage[3][6] = {
      {1, 0, 0x20, 7, 0, 0xff}, {1, 0, 0x34, 7, 0, 0xff},
      {1, 0, 0x48, 7, 0, 0xff}};
  const uint8_t phaseCurrent[3][6] = {
      {1, 0, 0x1f, 7, 0, 0xff}, {1, 0, 0x33, 7, 0, 0xff},
      {1, 0, 0x47, 7, 0, 0xff}};

  bool found = SmlParser::extractObis(data, power, meter.powerW);
  if (!std::isfinite(meter.powerW)) {
    double importW = NAN;
    double exportW = NAN;
    const bool hasImport =
        SmlParser::extractObis(data, importPowerObis, importW);
    const bool hasExport =
        SmlParser::extractObis(data, exportPowerObis, exportW);
    if (hasImport || hasExport) {
      meter.powerW = (hasImport ? importW : 0.0) -
                     (hasExport ? exportW : 0.0);
      found = true;
    }
  }
  double importWh = NAN;
  double exportWh = NAN;
  if (SmlParser::extractObis(data, importEnergy, importWh)) {
    meter.importKwh = importWh / 1000.0;
    found = true;
  }
  if (SmlParser::extractObis(data, exportEnergy, exportWh)) {
    meter.exportKwh = exportWh / 1000.0;
    found = true;
  }
  const auto useTariffTotal = [&](const uint8_t codes[2][6], double &target) {
    if (std::isfinite(target)) return false;
    double values[2] = {NAN, NAN};
    const bool first = SmlParser::extractObis(data, codes[0], values[0]);
    const bool second = SmlParser::extractObis(data, codes[1], values[1]);
    if (!first && !second) return false;
    target = ((first ? values[0] : 0.0) + (second ? values[1] : 0.0)) /
             1000.0;
    return true;
  };
  found |= useTariffTotal(importTariffObis, meter.importKwh);
  found |= useTariffTotal(exportTariffObis, meter.exportKwh);
  for (uint8_t phase = 0; phase < 3; ++phase) {
    const bool net = SmlParser::extractObis(
        data, phasePower[phase], meter.phasePowerW[phase]);
    found |= net;
    if (!net) {
      double importW = NAN;
      double exportW = NAN;
      const bool hasImport =
          SmlParser::extractObis(data, phaseImport[phase], importW);
      const bool hasExport =
          SmlParser::extractObis(data, phaseExport[phase], exportW);
      if (hasImport || hasExport) {
        meter.phasePowerW[phase] = (hasImport ? importW : 0.0) -
                                   (hasExport ? exportW : 0.0);
        found = true;
      }
    }
    found |= SmlParser::extractObis(
        data, phaseVoltage[phase], meter.phaseVoltageV[phase]);
    found |= SmlParser::extractObis(
        data, phaseCurrent[phase], meter.phaseCurrentA[phase]);
  }
  return found;
}

bool sameDouble(double first, double second) {
  const bool firstFinite = std::isfinite(first);
  const bool secondFinite = std::isfinite(second);
  if (!firstFinite || !secondFinite) return firstFinite == secondFinite;
  const double scale =
      std::max(1.0, std::max(std::fabs(first), std::fabs(second)));
  return std::fabs(first - second) <= 1e-12 * scale;
}

bool sameMeterData(const MeterData &first, const MeterData &second) {
  if (!sameDouble(first.powerW, second.powerW) ||
      !sameDouble(first.importKwh, second.importKwh) ||
      !sameDouble(first.exportKwh, second.exportKwh)) {
    return false;
  }
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (!sameDouble(first.phasePowerW[phase], second.phasePowerW[phase]) ||
        !sameDouble(first.phaseVoltageV[phase], second.phaseVoltageV[phase]) ||
        !sameDouble(first.phaseCurrentA[phase], second.phaseCurrentA[phase])) {
      return false;
    }
  }
  return true;
}
}  // namespace

bool SmlParser::extractObis(const std::vector<uint8_t> &data,
                            const uint8_t obis[6], double &target) {
  for (size_t i = 0; i + 6 <= data.size(); ++i) {
    if (memcmp(data.data() + i, obis, 6) != 0) continue;
    if (decodeLegacyWindow(data, i, target)) return true;
  }
  return false;
}

void SmlParser::reset() {
  frame_.clear();
  startMatched_ = 0;
  capturing_ = false;
  trailerRemaining_ = 0;
  qualificationMatches_ = 0;
  sentinelCountdown_ = kSentinelInterval;
  comparisonMismatches_ = 0;
  sentinelComparisons_ = 0;
  onePassQualified_ = false;
  legacyFallbackLatched_ = false;
}

SmlParser::Diagnostics SmlParser::diagnostics() const {
  return {qualificationMatches_, kQualificationFrames, kSentinelInterval,
          comparisonMismatches_, sentinelComparisons_, onePassQualified_,
          onePassQualified_ && !legacyFallbackLatched_,
          legacyFallbackLatched_};
}

MeterParseStatus SmlParser::parseFrame(MeterParseResult &result) {
  result = {};
  result.protocol = MeterProtocol::Sml;
  result.integrityPresent = true;
  result.frameData = frame_.data();
  result.frameSize = frame_.size();
  if (frame_.size() < 12) return MeterParseStatus::InvalidFrame;

  const size_t crcLowIndex = frame_.size() - 2;
  const uint16_t expected =
      frame_[crcLowIndex] |
      (static_cast<uint16_t>(frame_[crcLowIndex + 1]) << 8);
  // Damaged frames must never alter live values/history. Commit atomically
  // only after CRC, parsing and plausibility have succeeded upstream.
  result.integrityValid =
      smlCrc16(frame_.data(), crcLowIndex) == expected;
  if (!result.integrityValid) return MeterParseStatus::IntegrityError;

  if (legacyFallbackLatched_) {
    MeterData legacy;
    const bool found = parseLegacy(frame_, legacy);
    result.values = legacy;
    return found ? MeterParseStatus::Valid : MeterParseStatus::InvalidFrame;
  }

  MeterData onePass;
  const bool onePassFound = parseOnePass(frame_, onePass);
  bool compareWithLegacy = !onePassQualified_;
  if (onePassQualified_) {
    if (sentinelCountdown_ > 0) --sentinelCountdown_;
    if (sentinelCountdown_ == 0) {
      compareWithLegacy = true;
      sentinelCountdown_ = kSentinelInterval;
      ++sentinelComparisons_;
    }
  }
  if (compareWithLegacy) {
    MeterData legacy;
    const bool legacyFound = parseLegacy(frame_, legacy);
    if (legacyFound != onePassFound || !sameMeterData(legacy, onePass)) {
      ++comparisonMismatches_;
      legacyFallbackLatched_ = true;
      result.values = legacy;
      return legacyFound ? MeterParseStatus::Valid
                         : MeterParseStatus::InvalidFrame;
    }
    if (!onePassQualified_) {
      if (legacyFound) {
        if (qualificationMatches_ < kQualificationFrames) {
          ++qualificationMatches_;
        }
        if (qualificationMatches_ >= kQualificationFrames) {
          onePassQualified_ = true;
          sentinelCountdown_ = kSentinelInterval;
        }
      }
      result.values = legacy;
      return legacyFound ? MeterParseStatus::Valid
                         : MeterParseStatus::InvalidFrame;
    }
  }
  result.values = onePass;
  return onePassFound ? MeterParseStatus::Valid : MeterParseStatus::InvalidFrame;
}

MeterParseStatus SmlParser::consumeByte(uint8_t value,
                                        MeterParseResult &result) {
  if (!capturing_) {
    if (value == kSmlStart[startMatched_]) {
      ++startMatched_;
      if (startMatched_ == sizeof(kSmlStart)) {
        frame_.assign(kSmlStart, kSmlStart + sizeof(kSmlStart));
        capturing_ = true;
        startMatched_ = 0;
      }
    } else {
      startMatched_ = value == kSmlStart[0] ? 1 : 0;
    }
    return MeterParseStatus::None;
  }
  frame_.push_back(value);
  if (trailerRemaining_) {
    if (--trailerRemaining_ == 0) {
      const MeterParseStatus status = parseFrame(result);
      capturing_ = false;
      return status;
    }
    return MeterParseStatus::None;
  }
  if (frame_.size() > kMaximumFrame) {
    reset();
    return MeterParseStatus::InvalidFrame;
  }
  if (frame_.size() >= sizeof(kSmlEnd) &&
      matchAt(frame_, frame_.size() - sizeof(kSmlEnd), kSmlEnd,
              sizeof(kSmlEnd))) {
    trailerRemaining_ = 3;
  }
  return MeterParseStatus::None;
}

MeterParseStatus SmlParser::feed(const uint8_t *data, size_t length,
                                 MeterParseResult &result) {
  for (size_t i = 0; i < length; ++i) {
    const MeterParseStatus status = consumeByte(data[i], result);
    if (status != MeterParseStatus::None) return status;
  }
  return MeterParseStatus::None;
}
