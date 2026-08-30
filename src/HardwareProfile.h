#pragma once

#include <stdint.h>

// DE: Standard bleibt mit bestehenden WLAN/IR-Trackern kompatibel. Das
// Das Universalprofil reserviert die fest verdrahteten W5500-Pins. Fehlt der
// W5500, laeuft dieselbe Firmware unveraendert ueber WLAN.
// EN: The default remains compatible with existing Wi-Fi/IR trackers. The LAN
// universal profile reserves the hard-wired W5500 pins. If the W5500 is not
// fitted, the same firmware continues over Wi-Fi.
#ifndef IR_TRACKER_LAN_PROFILE
#define IR_TRACKER_LAN_PROFILE 0
#endif

#ifndef IR_TRACKER_ENABLE_MDNS
#define IR_TRACKER_ENABLE_MDNS 0
#endif

// DE: Rohdaten-Sniffer und die schreibende IR-Bridge sind ausschliesslich
// Werkzeuge fuer bewusst erzeugte Entwickler-Builds. Produktionsprofile
// enthalten weder die WebSocket-Server noch deren Konfiguration.
// EN: The raw-data sniffer and writable IR bridge are tools for explicitly
// selected developer builds only. Production profiles contain neither the
// WebSocket servers nor their configuration.
#ifndef IR_TRACKER_ENABLE_DEVELOPER_IO
#define IR_TRACKER_ENABLE_DEVELOPER_IO 0
#endif

// DE: Der Werkspruefungsmodus wird nur in einem bewusst gewaehlten FCT-Build
// eingebaut. Dadurch belegt die Bedienoberflaeche im normalen OTA-Image keinen
// Platz. EN: Factory testing is only included in an explicitly selected FCT
// build, keeping its UI and test logic out of the normal OTA image.
#ifndef IR_TRACKER_ENABLE_FACTORY_TEST
#define IR_TRACKER_ENABLE_FACTORY_TEST 0
#endif

#ifndef IR_TRACKER_ENABLE_GITHUB_UPDATE
#define IR_TRACKER_ENABLE_GITHUB_UPDATE 1
#endif

namespace HardwareProfile {

constexpr bool kLanPrepared = IR_TRACKER_LAN_PROFILE != 0;
constexpr bool kMdnsEnabled = IR_TRACKER_ENABLE_MDNS != 0;
constexpr bool kDeveloperIoEnabled = IR_TRACKER_ENABLE_DEVELOPER_IO != 0;
constexpr bool kFactoryTestEnabled = IR_TRACKER_ENABLE_FACTORY_TEST != 0;
constexpr uint8_t kW5500CsPin = 0;
constexpr uint8_t kW5500IntPin = 1;
constexpr uint8_t kW5500SckPin = 4;
constexpr uint8_t kW5500MosiPin = 7;
constexpr uint8_t kW5500MisoPin = 10;

constexpr bool isW5500Pin(const int pin) {
  return pin == kW5500CsPin || pin == kW5500IntPin ||
         pin == kW5500SckPin || pin == kW5500MosiPin ||
         pin == kW5500MisoPin;
}

constexpr bool trackerGpioAvailable(const int pin,
                                    const bool reserveEthernetPins) {
  return pin >= 0 && pin <= 10 &&
         (!reserveEthernetPins || !isW5500Pin(pin));
}

static_assert(!isW5500Pin(3) && !isW5500Pin(5) && !isW5500Pin(6),
              "W5500 pinout overlaps the fixed IR/LED defaults");

}  // namespace HardwareProfile
