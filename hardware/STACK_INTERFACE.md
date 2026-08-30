# IR-Tracker-Haupt-/LAN-Stack / IR Tracker main/LAN stack interface

## Deutsch

Dieses Dokument ist der verbindliche elektrische Vertrag zwischen der
`Platine-IR` und der geplanten LAN-/PoE-Tochterplatine.

### ESP32-C3-GPIO-Belegung

| Funktion | GPIO | ESP32-C3-MINI-1-Modulpin |
|---|---:|---:|
| IR-Empfänger / UART RX | 3 | 6 |
| IR-Sender / UART TX | 6 | 20 |
| Status-LED | 5 | 19 |
| W5500 Chip Select | 0 | 12 |
| W5500 Interrupt | 1 | 13 |
| W5500 SPI-Takt | 4 | 18 |
| W5500 SPI MOSI | 7 | 21 |
| W5500 SPI MISO | 10 | 16 |

### J1-Stackverbinder

| J1 | Signal | Richtung Hauptplatine | Zweck |
|---:|---|---|---|
| 1 | GND | Versorgung | Signalmasse |
| 2 | +3V3 | Ausgang | W5500-I/O-Versorgung |
| 3 | ETH_INT | Eingang | W5500 Interrupt, aktiv Low |
| 4 | ETH_MOSI | Ausgang | SPI zum W5500 |
| 5 | ETH_MISO | Eingang | SPI vom W5500 |
| 6 | ETH_CS | Ausgang | W5500 Chip Select, aktiv Low |
| 7 | ETH_SCK | Ausgang | SPI-Takt |
| 8 | RESET_N | Ausgang | gemeinsamer Reset, aktiv Low |
| 9 | SYS_5V | Eingang | bereits entkoppelte 5-V-Versorgung der PoE-Platine |
| 10 | GND | Versorgung | Versorgungsmasse |

`USB_5V` und `POE_5V_RAW` dürfen nie direkt verbunden werden. Die Umschaltung
erfolgt ausschließlich gemäß [POWER_ORING.md](POWER_ORING.md) mit je einer
PMEG2010EA pro Quelle. PoE ist eine Stromquelle und kein eigener
Softwaretransport. Ohne zusätzliche Sense-Leitung kann die Firmware W5500,
Link und IP erkennen, aber USB- und PoE-Versorgung nicht unterscheiden.

Die Universal-Firmware prüft beim Start das W5500-`VERSIONR`. Bei erkanntem
Controller sind GPIO 0, 1, 4, 7 und 10 für GPIO-Suche und Ausgangstests gesperrt.
Fehlt der W5500, wird SPI freigegeben und der Tracker läuft unverändert über
WLAN. Ethernet erhält Routing-Priorität 150, WLAN 100 und bleibt als Fallback
verbunden. Die LAN-/PoE-Hardware ist vor Serienfreigabe real zu prüfen.

## English

This file is the binding electrical contract between the `Platine-IR` main board
and the planned `Platine-IR-LAN-POE` daughterboard.

## ESP32-C3 GPIO allocation

| Function | GPIO | ESP32-C3-MINI-1 module pin |
|---|---:|---:|
| IR receiver / UART RX | 3 | 6 |
| IR transmitter / UART TX | 6 | 20 |
| Status LED | 5 | 19 |
| W5500 chip select | 0 | 12 |
| W5500 interrupt | 1 | 13 |
| W5500 SPI clock | 4 | 18 |
| W5500 SPI MOSI | 7 | 21 |
| W5500 SPI MISO | 10 | 16 |

## J1 stack connector

Main board: 2x5, 1.27 mm male pin header.
Daughterboard: mating 2x5, 1.27 mm female socket.

| J1 pin | Signal | Direction at main board | Purpose |
|---:|---|---|---|
| 1 | GND | power | signal return |
| 2 | +3V3 | output | W5500-io supply |
| 3 | ETH_INT | input | W5500 active-low interrupt |
| 4 | ETH_MOSI | output | SPI controller to W5500 |
| 5 | ETH_MISO | input | SPI W5500 to controller |
| 6 | ETH_CS | output | W5500 active-low chip select |
| 7 | ETH_SCK | output | SPI clock |
| 8 | RESET_N | output | common active-low reset |
| 9 | SYS_5V | input | already ORed 5 V from the PoE daughterboard |
| 10 | GND | power | power return |

## Mandatory power behavior

`USB_5V` and `POE_5V_RAW` must never be connected directly. They are combined as
specified in `POWER_ORING.md`: two PMEG2010EA Schottky diodes feed `SYS_5V`, which
supplies the AP2112K input. The daughterboard must not drive J1 pin 9 when its
PoE population is not fitted.

## Firmware integration rules

- The current Wi-Fi/IR firmware defaults (RX 3, TX 6, LED 5) match the PCB.
- The universal production firmware uses CS 0, INT 1, SCK 4, MOSI 7 and
  MISO 10 for the W5500.
- GPIO discovery/test routines must exclude Ethernet GPIOs while Ethernet is
  enabled; in particular they must not drive GPIO 0, 1, 4, 7 or 10.
- `RESET_N` follows the main board reset/power-on sequence. It must satisfy the
  W5500 reset timing before firmware probes `VERSIONR`.
- Wi-Fi-only operation must remain valid with the daughterboard absent.
- PoE is an isolated power source, not a software transport. With the current
  connector there is no dedicated PoE-present signal, so firmware can report
  W5500 presence/link/IP but cannot distinguish USB power from PoE power. Add a
  protected sense signal on a future connector revision if that distinction is
  required.

## Universal firmware profile

- `solakon_tracker_offline` is the universal production build. It probes the
  W5500 VERSIONR register during boot and otherwise continues over Wi-Fi.
- Once VERSIONR confirms a W5500, GPIO 0, 1, 4, 7 and 10 are excluded from
  IR/LED configuration, RX discovery and TX tests. If probing fails, the SPI
  bus is released and existing Wi-Fi-only GPIO configurations remain valid.
- The W5500 is attached to the native ESP-IDF/lwIP stack. Ethernet has routing
  priority 150 over the Wi-Fi STA priority 100. Web, MQTT, REST and signed OTA
  therefore share one implementation and automatically fall back to Wi-Fi.
- mDNS is disabled in the 4 MiB production image to keep the existing OTA
  partition layout and safe image reserve. Use the DHCP address or router
  hostname. The developer profile may still enable mDNS.
- The Ethernet/PoE hardware must only be fitted after safe USB/PoE power ORing
  has been implemented and verified on the PCB.
