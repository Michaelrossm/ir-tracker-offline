// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

// DE: Gleitender Mittelwert der letzten Minute aus dem RAM-Ringpuffer.
// EN: Rolling one-minute average from the existing RAM ring buffer.
double ecoTrackerMinuteAverage() {
  const time_t now = time(nullptr);
  if (now < 1700000000 || liveCount == 0) return meter.powerW;

  double sum = 0.0;
  size_t count = 0;
  // Walk backwards from the newest sample and stop once the 60-second window
  // is left. At the normal 5-second sample interval this examines about 13
  // entries instead of all 840, even when clients poll every few seconds.
  for (size_t i = 0; i < liveCount; ++i) {
    const size_t index =
        (liveWriteIndex + kLiveSamples - 1U - i) % kLiveSamples;
    const LiveSample &sample = liveSamples[index];
    if (sample.timestamp == 0) break;
    if (sample.timestamp > static_cast<uint32_t>(now)) continue;
    if (static_cast<uint32_t>(now) - sample.timestamp > 60U) break;
    if (!std::isfinite(sample.powerW)) continue;
    sum += sample.powerW;
    ++count;
  }
  return count ? sum / static_cast<double>(count) : meter.powerW;
}

String ecoTrackerJson() {
  String json;
  json.reserve(260);
  json = "{\"power\":" + numberOrNull(meter.powerW, 2) +
         ",\"powerAvg\":" + numberOrNull(ecoTrackerMinuteAverage(), 2);

  // Phase values are optional. Never invent zeros for unavailable OBIS data.
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (!std::isfinite(meter.phasePowerW[phase]) ||
        !valueFresh(meter.phasePowerUpdatedMs[phase]))
      continue;
    json += ",\"powerPhase" + String(phase + 1) + "\":" +
            numberOrNull(meter.phasePowerW[phase], 2);
  }
  json += ",\"energyCounterIn\":" +
          numberOrNull(std::isfinite(meter.importKwh)
                           ? meter.importKwh * 1000.0
                           : NAN,
                       3);
  json += ",\"energyCounterOut\":" +
          numberOrNull(std::isfinite(meter.exportKwh)
                           ? meter.exportKwh * 1000.0
                           : NAN,
                       3);
  json += ",\"energyCounterInT1\":null,\"energyCounterInT2\":null";

  // EcoTracker-compatible age in seconds. Unknown data stays null.
  json += ",\"agePower\":";
  json += meter.powerUpdatedMs ? String((millis() - meter.powerUpdatedMs) / 1000U)
                               : String("null");
  json += "}";
  return json;
}
