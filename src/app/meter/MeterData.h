#pragma once

#include <Arduino.h>

#include <cmath>
#include <cstdint>

// DE: Ein gemeinsames, normalisiertes Messwertmodell fuer alle Parser und
// Ausgabeschnittstellen. Positive Leistung bedeutet Netzbezug, negative
// Leistung Einspeisung. Energiezaehler sind kumulative kWh-Werte.
// EN: One normalized measurement model for every parser and output interface.
// Positive power means grid import, negative power means export. Energy
// counters are cumulative kWh values.
enum class MeterProtocol : uint8_t {
  Auto = 0,
  Sml = 1,
  Iec62056 = 2,
  Iec62056Active = 3,
};

inline const char *meterProtocolName(MeterProtocol protocol) {
  switch (protocol) {
    case MeterProtocol::Sml: return "sml";
    case MeterProtocol::Iec62056: return "iec62056-21";
    case MeterProtocol::Iec62056Active: return "iec62056-21-active";
    default: return "auto";
  }
}

struct MeterData {
  double powerW = NAN;
  double importKwh = NAN;
  double exportKwh = NAN;
  double phasePowerW[3] = {NAN, NAN, NAN};
  double phaseVoltageV[3] = {NAN, NAN, NAN};
  double phaseCurrentA[3] = {NAN, NAN, NAN};

  uint32_t telegrams = 0;
  uint32_t bytes = 0;
  uint32_t parseErrors = 0;
  uint32_t crcErrors = 0;     // Legacy-compatible sum of all integrity events.
  uint32_t smlCrcErrors = 0;  // CRC failures from complete SML frame candidates.
  uint32_t lastTelegramMs = 0;
  uint32_t powerUpdatedMs = 0;
  uint32_t importUpdatedMs = 0;
  uint32_t exportUpdatedMs = 0;
  uint32_t phasePowerUpdatedMs[3] = {};
  uint32_t phaseVoltageUpdatedMs[3] = {};
  uint32_t phaseCurrentUpdatedMs[3] = {};
  bool lastCrcValid = false;
  bool lastIntegrityPresent = false;
  MeterProtocol detectedProtocol = MeterProtocol::Auto;
};
