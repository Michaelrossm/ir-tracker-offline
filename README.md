# IR Tracker Offline — 2.0.0

**Deutsch** | [English](#english)

Lokale, cloudfreie Firmware für einen **ESP32-C3-basierten IR-Stromzähler-Tracker**. SML-, OBIS- sowie IEC-62056-21/D0-Daten werden direkt auf dem Gerät ausgewertet – ohne Cloud-Zwang und ohne externen Server.

Erstellt und gepflegt von **Michael Roßmann**.

> Unabhängiges Community-Projekt. Nicht mit Solakon verbunden und nicht von Solakon unterstützt. Solakon ist eine Marke ihrer jeweiligen Inhaber.

## Kernfunktionen

- lokale SML-/OBIS- und IEC-62056-21/D0-Auswertung
- Gesamtleistung, Bezug, Einspeisung und – soweit vom Zähler geliefert – L1/L2/L3-Werte
- Weboberfläche und lokale Compact-History
- neutrale HTTP-API unter `/api/v1/meter`
- Home Assistant MQTT Discovery, CSV, Prometheus/OpenMetrics und Influx Line Protocol
- optionale read-only Modbus-TCP-Schnittstelle
- read-only Shelly-/EcoTracker-kompatible lokale Endpunkte
- bis zu drei WLANs und Setup-Hotspot als Rückfall
- optionale W5500-LAN-Unterstützung mit WLAN-Fallback
- signierte vollständige `.irup`-Updates für Firmware und Weboberfläche
- Recovery-Oberfläche bei fehlenden oder beschädigten Webassets
- Einstellungs-/History-Backup und geschützte Diagnosefunktionen

> **Kompatibilität bedeutet nur das implementierte lokale API-/Netzwerkverhalten.** Der IR Tracker behält seine neutrale Identität und gibt sich nicht als Originalgerät eines Fremdherstellers aus.

## Neu in 2.0.0

2.0.0 führt die Compact-History und einen abgesicherten Migrations-/Updatepfad ein. Die Aufbewahrung wird stufenweise verdichtet: 1-Minuten-Werte für 24 Stunden, 5-Minuten-Werte für den zweiten Tag, 15-Minuten-Werte bis 460 Tage, 30-Minuten-Werte bis 825 Tage, 60-Minuten-Werte bis 1.190 Tage und anschließend Tageswerte in einem Ringpuffer mit 3.650 Einträgen. Gespeicherte Leistung verwendet 0,1-W-Auflösung; übernommene Energiezählerstände behalten ihre vorhandenen Floatwerte.

Die Migration startet erst, wenn **beide validierten OTA-App-Slots dieselbe 2.0.0-Firmware** enthalten. Asset-Backup und Transaktionsjournal liegen im reservierten Ende des inaktiven App-Slots, ohne History, NVS oder Einstellungen zu verschieben.

## Update auf 2.0.0

Vor dem Upgrade Einstellungen und vollständige History sichern. Bei einem Tracker, der bereits **Vollständiges Update (.irup)** anbietet, `ir-tracker-update-2.0.0.irup` installieren, Neustart und Messung prüfen und anschließend **dieselbe IRUP-Datei ein zweites Mal manuell installieren**. Erst danach kann die Compact-History-Migration starten. Während der Migration das Gerät nicht abschalten.

Ältere Geräte, die nur `.irfw` anbieten, benötigen zuerst eine nachweislich kompatible IRUP-Brückenversion oder den datenerhaltenden USB-Installer. `.irup` niemals in `.irfw` umbenennen. Ein echter Coredump-Partitionssubtyp muss vor Verwendung als Asset-Bereich per USB migriert werden; ein historisches `coredump`-Label mit bereits passendem SPIFFS-Subtyp ist zulässig. WLAN-Updates verändern die Partitionstabelle nicht.

Die vollständigen Hinweise und Grenzen stehen in den [Release Notes 2.0.0](release/RELEASE_NOTES-2.0.0.md) und unter [Installation](docs/INSTALLATION.md).

## Schnittstellen

Für neue Integrationen ist `/api/v1/meter` die bevorzugte stabile, herstellerneutrale Schnittstelle. `/api/v1/status` enthält einen deutlich größeren Laufzeit-/Diagnosestatus und sollte nicht als möglichst kleines Integrationsschema behandelt werden. Der Tracker arbeitet als **lesender Stromzähler**; Nulleinspeisung, Ladegrenzen und Regelung bleiben Aufgabe von Speicher, Wechselrichter, Wallbox oder Automatisierungssystem.

- [Schnittstellen-Übersicht](docs/INTERFACES.md)
- [HTTP-API-Referenz](docs/API.md)
- [Modbus-TCP-Register](docs/MODBUS.md)
- [Zähler-Kompatibilität](docs/compatibility/README.md)

## Erster Zugang

Nach einer frischen Installation startet der Tracker das WLAN `IR-Tracker-Setup-XXXX`. Die installationsspezifischen Zugangsdaten und Recovery-Schritte sind unter [Installation](docs/INSTALLATION.md) dokumentiert. Die Oberfläche verwendet HTTP und gehört ausschließlich in ein vertrauenswürdiges Heim- oder getrenntes IoT-Netz. **Keine Ports ins Internet freigeben.** Für Fernzugriff VPN verwenden.

## Dokumentation

- [Installation und Wiederherstellung](docs/INSTALLATION.md)
- [Release Notes 2.0.0](release/RELEASE_NOTES-2.0.0.md)
- [HTTP-API](docs/API.md)
- [Schnittstellen](docs/INTERFACES.md)
- [Firmware-Architektur](docs/ARCHITECTURE.md)
- [Webasset-Partition](docs/ASSET_PARTITION.md)
- [Modbus TCP](docs/MODBUS.md)
- [USB-Umschaltung](docs/USB_SWITCHING.md)
- [Hardwaretest](docs/HARDWARE_TEST.md)
- [Dauertest](docs/SOAK_TEST.md)
- [Release-Prüfliste](docs/RELEASE_CHECKLIST.md)
- [Rechteprüfung](docs/legal/RIGHTS_REVIEW.md)
- [Markenhinweis](docs/legal/TRADEMARKS.md)
- [Drittsoftware](docs/legal/THIRD_PARTY_NOTICES.md)

## Projektstatus

**2.0.0 ist der dokumentierte aktuelle stabile Stand.** Compact-History und der zweistufig abgesicherte WLAN-Migrationsweg wurden auf dem lokalen ESP32-C3 geprüft. Die Universal-Firmware enthält W5500-LAN mit WLAN-Fallback; die eigene LAN-/PoE-Platine wurde mit diesem Release noch nicht auf echter Hardware validiert.

Die ausführlichen Messwerte des geprüften Builds (Tests, Binärgröße, RAM/Heap und OTA-Reserve) stehen in den [Release Notes 2.0.0](release/RELEASE_NOTES-2.0.0.md), damit diese versionsgebundenen Zahlen nicht mehrfach im Repository gepflegt werden müssen.

## Lizenz

Copyright © 2026 Michael Roßmann. Lizenz: PolyForm Noncommercial 1.0.0. Private und sonstige nichtkommerzielle Nutzung ist gemäß Lizenz erlaubt; gewerbliche Nutzung ist nicht gestattet. Verbindlich sind [LICENSE.md](LICENSE.md), [AUTHORS.md](AUTHORS.md), [RIGHTS_REVIEW.md](docs/legal/RIGHTS_REVIEW.md) und [TRADEMARKS.md](docs/legal/TRADEMARKS.md).

---

<a id="english"></a>

## English

Local, cloud-free firmware for an **ESP32-C3-based infrared electricity-meter tracker**. SML/OBIS and IEC 62056-21/D0 data are processed directly on the device without a mandatory cloud service or external server.

Maintained by **Michael Roßmann**. This is an independent community project and is not affiliated with or supported by Solakon.

### Core features

The firmware provides a local web UI and compact history, the vendor-neutral `/api/v1/meter` HTTP API, Home Assistant MQTT Discovery, CSV, Prometheus/OpenMetrics, Influx Line Protocol, optional read-only Modbus TCP, and read-only Shelly-/EcoTracker-compatible local endpoints. Wi-Fi setup/fallback and optional W5500 Ethernet are supported. Complete signed `.irup` updates cover firmware and web assets.

Compatibility refers only to the implemented local API/network behavior. IR Tracker keeps its own neutral identity and does not impersonate third-party devices.

### Version 2.0.0

2.0.0 introduces Compact History and a safer migration/update path. History migration starts only when both validated OTA app slots contain the matching 2.0.0 firmware. Install the same complete 2.0.0 IRUP twice as described in the [Release Notes](release/RELEASE_NOTES-2.0.0.md) and [Installation guide](docs/INSTALLATION.md), and keep a backup before upgrading.

### Interfaces and documentation

New integrations should prefer `/api/v1/meter`; `/api/v1/status` is the larger runtime/diagnostic object. The tracker is a read-only electricity meter gateway: battery/inverter control remains outside the normal integration contract.

See [HTTP API](docs/API.md), [Interfaces](docs/INTERFACES.md), [Modbus](docs/MODBUS.md), [Architecture](docs/ARCHITECTURE.md), [Asset partition](docs/ASSET_PARTITION.md), [Compatibility](docs/compatibility/README.md), [Security](.github/SECURITY.md), and the [2.0.0 Release Notes](release/RELEASE_NOTES-2.0.0.md).

## License

Copyright © 2026 Michael Roßmann. Licensed under PolyForm Noncommercial 1.0.0. See [LICENSE.md](LICENSE.md) and the legal documentation for binding terms.
