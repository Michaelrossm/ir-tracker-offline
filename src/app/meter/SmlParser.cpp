#include "SmlParser.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

constexpr uint8_t kSmlStart[] = {
    0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01};
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
  if ((type != 0x50 && type != 0x60) || len < 2 || pos + len > data.size())
    return {};
  uint64_t raw = 0;
  for (size_t i = pos + 1; i < pos + len; ++i) raw = (raw << 8) | data[i];
  int64_t signedValue = static_cast<int64_t>(raw);
  if (type == 0x50) {
    const uint8_t bits = (len - 1) * 8;
    if (bits < 64 && (raw & (uint64_t(1) << (bits - 1))))
      signedValue = static_cast<int64_t>(raw | (~uint64_t(0) << bits));
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
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : crc >> 1;
  }
  return crc ^ 0xffff;
}

bool matchAt(const std::vector<uint8_t> &data, size_t pos,
             const uint8_t *needle, size_t length) {
  return pos + length <= data.size() &&
         memcmp(data.data() + pos, needle, length) == 0;
}

}  // namespace

bool SmlParser::extractObis(const std::vector<uint8_t> &data,
                            const uint8_t obis[6], double &target) {
  for (size_t i = 0; i + 6 < data.size(); ++i) {
    if (memcmp(data.data() + i, obis, 6) != 0) continue;
    std::vector<SmlNumber> numbers;
    const size_t limit = std::min(data.size(), i + 52);
    for (size_t p = i + 6; p < limit;) {
      if (p > i + 8 && data[p] == 0x77) break;
      const SmlNumber number = readSmlNumber(data, p);
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

void SmlParser::reset() {
  frame_.clear();
  startMatched_ = 0;
  capturing_ = false;
  trailerRemaining_ = 0;
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
  result.integrityValid = smlCrc16(frame_.data(), crcLowIndex) == expected;
  // DE: Defekte Frames duerfen Livewerte/Historie nie aendern.
  // EN: Damaged frames must never alter live values/history.
  if (!result.integrityValid) return MeterParseStatus::IntegrityError;

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

  MeterData &candidate = result.values;
  bool found = extractObis(frame_, powerObis, candidate.powerW);
  if (!std::isfinite(candidate.powerW)) {
    double importPowerW = NAN;
    double exportPowerW = NAN;
    const bool hasImportPower = extractObis(frame_, importPowerObis, importPowerW);
    const bool hasExportPower = extractObis(frame_, exportPowerObis, exportPowerW);
    if (hasImportPower || hasExportPower) {
      candidate.powerW = (hasImportPower ? importPowerW : 0.0) -
                         (hasExportPower ? exportPowerW : 0.0);
      found = true;
    }
  }
  double importWh = NAN;
  double exportWh = NAN;
  if (extractObis(frame_, importObis, importWh)) {
    candidate.importKwh = importWh / 1000.0;
    found = true;
  }
  if (extractObis(frame_, exportObis, exportWh)) {
    candidate.exportKwh = exportWh / 1000.0;
    found = true;
  }
  auto extractTariffTotal = [&](const uint8_t codes[2][6], double &target) {
    if (std::isfinite(target)) return false;
    double tariffs[2] = {NAN, NAN};
    const bool first = extractObis(frame_, codes[0], tariffs[0]);
    const bool second = extractObis(frame_, codes[1], tariffs[1]);
    if (!first && !second) return false;
    target = ((first ? tariffs[0] : 0.0) + (second ? tariffs[1] : 0.0)) /
             1000.0;
    return true;
  };
  found |= extractTariffTotal(importTariffObis, candidate.importKwh);
  found |= extractTariffTotal(exportTariffObis, candidate.exportKwh);
  for (uint8_t phase = 0; phase < 3; ++phase) {
    const bool netPhasePower =
        extractObis(frame_, phasePowerObis[phase], candidate.phasePowerW[phase]);
    found |= netPhasePower;
    if (!netPhasePower) {
      double phaseImportW = NAN;
      double phaseExportW = NAN;
      const bool hasImport =
          extractObis(frame_, phaseImportPowerObis[phase], phaseImportW);
      const bool hasExport =
          extractObis(frame_, phaseExportPowerObis[phase], phaseExportW);
      if (hasImport || hasExport) {
        candidate.phasePowerW[phase] =
            (hasImport ? phaseImportW : 0.0) -
            (hasExport ? phaseExportW : 0.0);
        found = true;
      }
    }
    found |= extractObis(frame_, phaseVoltageObis[phase],
                         candidate.phaseVoltageV[phase]);
    found |= extractObis(frame_, phaseCurrentObis[phase],
                         candidate.phaseCurrentA[phase]);
  }
  return found ? MeterParseStatus::Valid : MeterParseStatus::InvalidFrame;
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
              sizeof(kSmlEnd)))
    trailerRemaining_ = 3;
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
