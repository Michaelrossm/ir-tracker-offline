#include "D0Parser.h"

#include <algorithm>
#include <cstring>

namespace {

constexpr uint8_t kStx = 0x02;
constexpr uint8_t kEtx = 0x03;

bool obisEquals(const char *code, const char *wanted) {
  const char *value = strrchr(code, ':');
  value = value ? value + 1 : code;
  const size_t length = strlen(wanted);
  return strncmp(value, wanted, length) == 0 &&
         (value[length] == '\0' || value[length] == '*' ||
          value[length] == '(');
}

double convertedValue(double value, const char *unit, bool energy) {
  if (!unit) return value;
  if (energy) {
    if (strcmp(unit, "Wh") == 0) return value / 1000.0;
    if (strcmp(unit, "MWh") == 0) return value * 1000.0;
    return value;  // kWh or a meter-specific unit already configured as kWh.
  }
  if (strcmp(unit, "kW") == 0) return value * 1000.0;
  if (strcmp(unit, "MW") == 0) return value * 1000000.0;
  if (strcmp(unit, "mA") == 0) return value / 1000.0;
  return value;
}

bool parseDecimal(const char *text, double &value, const char *&end) {
  const char *cursor = text;
  bool negative = false;
  if (*cursor == '+' || *cursor == '-') {
    negative = *cursor == '-';
    ++cursor;
  }
  bool hasDigit = false;
  double result = 0.0;
  while (*cursor >= '0' && *cursor <= '9') {
    result = result * 10.0 + (*cursor++ - '0');
    hasDigit = true;
  }
  if (*cursor == '.' || *cursor == ',') {
    ++cursor;
    double place = 0.1;
    while (*cursor >= '0' && *cursor <= '9') {
      result += (*cursor++ - '0') * place;
      place *= 0.1;
      hasDigit = true;
    }
  }
  if (!hasDigit) return false;
  if (*cursor == 'e' || *cursor == 'E') {
    const char *exponentStart = cursor++;
    bool exponentNegative = false;
    if (*cursor == '+' || *cursor == '-') {
      exponentNegative = *cursor == '-';
      ++cursor;
    }
    int exponent = 0;
    bool hasExponent = false;
    while (*cursor >= '0' && *cursor <= '9') {
      exponent = std::min(exponent * 10 + (*cursor++ - '0'), 12);
      hasExponent = true;
    }
    if (!hasExponent) {
      cursor = exponentStart;
    } else {
      while (exponent-- > 0) result *= exponentNegative ? 0.1 : 10.0;
    }
  }
  value = negative ? -result : result;
  end = cursor;
  return std::isfinite(value);
}

bool parseObisLine(char *line, MeterData &reading,
                   double &importPowerW, double &exportPowerW,
                   double importTariffs[2], double exportTariffs[2],
                   double phaseImportPowerW[3],
                   double phaseExportPowerW[3]) {
  char *open = strchr(line, '(');
  if (!open) return false;
  char *close = strchr(open + 1, ')');
  if (!close) return false;
  *open = '\0';
  *close = '\0';
  const char *end = nullptr;
  double raw = NAN;
  if (!parseDecimal(open + 1, raw, end)) return false;
  const char *unit = *end == '*' ? end + 1 : nullptr;

  if (obisEquals(line, "1.8.0")) {
    reading.importKwh = convertedValue(raw, unit, true);
  } else if (obisEquals(line, "2.8.0")) {
    reading.exportKwh = convertedValue(raw, unit, true);
  } else if (obisEquals(line, "1.8.1")) {
    importTariffs[0] = convertedValue(raw, unit, true);
  } else if (obisEquals(line, "1.8.2")) {
    importTariffs[1] = convertedValue(raw, unit, true);
  } else if (obisEquals(line, "2.8.1")) {
    exportTariffs[0] = convertedValue(raw, unit, true);
  } else if (obisEquals(line, "2.8.2")) {
    exportTariffs[1] = convertedValue(raw, unit, true);
  } else if (obisEquals(line, "16.7.0") ||
             obisEquals(line, "15.7.0")) {
    reading.powerW = convertedValue(raw, unit, false);
  } else if (obisEquals(line, "1.7.0")) {
    importPowerW = convertedValue(raw, unit, false);
  } else if (obisEquals(line, "2.7.0")) {
    exportPowerW = convertedValue(raw, unit, false);
  } else {
    constexpr const char *phasePower[3] = {"36.7.0", "56.7.0", "76.7.0"};
    constexpr const char *phaseImportPower[3] = {"21.7.0", "41.7.0", "61.7.0"};
    constexpr const char *phaseExportPower[3] = {"22.7.0", "42.7.0", "62.7.0"};
    constexpr const char *phaseVoltage[3] = {"32.7.0", "52.7.0", "72.7.0"};
    constexpr const char *phaseCurrent[3] = {"31.7.0", "51.7.0", "71.7.0"};
    bool matched = false;
    for (uint8_t phase = 0; phase < 3; ++phase) {
      if (obisEquals(line, phasePower[phase])) {
        reading.phasePowerW[phase] = convertedValue(raw, unit, false);
        matched = true;
      } else if (obisEquals(line, phaseImportPower[phase])) {
        phaseImportPowerW[phase] = convertedValue(raw, unit, false);
        matched = true;
      } else if (obisEquals(line, phaseExportPower[phase])) {
        phaseExportPowerW[phase] = convertedValue(raw, unit, false);
        matched = true;
      } else if (obisEquals(line, phaseVoltage[phase])) {
        reading.phaseVoltageV[phase] = raw;
        matched = true;
      } else if (obisEquals(line, phaseCurrent[phase])) {
        reading.phaseCurrentA[phase] = convertedValue(raw, unit, false);
        matched = true;
      }
    }
    return matched;
  }
  return true;
}

}  // namespace

