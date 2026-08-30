# IR Tracker Offline — 1.3.0

**Deutsch** | [English](#english)

Lokale, cloudfreie Firmware für einen **ESP32-C3-basierten IR-Stromzähler-Tracker**. SML- und OBIS-Daten werden direkt auf dem Gerät ausgewertet – ohne Cloud-Zwang und ohne externen Server.

Erstellt und gepflegt von **Michael Roßmann**.

> Unabhängiges Community-Projekt. Nicht mit Solakon verbunden und nicht von Solakon unterstützt. Solakon ist eine Marke ihrer jeweiligen Inhaber.

## Warum IR Tracker Offline?

Der Tracker soll Messwerte nicht nur anzeigen, sondern sie möglichst einfach für bestehende lokale Systeme bereitstellen. Deshalb stehen mehrere Schnittstellen parallel zur Verfügung.

Besonders praktisch ist die **Shelly-Kompatibilität**: Der Tracker stellt nur lesende Shelly-EM- und Shelly-Pro-EM-kompatible Endpunkte bereit. Systeme, die entsprechende Shelly-Energiemessgeräte abfragen können, können dadurch unter Umständen ohne eine speziell für den IR Tracker entwickelte Integration auf die Messwerte zugreifen.

Zusätzlich stehen **Home Assistant MQTT Discovery, JSON/HTTP, CSV, Prometheus/OpenMetrics und Influx Line Protocol** zur Verfügung. Die komplette SML-/OBIS-Auswertung findet lokal auf dem ESP32-C3 statt.

> **Wichtig:** Shelly-Kompatibilität bedeutet keine garantierte Kompatibilität mit jedem Produkt oder jeder Software, die Shelly-Geräte unterstützt. Der Tracker emuliert ausgewählte, nur lesende Messendpunkte.

## Funktionen

- **vollständig lokale SML-/OBIS- sowie IEC-62056-21/D0-Auswertung** ohne Cloud-Abhängigkeit
- Livewerte für Gesamtleistung, Netzbezug, Einspeisung sowie verfügbare L1/L2/L3-Werte
- **Shelly-EM- und Shelly-Pro-EM-kompatible Leseendpunkte** für eine möglichst einfache Einbindung in bestehende Systeme
- **Home Assistant über MQTT Discovery**
- lokale Schnittstellen über **JSON/HTTP, CSV, Prometheus/OpenMetrics und Influx Line Protocol**
- lokale Historie mit Stunde, Tag, Woche, Monat, Jahr und Langzeitansicht
- Spannungs- und Stromwerte nur im RAM, nicht in der Historie
- interaktive Diagramme für Maus und Touch
- bis zu drei WLANs; automatischer zeitbegrenzter Setup-Hotspot als Rückfall
- eine Universal-Firmware für WLAN und optionales W5500-LAN; LAN wird bei
  vorhandenem Link bevorzugt, WLAN bleibt als automatischer Rückfall verbunden
- signierte manuelle WLAN-Updates sowie sichere GitHub-Prüfung mit optionaler automatischer Installation
- Einstellungs-/Historienbackup, Selbsttest und Diagnose
- geschützte GPIO-/Baudraten-Diagnose zur Unterstützung unterschiedlicher Hardware
- browserlokale Farbauswahl und Sprache Deutsch/Englisch
- Eco-Modus, adaptiver WLAN-Sendepegel und automatische Leistungs-Boosts für
  Verbindungsaufbau, Updates, Exporte und Prüfungen
- automatische UART-/Parser-Wiederherstellung bei ausbleibenden Zählerdaten
- gesonderter Werksprüfungs-Build mit PASS/FAIL-Prüfung für die eigene LAN-/PoE-Platine
- Produktions-Build ausschließlich mit lesenden Speicher-/Smart-Home-Schnittstellen

## Schnittstellen und Integration

| Schnittstelle | Verwendung |
| --- | --- |
| Shelly-kompatible Endpunkte | Einbindung in Systeme, die Shelly EM / Pro EM abfragen können |
| MQTT Discovery | automatische Sensoren in Home Assistant |
| JSON/HTTP | eigene Integrationen, Automatisierung und lokale Abfragen |
| CSV | einfache Weiterverarbeitung aktueller Werte |
| Prometheus / OpenMetrics | Monitoring und Zeitreihen-Erfassung |
| Influx Line Protocol | Übergabe an Influx-kompatible Systeme |

Der Tracker arbeitet dabei ausschließlich als **lesender Stromzähler**. Regelungen wie Nulleinspeisung, Ladegrenzen oder Zeitpläne gehören weiterhin in Speicher, Wechselrichter, Wallbox oder das jeweilige Automatisierungssystem.

Die konkreten URLs und API-Endpunkte stehen in [INTERFACES.md](INTERFACES.md).

## Unterstützte Messwerte und Stromzähler

Welche Werte verfügbar sind, hängt vom angeschlossenen Stromzähler und dessen freigeschalteten OBIS-Daten ab. L1/L2/L3, Spannung oder Strom können nur ausgegeben werden, wenn der Zähler diese Werte tatsächlich über seine optische Schnittstelle überträgt.

Neben SML unterstützt die Firmware passive und aktive IEC-62056-21/D0-Zähler.
Die aktive Abfrage verwendet `/?!` und `ACK 000`. Für Hardware mit unbekannter
RX-Belegung steht eine geschützte GPIO-/Baudraten-Diagnose zur Verfügung; ein
Eingang wird erst nach einem frischen, gültigen Telegramm bestätigt.

## Erster Zugang – Standardpasswort

Nach einer frischen Installation startet der Tracker das WLAN `IR-Tracker-Setup-XXXX`.
Die vier Zeichen `XXXX` werden direkt aus diesem WLAN-Namen übernommen:

| Zugang | Benutzername | Passwort |
| --- | --- | --- |
| Setup-WLAN `IR-Tracker-Setup-XXXX` | – | `IRTracker-XXXX` |
| Weboberfläche | `admin` | `IRTracker-XXXX` |

Beispiel: Heißt das WLAN `IR-Tracker-Setup-F2A0`, lautet das Passwort `IRTracker-F2A0`. Groß-/Kleinschreibung und Bindestrich müssen exakt stimmen. Nach einer eigenen Passwortänderung gilt stattdessen das selbst gewählte Admin-Passwort.

## Sicherheit

Die Oberfläche verwendet HTTP und gehört ausschließlich in ein vertrauenswürdiges Heim- oder getrenntes IoT-Netz. **Keine Ports ins Internet freigeben.** Für Fernzugriff VPN verwenden. Details: [SECURITY.md](SECURITY.md).

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

## Projektstatus

Version **1.3.0 ist die erste stabile Veröffentlichung** ohne Beta-Kennzeichnung.
Rückmeldungen zu unterschiedlichen Stromzählern und lokalen Integrationen sind willkommen.

Die Universal-Firmware enthält bereits W5500-LAN mit WLAN-Fallback. Die
LAN-/PoE-Platine ist noch nicht auf echter Hardware validiert.

## Lizenz

Copyright © 2026 Michael Roßmann. Lizenz: PolyForm Noncommercial 1.0.0. Private und sonstige nichtkommerzielle Nutzung ist gemäß Lizenz erlaubt; gewerbliche Nutzung ist nicht gestattet. Verbindlich sind [LICENSE.md](LICENSE.md), der [Urheberhinweis](AUTHORS.md), die [Rechteprüfung](RIGHTS_REVIEW.md) und der [Markenhinweis](TRADEMARKS.md).

---

## English

Local, cloud-free firmware for an **ESP32-C3-based IR electricity meter tracker**. SML and OBIS data is processed directly on the device without requiring a cloud service or external server.

Created and maintained by **Michael Roßmann**.

> Independent community project. Not affiliated with or endorsed by Solakon. Solakon is a trademark of its respective owners.

### Why IR Tracker Offline?

The tracker is designed not only to display readings but also to expose them to existing local systems through several interfaces.

A particularly useful feature is **Shelly compatibility**. The tracker provides read-only Shelly EM and Shelly Pro EM compatible endpoints. Systems capable of reading corresponding Shelly energy meters may therefore be able to consume IR Tracker measurements without a dedicated IR Tracker integration.

It also provides **Home Assistant MQTT Discovery, JSON/HTTP, CSV, Prometheus/OpenMetrics and Influx Line Protocol**. SML/OBIS processing remains completely local on the ESP32-C3.

> **Note:** Shelly compatibility does not guarantee compatibility with every product or application supporting Shelly devices. IR Tracker emulates selected read-only measurement endpoints.

### Features

- **fully local SML/OBIS and IEC 62056-21/D0 processing** without cloud dependency
- live total power, grid import/export and available L1/L2/L3 readings
- **read-only Shelly EM and Shelly Pro EM compatible endpoints** for easier integration with existing systems
- **Home Assistant MQTT Discovery**
- local **JSON/HTTP, CSV, Prometheus/OpenMetrics and Influx Line Protocol** interfaces
- local history for hour, day, week, month, year and long-term views
- voltage and current kept in RAM only, never in history
- interactive mouse and touch charts
- up to three Wi-Fi networks; automatic time-limited setup hotspot fallback
- one universal firmware for Wi-Fi and optional W5500 Ethernet; Ethernet is
  preferred while linked and Wi-Fi remains connected as automatic fallback
- signed manual Wi-Fi updates plus secure GitHub checks and optional automatic installation
- settings/history backup, guided self-test and diagnostics
- protected GPIO/baud-rate diagnostics for different hardware variants
- browser-local colors and German/English language selection
- Eco mode, adaptive Wi-Fi transmit power and automatic performance boosts for
  association, updates, exports and tests
- automatic UART/parser recovery when meter data stops
- separate factory-test build with PASS/FAIL checks for the custom LAN/PoE board
- production build contains read-only battery and smart-home interfaces only

### Interfaces and integration

| Interface | Use |
| --- | --- |
| Shelly-compatible endpoints | integration with systems capable of reading Shelly EM / Pro EM |
| MQTT Discovery | automatic Home Assistant sensors |
| JSON/HTTP | custom integrations, automation and local queries |
| CSV | simple processing of current readings |
| Prometheus / OpenMetrics | monitoring and time-series collection |
| Influx Line Protocol | output to Influx-compatible systems |

The tracker operates exclusively as a **read-only electricity meter**. Zero-export control, charge limits and schedules remain the responsibility of the battery, inverter, wallbox or automation system.

See [INTERFACES.md](INTERFACES.md) for the actual URLs and API endpoints.

### Supported readings and meters

Available readings depend on the connected electricity meter and the OBIS values it exposes. L1/L2/L3, voltage and current can only be provided when the meter actually transmits those values through its optical interface.

In addition to SML, the firmware supports passive and active IEC 62056-21/D0
meters. Active polling uses `/?!` and `ACK 000`. A protected GPIO/baud-rate
diagnostic is available for hardware with an unknown RX pin; an input is only
accepted after a fresh, valid telegram.

### First access – default password

After a fresh installation, the tracker starts the Wi-Fi network `IR-Tracker-Setup-XXXX`. Copy the four `XXXX` characters directly from that network name:

| Access | User name | Password |
| --- | --- | --- |
| Setup Wi-Fi `IR-Tracker-Setup-XXXX` | – | `IRTracker-XXXX` |
| Web interface | `admin` | `IRTracker-XXXX` |

Example: If the Wi-Fi network is named `IR-Tracker-Setup-F2A0`, the password is `IRTracker-F2A0`. Capitalization and the hyphen must match exactly. After setting a custom password, use that chosen administrator password instead.

### Security

The interface uses HTTP and must only be operated in a trusted home network or isolated IoT network. **Never expose its ports to the internet.** Use a VPN for remote access. See [SECURITY.md](SECURITY.md).

### Installation

See [INSTALLATION.md](INSTALLATION.md). Before flashing, back up the complete device, settings and history. The personal original firmware must never be distributed publicly.

### Documentation

- [Installation and recovery](INSTALLATION.md)
- [Security](SECURITY.md)
- [Interfaces](INTERFACES.md)
- [USB switching](USB_SWITCHING.md)
- [Hardware test](HARDWARE_TEST.md)
- [Soak test](SOAK_TEST.md)
- [Release checklist](RELEASE_CHECKLIST.md)
- [Rights review](RIGHTS_REVIEW.md)

### Project status

Version **1.3.0 is the first stable release** without a beta suffix. Feedback
about different electricity meters and local integrations is welcome.

The universal firmware already contains W5500 Ethernet with Wi-Fi fallback.
The Ethernet/PoE board has not yet been validated on real hardware.

### License

Copyright © 2026 Michael Roßmann. Licensed under PolyForm Noncommercial 1.0.0. Private and other noncommercial use is permitted under the license; commercial use is not permitted. See the authoritative [license](LICENSE.md), [authorship notice](AUTHORS.md), [rights review](RIGHTS_REVIEW.md), and [trademark notice](TRADEMARKS.md).

---

<small>Hinweis / Note: Die LAN-Unterstützung wurde noch nicht an echter Hardware getestet. / Ethernet support has not yet been tested on real hardware.</small>
