// FactoryNvsProbe.cpp
// Minimal additive extension for the existing factory test.

#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

#if IR_TRACKER_ENABLE_FACTORY_TEST

struct FactoryNvsProbeState {
  bool tested = false;
  bool passed = false;
} factoryNvsProbe;

bool runFactoryNvsProbe() {
  Preferences probe;
  factoryNvsProbe.tested = true;

  if (!probe.begin("ir-fct-probe", false)) {
    factoryNvsProbe.passed = false;
    return false;
  }

  const uint32_t expected = esp_random();
  const size_t written = probe.putUInt("token", expected);
  const uint32_t actual = probe.getUInt("token", expected ^ 0xffffffffU);

  const bool removed = probe.remove("token");
  probe.end();

  factoryNvsProbe.passed =
      written == sizeof(uint32_t) && actual == expected && removed;

  eventLog.add(factoryNvsProbe.passed ? "INFO" : "ERROR", "FCT_NVS",
               factoryNvsProbe.passed
                   ? "NVS Scratch-Schreib-/Lesetest bestanden"
                   : "NVS Scratch-Schreib-/Lesetest fehlgeschlagen");
  return factoryNvsProbe.passed;
}

bool factoryNvsProbePassed() {
  return factoryNvsProbe.tested && factoryNvsProbe.passed;
}

#endif
