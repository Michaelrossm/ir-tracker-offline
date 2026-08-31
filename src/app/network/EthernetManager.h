#pragma once

#include <Arduino.h>

// DE: Bindet einen optionalen W5500 in denselben lwIP-Netzwerkstack wie WLAN
// ein. Dadurch verwenden WebServer, MQTT, REST und OTA weiterhin genau einen
// Netzwerkpfad. LAN erhält eine höhere Routing-Priorität; ohne W5500 bleibt
// die bestehende WLAN-Funktion unverändert.
// EN: Adds an optional W5500 to the same lwIP network stack as Wi-Fi. This
// keeps WebServer, MQTT, REST and OTA on one network path. Ethernet receives a
// higher route priority; without a W5500 the existing Wi-Fi behavior remains.
class EthernetManager {
 public:
  bool begin(const char *hostname);
  void loop();

  bool initialized() const { return initialized_; }
  bool hardwareDetected() const { return hardwareDetected_; }
  bool linkUp() const { return linkUp_; }
  bool connected() const { return gotIp_; }
  IPAddress localIP() const;
  const String &lastError() const { return lastError_; }

  void onEthernetEvent(int32_t eventId);
  void onGotIp(uint32_t address);
  void onLostIp();

 private:
  bool initialized_ = false;
  bool hardwareDetected_ = false;
  bool linkUp_ = false;
  bool gotIp_ = false;
  uint32_t ipAddress_ = 0;
  uint32_t lastMaintenanceMs_ = 0;
  String lastError_;
};
