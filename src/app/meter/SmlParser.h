#pragma once

#include "MeterParser.h"

#include <vector>

class SmlParser final : public MeterParser {
 public:
  struct Diagnostics {
    uint16_t qualificationMatches;
    uint16_t qualificationTarget;
    uint16_t sentinelInterval;
    uint32_t comparisonMismatches;
    uint32_t sentinelComparisons;
    bool qualificationComplete;
    bool onePassActive;
    bool legacyFallbackLatched;
  };

  MeterParseStatus feed(const uint8_t *data, size_t length,
                        MeterParseResult &result) override;
  void reset() override;
  Diagnostics diagnostics() const;

  static bool extractObis(const std::vector<uint8_t> &data,
                          const uint8_t obis[6], double &target);

 private:
  static constexpr size_t kMaximumFrame = 2048;
  static constexpr uint16_t kQualificationFrames = 32;
  static constexpr uint16_t kSentinelInterval = 512;

  MeterParseStatus consumeByte(uint8_t value, MeterParseResult &result);
  MeterParseStatus parseFrame(MeterParseResult &result);

  std::vector<uint8_t> frame_;
  size_t startMatched_ = 0;
  bool capturing_ = false;
  uint8_t trailerRemaining_ = 0;
  uint16_t qualificationMatches_ = 0;
  uint16_t sentinelCountdown_ = kSentinelInterval;
  uint32_t comparisonMismatches_ = 0;
  uint32_t sentinelComparisons_ = 0;
  bool onePassQualified_ = false;
  bool legacyFallbackLatched_ = false;
};
