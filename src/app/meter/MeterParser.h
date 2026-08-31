#pragma once

#include "MeterData.h"

#include <cstddef>
#include <cstdint>

enum class MeterParseStatus : uint8_t {
  None,
  Valid,
  InvalidFrame,
  IntegrityError,
};

struct MeterParseResult {
  MeterData values;
  MeterProtocol protocol = MeterProtocol::Auto;
  bool integrityPresent = false;
  bool integrityValid = false;
  const uint8_t *frameData = nullptr;
  size_t frameSize = 0;
};

// DE: Streaming-Schnittstelle fuer SML, IEC 62056-21/D0 und spaetere
// Zaehlerprotokolle. Die Instanzen werden statisch angelegt; es gibt keine
// dynamische Parser-Allokation.
// EN: Streaming interface for SML, IEC 62056-21/D0 and future meter
// protocols. Instances are static, so parser selection requires no heap use.
class MeterParser {
 public:
  virtual ~MeterParser() = default;
  virtual MeterParseStatus feed(const uint8_t *data, size_t length,
                                MeterParseResult &result) = 0;
  virtual void reset() = 0;
};