void D0Parser::reset() {
  frameSize_ = 0;
  capturing_ = false;
  hasStx_ = false;
  waitForBcc_ = false;
  sawBang_ = false;
  bcc_ = 0;
}

void D0Parser::storeLastFrame() {
  lastFrameSize_ = std::min(frameSize_, kMaximumFrame);
}

bool D0Parser::parseFrame(MeterData &reading,
                                   bool bccPresent, bool bccValid) {
  if (bccPresent && !bccValid) return false;
  char text[kMaximumFrame + 1];
  for (size_t i = 0; i < frameSize_; ++i) {
    const uint8_t value = frame_[i] & 0x7f;  // Also decodes 7E1 received as 8N1.
    text[i] = value == kStx ? '\n' : value == kEtx ? '\0'
                                                     : static_cast<char>(value);
  }
  text[frameSize_] = '\0';

  if (text[0] == '/') {
    size_t length = strcspn(text, "\r\n");
    length = std::min(length, sizeof(identification_) - 1);
    memcpy(identification_, text, length);
    identification_[length] = '\0';
  }

  double importPowerW = NAN;
  double exportPowerW = NAN;
  double importTariffs[2] = {NAN, NAN};
  double exportTariffs[2] = {NAN, NAN};
  double phaseImportPowerW[3] = {NAN, NAN, NAN};
  double phaseExportPowerW[3] = {NAN, NAN, NAN};
  bool found = false;
  char *cursor = text;
  while (*cursor) {
    while (*cursor == '\r' || *cursor == '\n') ++cursor;
    if (!*cursor) break;
    char *lineEnd = strpbrk(cursor, "\r\n");
    if (lineEnd) *lineEnd = '\0';
    found |= parseObisLine(cursor, reading, importPowerW, exportPowerW,
                           importTariffs, exportTariffs, phaseImportPowerW,
                           phaseExportPowerW);
    if (!lineEnd) break;
    cursor = lineEnd + 1;
  }

  if (!std::isfinite(reading.powerW) &&
      (std::isfinite(importPowerW) || std::isfinite(exportPowerW))) {
    reading.powerW = (std::isfinite(importPowerW) ? importPowerW : 0.0) -
                     (std::isfinite(exportPowerW) ? exportPowerW : 0.0);
  }
  if (!std::isfinite(reading.importKwh) &&
      (std::isfinite(importTariffs[0]) || std::isfinite(importTariffs[1]))) {
    reading.importKwh = (std::isfinite(importTariffs[0]) ? importTariffs[0] : 0.0) +
                        (std::isfinite(importTariffs[1]) ? importTariffs[1] : 0.0);
  }
  if (!std::isfinite(reading.exportKwh) &&
      (std::isfinite(exportTariffs[0]) || std::isfinite(exportTariffs[1]))) {
    reading.exportKwh = (std::isfinite(exportTariffs[0]) ? exportTariffs[0] : 0.0) +
                        (std::isfinite(exportTariffs[1]) ? exportTariffs[1] : 0.0);
  }
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (!std::isfinite(reading.phasePowerW[phase]) &&
        (std::isfinite(phaseImportPowerW[phase]) ||
         std::isfinite(phaseExportPowerW[phase]))) {
      reading.phasePowerW[phase] =
          (std::isfinite(phaseImportPowerW[phase])
               ? phaseImportPowerW[phase]
               : 0.0) -
          (std::isfinite(phaseExportPowerW[phase])
               ? phaseExportPowerW[phase]
               : 0.0);
    }
  }
  if (!std::isfinite(reading.powerW) &&
      std::isfinite(reading.phasePowerW[0]) &&
      std::isfinite(reading.phasePowerW[1]) &&
      std::isfinite(reading.phasePowerW[2])) {
    reading.powerW = reading.phasePowerW[0] + reading.phasePowerW[1] +
                     reading.phasePowerW[2];
  }
  return found;
}

