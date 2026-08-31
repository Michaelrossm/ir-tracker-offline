#pragma once

#include "MeterParser.h"

#include <Arduino.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

// DE: Lesender IEC-62056-21/D0-Parser fuer aeltere ASCII-Zaehler. Er
// akzeptiert gepruefte STX/ETX/BCC-Telegramme sowie die in der Praxis
// vorkommende passive Variante ohne BCC, sofern ein vollstaendiger,
// plausibler OBIS-Datensatz bis "!" empfangen wurde.
// EN: Read-only IEC 62056-21/D0 parser for older ASCII meters. It accepts
// checked STX/ETX/BCC frames and the commonly encountered passive variant
// without BCC when a complete, plausible OBIS data set ending in "!" arrived.
class D0Parser final : public MeterParser {
 public:
  MeterParseStatus feed(const uint8_t *data, size_t length,
                        MeterParseResult &result) override;
  void reset() override;

  const uint8_t *lastFrameData() const { return frame_; }
  size_t lastFrameSize() const { return lastFrameSize_; }
  uint32_t checksumErrors() const { return checksumErrors_; }
  bool identificationReady() const { return identificationReady_; }
  void clearIdentificationReady() { identificationReady_ = false; }

 private:
  static constexpr size_t kMaximumFrame = 2048;

  MeterParseStatus consumeByte(uint8_t byte, MeterParseResult &result);
  bool parseFrame(MeterData &reading, bool bccPresent,
                  bool bccValid);
  void storeLastFrame();

  uint8_t frame_[kMaximumFrame] = {};
  size_t frameSize_ = 0;
  size_t lastFrameSize_ = 0;
  bool capturing_ = false;
  bool hasStx_ = false;
  bool waitForBcc_ = false;
  bool sawBang_ = false;
  bool identificationReady_ = false;
  uint8_t bcc_ = 0;
  uint32_t checksumErrors_ = 0;
  char identification_[32] = {};
};
