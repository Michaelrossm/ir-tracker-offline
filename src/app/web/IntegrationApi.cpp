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
