# IR Tracker Offline — 2.0.0

**Deutsch** | [English](#english)

Lokale, cloudfreie Firmware für einen **ESP32-C3-basierten IR-Stromzähler-Tracker**. SML-, OBIS- sowie IEC-62056-21/D0-Daten werden direkt auf dem Gerät ausgewertet – ohne Cloud-Zwang und ohne externen Server.

Erstellt und gepflegt von **Michael Roßmann**.

> Unabhängiges Community-Projekt. Nicht mit Solakon verbunden und nicht von Solakon unterstützt. Solakon ist eine Marke ihrer jeweiligen Inhaber.

## Warum IR Tracker Offline?

Der Tracker verarbeitet Messwerte vollständig lokal und stellt sie gleichzeitig mehreren lokalen Systemen bereit. Unterstützt werden unter anderem Home Assistant MQTT Discovery, JSON/HTTP, CSV, Prometheus/OpenMetrics, Influx Line Protocol, optionale read-only Modbus-TCP-Abfragen sowie Shelly-/EcoTracker-kompatible lokale Leseendpunkte.

> **Wichtig:** Shelly- und EcoTracker-Kompatibilität bezeichnet ausschließlich kompatibles lokales API- und Netzwerkverhalten. Der IR Tracker gibt sich niemals als Originalgerät eines Fremdherstellers aus. Modell `IRTRACKER-C3`, API-Modell `IRTRACKER-C3-3EM`, Seriennummer `IRT-XXXXXX`, Hostname `irtracker-XXXXXX` und die echte ESP32-MAC bleiben neutral.

## Funktionen

- vollständig lokale SML-/OBIS- sowie IEC-62056-21/D0-Auswertung
- Livewerte für Gesamtleistung, Netzbezug, Einspeisung und verfügbare L1/L2/L3-Werte
- Home Assistant MQTT Discovery
- JSON/HTTP, CSV, Prometheus/OpenMetrics und Influx Line Protocol
- Shelly-EM-/Shelly-Pro-EM-kompatible read-only Endpunkte
- EcoTracker-kompatible lokale API unter `/v1/json`
- neutrale Integrations-API unter `/api/v1/meter`
- optionales read-only Modbus TCP
- lokale History mit Kurzzeit- und Langzeitansichten
- signierte vollständige `.irup`-Updates für Firmware und Weboberfläche
- kompakte Recovery-Oberfläche bei fehlenden oder beschädigten Webassets
- Einstellungs- und History-Backup
- Setup-Hotspot als Rückfall
- bis zu drei WLANs
- optionale W5500-LAN-Unterstützung mit WLAN-Fallback
- geschützte GPIO-/Baudraten-Diagnose
- Eco-Modus, adaptiver WLAN-Sendepegel und automatische Leistungs-Boosts
- automatische UART-/Parser-Wiederherstellung bei ausbleibenden Zählerdaten

## Neu in 2.0.0

Version 2.0.0 führt die neue **Compact-History** sowie einen deutlich sichereren Migrations- und Updatepfad ein.

### Compact-History und Aufbewahrung

- 1-Minuten-Werte für 24 Stunden
- 5-Minuten-Werte für den zweiten Tag
- 15-Minuten-Werte bis 460 Tage
- 30-Minuten-Werte bis 825 Tage
- 60-Minuten-Werte bis 1.190 Tage
- anschließend Tageswerte in einem Ringpuffer mit 3.650 Einträgen
- gespeicherte Leistung im Compact-Format mit 0,1-W-Auflösung
- übernommene Energiezählerstände behalten ihre vorhandenen Floatwerte

Die Stufen arbeiten als Ringpuffer. Ist eine Stufe voll, wird nur deren ältester Eintrag ersetzt. Die nachgelagerten gröberen History-Stufen bleiben davon unabhängig erhalten.

### Sichere Migration alter History

- Migration startet erst, wenn **beide OTA-App-Slots dieselbe validierte 2.0.0-Firmware** enthalten
- Schreibvorgänge und Platzfreigabe sind wiederaufnehmbar
- eindeutige doppelte Zeitstempel können anhand plausibler Nachbar- und Energiezählerstände aufgelöst werden
- nachweislich beschädigte Datensätze werden nicht durch erfundene Werte ersetzt und Zeitstempel werden nicht verschoben
- bei Mehrdeutigkeit, Lesefehlern oder unzureichendem Platz bleibt das Originalarchiv erhalten
- History, NVS und Einstellungen werden durch die neue Asset-Rollback-Struktur nicht verschoben

### Update- und Laufzeitverbesserungen

- Asset-Backup und Transaktionsjournal im reservierten Ende des inaktiven App-Slots
- häufigere UART-Bedienung während längerer Netzwerkoperationen
- kleinerer MQTT-Paketpuffer und Streaming-Ausgabe
- SML-/JSON-Speicheroptimierungen
- robusteres Modbus-TCP-Framing
- gehärtete mDNS- und Flash-Fehlerpfade
- WLAN-Zeitplan-NVS-Schlüssel und dessen Backup/Wiederherstellung korrigiert
- Dashboard, CSV und Backups unterstützen die zusätzliche History-Stufe

## Update auf 2.0.0

### Tracker mit „Vollständiges Update (.irup)“

1. Einstellungen und vollständige Historie sichern.
2. `ir-tracker-update-2.0.0.irup` unter **Wartung → Vollständiges Update** installieren.
3. Neustart abwarten und Messung/Weboberfläche prüfen.
4. **Dieselbe IRUP-Datei ein zweites Mal manuell installieren.**
5. Nach dem zweiten Neustart die automatische History-Migration vollständig durchlaufen lassen und das Gerät währenddessen nicht abschalten.

Der zweite Durchgang ist für den Wechsel auf die Compact-History nötig: Erst wenn beide validierten App-Slots dieselbe neue Firmware enthalten, kann ein OTA-Rollback nicht auf einen alten History-Reader zurückfallen.

### Ältere Geräte

- Geräte, die nur `.irfw` anbieten, benötigen zuerst eine nachweislich kompatible IRUP-Brückenversion oder einen datenerhaltenden USB-Installer.
- `.irup` niemals in `.irfw` umbenennen.
- Ein echter Coredump-Partitionssubtyp muss vor dem aktuellen Asset-Layout per USB migriert werden.
- Ein historisches `coredump`-Label mit bereits passendem SPIFFS-Subtyp ist zulässig.
- WLAN-Updates ändern keine Partitionstabelle.

Das normale WLAN-Update benötigt nur `ir-tracker-update-2.0.0.irup`. Das separate Asset-Image und die Partitionstabelle sind nur für Diagnose, Recovery bzw. den passenden USB-Installer vorgesehen.

## Prüfung von 2.0.0

- **80 automatisierte Tests bestanden**
- alle drei PlatformIO-Profile erfolgreich gebaut
- vollständiges signiertes IRUP-Paket erfolgreich verifiziert
- WLAN-Installation auf einem lokalen ESP32-C3 in beide App-Slots durchgeführt
- Weboberfläche, Assets, Messung, Einstellungen und archivierte Werte danach geprüft
- App-BIN: **1.237.680 Byte**
- statisches RAM: **97.004 Byte**
- nutzbare OTA-Reserve nach Asset-Backup-/Journalreservierung: **68.944 Byte**
- gemessener freier Heap nach Start: **139.836 Byte**
- gemessener Minimum-Heap: **122.328 Byte**
- größte freie Region: **114.676 Byte**
- Stack-High-Water-Mark: **3.624 Byte**

Der neue Asset-Rollback-Schutz kann den allerersten Übergang von alter Firmware nicht rückwirkend absichern. Simulationen und Hosttests ersetzen keine echten Power-Cut-Tests auf jeder Hardware. Backups bleiben empfohlen.

## Schnittstellen

| Schnittstelle | Verwendung |
| --- | --- |
| Shelly-kompatible Endpunkte | lokale read-only Einbindung in Systeme mit Shelly-EM-/Pro-EM-Unterstützung |
| EcoTracker-kompatible API | lokale Einbindung über `/v1/json` |
| MQTT Discovery | automatische Home-Assistant-Sensoren |
| JSON/HTTP | eigene Integrationen und Automatisierung |
| CSV | einfache Weiterverarbeitung |
| Prometheus / OpenMetrics | Monitoring und Zeitreihen-Erfassung |
| Influx Line Protocol | Übergabe an Influx-kompatible Systeme |
| Modbus TCP | optionales read-only Registerschema |

Der Tracker arbeitet ausschließlich als **lesender Stromzähler**. Nulleinspeisung, Ladegrenzen und Regelungen gehören weiterhin in Speicher, Wechselrichter, Wallbox oder das jeweilige Automatisierungssystem.

Siehe [INTERFACES.md](docs/INTERFACES.md) für die konkreten Endpunkte.

## Unterstützte Stromzähler

Welche Werte verfügbar sind, hängt vom angeschlossenen Stromzähler und dessen freigeschalteten OBIS-Daten ab. Neben SML unterstützt die Firmware passive und aktive IEC-62056-21/D0-Zähler. Die aktive Abfrage verwendet `/?!` und `ACK 000`.

Die aktuelle Übersicht steht unter [Kompatibilität](docs/compatibility/README.md).

## Erster Zugang

Nach einer frischen Installation startet der Tracker das WLAN `IR-Tracker-Setup-XXXX`.

| Zugang | Benutzername | Passwort |
| --- | --- | --- |
| Setup-WLAN `IR-Tracker-Setup-XXXX` | – | `IRTracker-XXXX` |
| Weboberfläche | `admin` | `IRTracker-XXXX` |

Beispiel: `IR-Tracker-Setup-F2A0` → Passwort `IRTracker-F2A0`.

## Sicherheit

Die Oberfläche verwendet HTTP und gehört ausschließlich in ein vertrauenswürdiges Heim- oder getrenntes IoT-Netz. **Keine Ports ins Internet freigeben.** Für Fernzugriff VPN verwenden. Details: [SECURITY.md](.github/SECURITY.md).

## Dokumentation

- [Release Notes 2.0.0](release/RELEASE_NOTES-2.0.0.md)
- [Installation und Wiederherstellung](docs/INSTALLATION.md)
- [Kompatibilität](docs/compatibility/README.md)
- [Sicherheit](.github/SECURITY.md)
- [Schnittstellen](docs/INTERFACES.md)
- [Modbus TCP](docs/MODBUS.md)
- [USB-Umschaltung](docs/USB_SWITCHING.md)
- [Hardwaretest](docs/HARDWARE_TEST.md)
- [Dauertest](docs/SOAK_TEST.md)
- [Release-Prüfliste](docs/RELEASE_CHECKLIST.md)
- [Firmware-Architektur](docs/ARCHITECTURE.md)
- [Webasset-Partition](docs/ASSET_PARTITION.md)
- [Rechteprüfung](docs/legal/RIGHTS_REVIEW.md)
- [Markenhinweis](docs/legal/TRADEMARKS.md)
- [Drittsoftware](docs/legal/THIRD_PARTY_NOTICES.md)

## Projektstatus

**2.0.0 ist der aktuelle stabile Stand.** Die Compact-History und die zweistufig abgesicherte WLAN-Migration wurden auf dem lokalen ESP32-C3 geprüft. Die Universal-Firmware enthält W5500-LAN mit WLAN-Fallback; die eigene LAN-/PoE-Platine wurde mit diesem Stand noch nicht auf echter Hardware validiert.

Neue USB-Installationen verwenden für den optionalen 64-kB-Debugspeicher das Label `debugfs`. Bestehende Geräte mit historischem `coredump`-Label und passendem SPIFFS-Subtyp bleiben kompatibel. Ein echter Coredump-Subtyp muss vor Verwendung als Asset-Bereich per USB migriert werden.

## Lizenz

Copyright © 2026 Michael Roßmann. Lizenz: PolyForm Noncommercial 1.0.0. Private und sonstige nichtkommerzielle Nutzung ist gemäß Lizenz erlaubt; gewerbliche Nutzung ist nicht gestattet. Verbindlich sind [LICENSE.md](LICENSE.md), [AUTHORS.md](AUTHORS.md), [RIGHTS_REVIEW.md](docs/legal/RIGHTS_REVIEW.md) und [TRADEMARKS.md](docs/legal/TRADEMARKS.md).

---

## English

Local, cloud-free firmware for an **ESP32-C3-based IR electricity meter tracker**. SML, OBIS, and IEC 62056-21/D0 data is processed directly on the device without a mandatory cloud service or external server.

Created and maintained by **Michael Roßmann**.

> Independent community project. Not affiliated with or endorsed by Solakon. Solakon is a trademark of its respective owners.

### Why IR Tracker Offline?

IR Tracker Offline keeps meter processing and history local while exposing readings through several local interfaces. It supports Home Assistant MQTT Discovery, JSON/HTTP, CSV, Prometheus/OpenMetrics, Influx Line Protocol, optional read-only Modbus TCP, and local read-only Shelly/EcoTracker-compatible endpoints.

> **Note:** Shelly and EcoTracker compatibility refers only to compatible local API and network behavior. IR Tracker does not impersonate third-party hardware. Model `IRTRACKER-C3`, API model `IRTRACKER-C3-3EM`, serial `IRT-XXXXXX`, hostname `irtracker-XXXXXX`, and the genuine ESP32 MAC remain neutral.

### Main features

- fully local SML/OBIS and IEC 62056-21/D0 processing
- live total power, grid import/export, and available L1/L2/L3 readings
- Home Assistant MQTT Discovery
- JSON/HTTP, CSV, Prometheus/OpenMetrics, and Influx Line Protocol
- read-only Shelly EM / Shelly Pro EM compatible endpoints
- EcoTracker-compatible local API at `/v1/json`
- neutral integration API at `/api/v1/meter`
- optional read-only Modbus TCP
- local short- and long-term history
- signed complete `.irup` updates containing firmware and web assets
- compact recovery UI if web assets are missing or damaged
- settings/history backup
- fallback setup hotspot
- up to three Wi-Fi networks
- optional W5500 Ethernet with Wi-Fi fallback
- protected GPIO/baud-rate diagnostics
- automatic UART/parser recovery when meter traffic stops

### What is new in 2.0.0?

Version 2.0.0 introduces the new **compact history** and a substantially safer migration/update path.

#### Compact history and retention

- 1-minute values for 24 hours
- 5-minute values for the second day
- 15-minute values up to 460 days
- 30-minute values up to 825 days
- 60-minute values up to 1,190 days
- daily values afterwards in a 3,650-entry ring buffer
- stored compact power values use 0.1 W resolution
- migrated cumulative energy counters retain their existing floating-point values

Each stage is a ring buffer. When a stage reaches capacity, only its oldest record is replaced; downstream coarser stages remain independent.

#### Safe legacy-history migration

- migration starts only after **both OTA app slots contain the same validated 2.0.0 firmware**
- migration writes and space-release operations are resumable
- unambiguous duplicate timestamps can be resolved from plausible neighboring data and cumulative energy counters
- clearly corrupted records are not replaced with invented measurements and timestamps are not shifted
- ambiguous archives, read errors, or insufficient space preserve the original archive
- history, NVS, and settings partitions are not moved by the new asset rollback layout

#### Update and runtime improvements

- asset backup and transaction journal stored in reserved space at the end of the inactive app slot
- more frequent UART servicing during longer network operations
- smaller MQTT packet buffer and streaming output
- SML/JSON memory optimizations
- hardened Modbus TCP framing
- more robust mDNS and flash-error paths
- fixed Wi-Fi schedule NVS key and backup/restore handling
- dashboard, CSV exports, and backups support the additional history stage

### Upgrading to 2.0.0

#### Devices with “Complete update (.irup)”

1. Back up settings and the complete history.
2. Install `ir-tracker-update-2.0.0.irup` under **Maintenance → Complete update**.
3. Wait for the reboot and verify meter readings and the web interface.
4. **Manually install the same IRUP file a second time.**
5. After the second reboot, allow the automatic history migration to finish and do not power the tracker off while it is running.

The second installation is required for the compact-history transition. Migration starts only when both validated OTA app slots contain the same new firmware, preventing an OTA rollback from falling back to an old history reader.

#### Older devices

- IRFW-only devices first need a verified compatible IRUP bridge or a data-preserving USB installer.
- Never rename `.irup` to `.irfw`.
- A true coredump partition subtype must be migrated over USB before using the current asset layout.
- A legacy `coredump` label that already uses the expected SPIFFS subtype is acceptable.
- Wi-Fi updates do not modify the partition table.

Normal Wi-Fi upgrades only require `ir-tracker-update-2.0.0.irup`. The separate asset image and partition table are intended for diagnostics/recovery and the matching USB installer.

### 2.0.0 verification

- **80 automated tests passed**
- all three PlatformIO profiles built successfully
- signed complete IRUP package verified successfully
- 2.0.0 installed over Wi-Fi into both app slots on a local ESP32-C3 test tracker
- web UI, assets, meter acquisition, settings, and archived readings checked afterwards
- app binary: **1,237,680 bytes**
- static RAM: **97,004 bytes**
- usable OTA reserve after asset-backup/journal reservation: **68,944 bytes**
- measured free heap after boot: **139,836 bytes**
- measured minimum free heap: **122,328 bytes**
- largest free block: **114,676 bytes**
- stack high-water mark: **3,624 bytes**

The new asset rollback protection cannot retroactively protect the very first transition from older firmware. Simulations and host tests are not a replacement for real power-cut testing on every hardware revision. Keep backups.

### Interfaces

| Interface | Use |
| --- | --- |
| Shelly-compatible endpoints | local read-only integration with systems supporting Shelly EM / Pro EM |
| EcoTracker-compatible API | local integration through `/v1/json` |
| MQTT Discovery | automatic Home Assistant sensors |
| JSON/HTTP | custom integrations and automation |
| CSV | simple data export |
| Prometheus / OpenMetrics | monitoring and time-series collection |
| Influx Line Protocol | output to Influx-compatible systems |
| Modbus TCP | optional read-only register schema |

The tracker operates exclusively as a **read-only electricity meter**. Zero-export control, charge limits, and control loops remain the responsibility of the battery, inverter, wallbox, or automation system.

See [INTERFACES.md](docs/INTERFACES.md) for the exact endpoints.

### Supported meters

Available readings depend on the connected meter and the OBIS values it exposes. In addition to SML, the firmware supports passive and active IEC 62056-21/D0 meters. Active polling uses `/?!` and `ACK 000`.

See the current [compatibility overview](docs/compatibility/README.md).

### First access

After a fresh installation, the tracker starts `IR-Tracker-Setup-XXXX`.

| Access | User name | Password |
| --- | --- | --- |
| Setup Wi-Fi `IR-Tracker-Setup-XXXX` | – | `IRTracker-XXXX` |
| Web interface | `admin` | `IRTracker-XXXX` |

Example: `IR-Tracker-Setup-F2A0` → password `IRTracker-F2A0`.

### Security

The interface uses HTTP and should only be operated on a trusted home network or isolated IoT network. **Do not expose its ports directly to the internet.** Use a VPN for remote access. See [SECURITY.md](.github/SECURITY.md).

### Documentation

- [Release notes 2.0.0](release/RELEASE_NOTES-2.0.0.md)
- [Installation and recovery](docs/INSTALLATION.md)
- [Compatibility](docs/compatibility/README.md)
- [Security](.github/SECURITY.md)
- [Interfaces](docs/INTERFACES.md)
- [Modbus TCP](docs/MODBUS.md)
- [USB switching](docs/USB_SWITCHING.md)
- [Hardware test](docs/HARDWARE_TEST.md)
- [Soak test](docs/SOAK_TEST.md)
- [Release checklist](docs/RELEASE_CHECKLIST.md)
- [Firmware architecture](docs/ARCHITECTURE.md)
- [Web asset partition](docs/ASSET_PARTITION.md)
- [Rights review](docs/legal/RIGHTS_REVIEW.md)
- [Trademarks](docs/legal/TRADEMARKS.md)
- [Third-party notices](docs/legal/THIRD_PARTY_NOTICES.md)

### Project status

**2.0.0 is the current stable release.** Compact-history migration and the two-slot Wi-Fi migration path were verified on the local ESP32-C3 test tracker. The universal firmware contains W5500 Ethernet with Wi-Fi fallback; the custom LAN/PoE board has not yet been validated on real hardware with this release.

New USB installations use the `debugfs` label for optional 64 KiB debug storage. Existing devices using the historical `coredump` label remain compatible when that partition already has the expected SPIFFS subtype. A true coredump subtype must be migrated over USB before it can be used as the asset area.

### License

Copyright © 2026 Michael Roßmann. Licensed under PolyForm Noncommercial 1.0.0. Private and other noncommercial use is permitted; commercial use is not permitted. See [LICENSE.md](LICENSE.md), [AUTHORS.md](AUTHORS.md), [RIGHTS_REVIEW.md](docs/legal/RIGHTS_REVIEW.md), and [TRADEMARKS.md](docs/legal/TRADEMARKS.md).

---

<small>Hinweis / Note: Die LAN-/PoE-Platine wurde mit 2.0.0 noch nicht auf echter Hardware validiert. / The LAN/PoE board has not yet been validated on real hardware with 2.0.0.</small>
