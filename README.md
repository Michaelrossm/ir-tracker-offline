# IR Tracker Offline — 1.0.1 Beta

**Deutsch** | [English](#english)

Lokale, cloudfreie Firmware für einen ESP32-C3-basierten IR-Stromzähler-Tracker. Erstellt und gepflegt von **Michael Roßmann**.

> Unabhängiges Community-Projekt. Nicht mit Solakon verbunden und nicht von Solakon unterstützt. Solakon ist eine Marke ihrer jeweiligen Inhaber.

## Funktionen

- SML-/OBIS-Auswertung vollständig lokal
- Livewerte für Gesamtleistung, Bezug, Einspeisung sowie verfügbare L1/L2/L3-Werte
- Spannungs- und Stromwerte nur im RAM, nicht in der Historie
- lokale Historie mit Stunde, Tag, Woche, Monat, Jahr und Langzeitansicht
- interaktive Diagramme für Maus und Touch
- bis zu drei WLANs; automatischer zeitbegrenzter Setup-Hotspot als Rückfall
- Home Assistant über MQTT Discovery
- JSON, CSV, Prometheus/OpenMetrics, Influx-Ausgabe und Shelly-kompatible Leseendpunkte
- signierte manuelle WLAN-Updates sowie sichere GitHub-Prüfung mit optionaler automatischer Installation
- Einstellungs-/Historienbackup, Selbsttest und Diagnose
- browserlokale Farbauswahl und Sprache Deutsch/Englisch
- Eco-Modus, adaptiver WLAN-Sendepegel und abschaltbare optionale Schnittstellen

## Sicherheit

Die Oberfläche verwendet HTTP und gehört ausschließlich in ein vertrauenswürdiges Heim- oder getrenntes IoT-Netz. Keine Ports ins Internet freigeben. Für Fernzugriff VPN verwenden. Details: [SECURITY.md](SECURITY.md).

## Installation

Siehe [INSTALLATION.md](INSTALLATION.md). Vor jedem Flashvorgang vollständige Gerätesicherung, Einstellungen und Historie sichern. Die persönliche Original-Firmware darf nicht öffentlich verteilt werden.

## Dokumentation

- [Installation und Rückkehr / Installation and recovery](INSTALLATION.md)
- [Sicherheit / Security](SECURITY.md)
- [Schnittstellen / Interfaces](INTERFACES.md)
- [USB-Umschaltung / USB switching](USB_SWITCHING.md)
- [Hardwaretest / Hardware test](HARDWARE_TEST.md)
- [Dauertest / Soak test](SOAK_TEST.md)
- [Release-Prüfung / Release checklist](RELEASE_CHECKLIST.md)
- [Rechteprüfung / Rights review](RIGHTS_REVIEW.md)

## Lizenz

Copyright © 2026 Michael Roßmann. Lizenz: PolyForm Noncommercial 1.0.0. Private und sonstige nichtkommerzielle Nutzung ist gemäß Lizenz erlaubt; gewerbliche Nutzung ist nicht gestattet. Der verbindliche Text steht in [LICENSE.md](LICENSE.md).

---

## English

Local, cloud-free firmware for an ESP32-C3-based IR electricity meter tracker. Created and maintained by **Michael Roßmann**.

> Independent community project. Not affiliated with or endorsed by Solakon. Solakon is a trademark of its respective owners.

### Features

- fully local SML/OBIS processing
- live total power, grid import/export and available L1/L2/L3 readings
- voltage and current kept in RAM only, never in history
- local history for hour, day, week, month, year and long-term views
- interactive mouse and touch charts
- up to three Wi-Fi networks; automatic time-limited setup hotspot fallback
- Home Assistant MQTT Discovery
- JSON, CSV, Prometheus/OpenMetrics, Influx output and read-only Shelly-compatible endpoints
- signed manual Wi-Fi updates plus secure GitHub checks and optional automatic installation
- settings/history backup, guided self-test and diagnostics
- browser-local colors and German/English language selection
- Eco mode, adaptive Wi-Fi transmit power and optional interfaces that can be disabled

### Security

The interface uses HTTP and must only be operated in a trusted home network or isolated IoT network. Never expose its ports to the internet. Use a VPN for remote access. See [SECURITY.md](SECURITY.md).

### Installation

See [INSTALLATION.md](INSTALLATION.md). Before flashing, back up the complete device, settings and history. The personal original firmware must never be distributed publicly.

### License

Copyright © 2026 Michael Roßmann. Licensed under PolyForm Noncommercial 1.0.0. Private and other noncommercial use is permitted under the license; commercial use is not permitted. The authoritative terms are in [LICENSE.md](LICENSE.md).