MeterParseStatus D0Parser::consumeByte(uint8_t byte,
                                      MeterParseResult &result) {
  const uint8_t value = byte & 0x7f;
  if (waitForBcc_) {
    const bool valid = value == (bcc_ & 0x7f);
    if (!valid) ++checksumErrors_;
    storeLastFrame();
    result = {};
    result.protocol = MeterProtocol::Iec62056;
    result.integrityPresent = true;
    result.integrityValid = valid;
    result.frameData = frame_;
    result.frameSize = lastFrameSize_;
    const bool parsed = parseFrame(result.values, true, valid);
    reset();
    if (!valid) return MeterParseStatus::IntegrityError;
    // In automatic mode arbitrary binary SML bytes can resemble a D0 frame
    // start. Preserve the former behavior and ignore incomplete/non-D0 data.
    return parsed ? MeterParseStatus::Valid : MeterParseStatus::None;
  }

  if (!capturing_) {
    if (value != '/' && value != kStx) return MeterParseStatus::None;
    capturing_ = true;
  }
  if (frameSize_ >= kMaximumFrame) {
    reset();
    return MeterParseStatus::None;
  }
  frame_[frameSize_++] = value;

  if (value == kStx) {
    hasStx_ = true;
    bcc_ = value;
  } else if (hasStx_) {
    bcc_ ^= value;
  }
  if (value == '!') sawBang_ = true;
  if (value == '\n' && frameSize_ > 1 && frame_[0] == '/' && !sawBang_)
    identificationReady_ = true;
  if (value == kEtx && hasStx_) {
    waitForBcc_ = true;
    return MeterParseStatus::None;
  }
  if (value == '\n' && sawBang_ && !hasStx_) {
    storeLastFrame();
    result = {};
    result.protocol = MeterProtocol::Iec62056;
    result.integrityPresent = false;
    result.integrityValid = true;
    result.frameData = frame_;
    result.frameSize = lastFrameSize_;
    const bool parsed = parseFrame(result.values, false, false);
    reset();
    return parsed ? MeterParseStatus::Valid : MeterParseStatus::None;
  }
  return MeterParseStatus::None;
}

MeterParseStatus D0Parser::feed(const uint8_t *data, size_t length,
                                MeterParseResult &result) {
  for (size_t i = 0; i < length; ++i) {
    const MeterParseStatus status = consumeByte(data[i], result);
    if (status != MeterParseStatus::None) return status;
  }
  return MeterParseStatus::None;
}
