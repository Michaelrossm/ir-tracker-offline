// This module is included by main.cpp inside its private namespace.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

// DE: Alle nur lesenden Integrationen verwenden denselben Antwortpfad. Damit
// bleiben Zugriffsschutz, Cache-Verhalten und Versionsangabe bei JSON, Shelly,
// EcoTracker, Prometheus, Influx und CSV identisch.
// EN: Every read-only integration uses the same response path so access
// control, caching and version metadata remain consistent across APIs.
void sendIntegrationResponse(const char *contentType, const String &payload,
                             int status = 200) {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.sendHeader("X-IR-Tracker-Mode", "read-only");
  server.sendHeader("X-IR-Tracker-Version", kFirmwareVersion);
  server.send(status, contentType, payload);
}

void sendIntegrationJson(const String &payload, int status = 200) {
  sendIntegrationResponse("application/json", payload, status);
}

String neutralMeterJson() {
  String json;
  json.reserve(560);
  json = "{\"schema\":\"irtracker.meter.v1\",\"model\":\"";
  json += DeviceIdentity::kModel;
  json += "\",\"serial\":\"" + String(deviceIdentity.serial) + "\",";
  const time_t now = time(nullptr);
  json += "\"timestamp\":";
  json += now >= 1700000000 ? String(static_cast<uint32_t>(now)) : "null";
  json += ",\"fresh\":" +
          String(valueFresh(meter.powerUpdatedMs) ? "true" : "false") +
          ",\"age_s\":" + ageOrNull(meter.powerUpdatedMs) +
          ",\"power\":" + numberOrNull(meter.powerW, 2) +
          ",\"import\":" + numberOrNull(meter.importKwh, 6) +
          ",\"export\":" + numberOrNull(meter.exportKwh, 6) +
          ",\"phases\":[";
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (phase) json += ',';
    json += "{\"id\":" + String(phase + 1) +
            ",\"power\":" + numberOrNull(meter.phasePowerW[phase], 2) +
            ",\"voltage\":" + numberOrNull(meter.phaseVoltageV[phase], 2) +
            ",\"current\":" + numberOrNull(meter.phaseCurrentA[phase], 3) +
            "}";
  }
  json += "],\"units\":{\"power\":\"W\",\"energy\":\"kWh\",";
  json += "\"voltage\":\"V\",\"current\":\"A\"}}";
  return json;
}
