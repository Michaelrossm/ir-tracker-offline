# Änderungsprotokoll / Changelog

## 1.0.1-beta.1 — 2026-08-06

- DE: Sichere GitHub-Updateprüfung mit optionaler automatischer Installation
  kryptografisch signierter IRFW-Pakete ergänzt; automatische Installation ist
  standardmäßig deaktiviert.
- EN: Added secure GitHub update checks with optional automatic installation of
  cryptographically signed IRFW packages; automatic installation is disabled
  by default.
- DE: Der weiterhin lokale, unveröffentlichte Installer besitzt eine vorbereitete
  Selbstupdate-Prüfung mit Plattformfilter und verpflichtender SHA-256-Prüfung.
- EN: The still local and unpublished installer now has a prepared self-update
  check with platform filtering and mandatory SHA-256 verification.

- DE: Echte, nicht dauerhaft schreibende GPIO-/Baud-Suche über WLAN ergänzt. Ein
  RX-Pin gilt nur nach einem frischen, CRC-gültigen SML-Telegramm als erkannt.
- EN: Added a real, non-persistent GPIO/baud scan over Wi-Fi. An RX pin is only
  accepted after receiving a fresh, CRC-valid SML telegram.
- DE: Installer kann die Suche an einem bereits laufenden Custom-Tracker starten;
  Profilwerte werden nicht mehr als Messergebnis ausgegeben.
- EN: The installer can start the scan on an already running custom tracker;
  profile defaults are no longer presented as measured results.

## 1.0.0-beta.1 — 2026-08-04

### Deutsch

- Erste konsolidierte öffentliche Beta unter Version 1.
- Vollständig lokale SML-/OBIS-Auswertung, Historie und professionelle Diagramme.
- Deutsch/Englisch-Umschaltung auf allen Webseiten, browserlokal gespeichert.
- Signierte OTA-Updates, Admin-Schutz, CSRF, Anmeldesperre und sichere Backups.
- Home Assistant/MQTT, JSON, CSV, Prometheus/OpenMetrics, Influx und Shelly-kompatible Leseendpunkte.
- Gestreamte Weboberflächen vermeiden große zusammenhängende RAM-Zuweisungen.
- Projektstruktur, Dokumentation und Release-Artefakte bereinigt.

### English

- First consolidated public beta under version 1.
- Fully local SML/OBIS processing, history and professional charts.
- German/English switch on every web page, stored locally in the browser.
- Signed OTA updates, admin protection, CSRF, login lockout and secure backups.
- Home Assistant/MQTT, JSON, CSV, Prometheus/OpenMetrics, Influx and read-only Shelly-compatible endpoints.
- Streamed web pages avoid large contiguous RAM allocations.
- Project structure, documentation and release artifacts cleaned up.
