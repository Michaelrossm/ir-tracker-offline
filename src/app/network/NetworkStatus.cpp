// This module is included by main.cpp inside its private namespace.
// Standalone compilation is excluded in platformio.ini to preserve behavior.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }

bool networkConnected() { return ethernet.connected() || wifiConnected(); }

String primaryNetworkIp() {
  if (ethernet.connected()) return ethernet.localIP().toString();
  if (wifiConnected()) return WiFi.localIP().toString();
  if (accessPointMode) return WiFi.softAPIP().toString();
  return "0.0.0.0";
}

const char *primaryTransportName() {
  if (ethernet.connected()) return "ethernet";
  if (wifiConnected()) return "wifi";
  if (accessPointMode) return "setup_ap";
  return "offline";
}
