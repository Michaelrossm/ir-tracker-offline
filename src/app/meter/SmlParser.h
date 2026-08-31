#pragma once

#include "MeterParser.h"

#include <vector>

class SmlParser final : public MeterParser {
 public:
  MeterParseStatus feed(const uint8_t *data, size_t length,
                        MeterParseResult &result) override;
  void reset() override;

  static bool extractObis(const std::vector<uint8_t> &data,
                          const uint8_t obis[6], double &target);

 private:
  static constexpr size_t kMaximumFrame = 2048;

  MeterParseStatus consumeByte(uint8_t value, MeterParseResult &result);
  MeterParseStatus parseFrame(MeterParseResult &result);

  std::vector<uint8_t> frame_;
  size_t startMatched_ = 0;
  bool capturing_ = false;
  uint8_t trailerRemaining_ = 0;
};

