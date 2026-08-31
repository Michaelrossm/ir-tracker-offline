# Änderungsprotokoll / Changelog

## Unveröffentlicht / Unreleased

## 1.3.2 Beta 1 — 2026-08-31

### Deutsch

- Der gemeinsame Status-JSON reserviert seinen Ausgabepuffer einmalig und
  vermeidet wiederholte Heap-Vergrößerungen bei Web- und MQTT-Aufrufen.
- Der regelmäßige MQTT-Publishpfad verwendet feste, wiederverwendete Topic- und
  Zahlenpuffer statt zahlreicher temporärer `String`-Objekte. Home Assistant,
  Homie und alle bisherigen Topics bleiben kompatibel.
- Der bestehende Zähler-UART wird vor und nach möglicherweise blockierenden
  Netzwerk-, MQTT- und Webarbeiten bedient. Parser und `MeterData` bleiben die
  einzige gemeinsame Verarbeitungsschicht.
- `HistoryStore::forEach()` liest den Ringpuffer in höchstens zwei
  zusammenhängenden Bereichen. Reihenfolge, Plausibilitätsprüfung, Callback-API,
  doppelte Header und Stromausfallsicherheit bleiben unverändert.
- Lokaler OTA-Test auf ESP32-C3 sowie alle Projekt- und HTTP-Abnahmetests
  erfolgreich.

### English

- The shared status JSON reserves its output buffer once, avoiding repeated
  heap growth during Web and MQTT requests.
- The recurring MQTT publish path uses fixed reusable topic and numeric buffers
  instead of many temporary `String` objects. Home Assistant, Homie and all
  existing topics remain compatible.
- The existing meter UART is serviced before and after potentially blocking
  network, MQTT and Web work. The parsers and `MeterData` remain the single
  shared processing layer.
- `HistoryStore::forEach()` reads the ring buffer in at most two contiguous
  regions. Ordering, plausibility checks, callback API, duplicate headers and
  power-loss safety remain unchanged.
- Local OTA validation on ESP32-C3 and all project and HTTP acceptance tests
  passed.

## 1.3.1 — 2026-08-31

### Deutsch

- Alle lesenden Integrationen verwenden einen gemeinsamen Antwortpfad mit
  einheitlichem Zugriffsschutz, Cache-Verhalten und Versionsmetadaten.
- Der Modus „API deaktiviert“ sperrt Schnittstellen nun auch bei mitgesendeten
  Admin-Zugangsdaten zuverlässig.
- EcoTracker-Abfragen werten für den Minutenmittelwert nur noch das notwendige
  60-Sekunden-Fenster aus und lösen keinen CPU-Boost aus.
- Lange GPIO-Suchen halten den Leistungsmodus bis zum tatsächlichen Ende aktiv;
  danach kehrt der Eco-Modus automatisch in den 80-MHz-Betrieb zurück.
- Nur lesende Shelly-JSON-RPC-Anfragen sind größenbegrenzt und liefern
  einheitlich gehärtete Antworten.
- EcoTracker-kompatible API unter `/v1/json` mit Momentanleistung,
  1-Minuten-Mittelwert, Energiezählern, optionalen Phasenwerten und Datenalter.
- Shelly-Kompatibilität um Geräteerkennung, getrennte EM-/EMData-Antworten,
  dreiphasige Momentanwerte und nur lesende JSON-RPC-Aufrufe erweitert.
- Firmwarekomponenten vollständig unter `src/app/` einsortiert; SML und
  IEC 62056-21/D0 verwenden eine gemeinsame Parser-Schnittstelle und das
  zentrale `MeterData`-Messwertmodell.
- Gzip-komprimierte Browserassets werden reproduzierbar im Buildverzeichnis
  erzeugt und nicht mehr als generierter C++-Quellcode versioniert.
- Die GitHub-CI verwendet die aktuellen Node-24-basierten offiziellen Actions.
- Die optionale 64-kB-Debugpartition heißt bei neuen USB-Installationen nun
  `debugfs`. Dieselbe Firmware bevorzugt dieses Label und fällt bei bestehenden
  OTA-Geräten automatisch auf das alte Label `coredump` zurück; Offsets,
  OTA-Slots und Historie bleiben unverändert.
- `DebugStorage` trennt Partitionslabel und Dateipfade, mountet die
  Debugpartition unabhängig von der Historie und unterstützt weiterhin alte
  `/coredump/...`-Dateipfade neben dem neuen `/debug/...`-Pfad.
  Nur eine vollständig leere Partition wird initialisiert; nicht lesbare,
  nichtleere Bestandsdaten werden nicht automatisch formatiert.
- Release- und Werks-Builds entfernen Arduino-Core-Debugausgaben und
  unbenötigte Unwind-Tabellen. Einstellungsbackups werden als kompaktes JSON
  ausgegeben; gzip-Webassets, LTO und Feature-Buildflags bleiben aktiv.

### English

- All read-only integrations now use one response path with consistent access
  control, caching and version metadata.
- The "API disabled" mode now reliably blocks interfaces even when valid admin
  credentials are supplied.
- EcoTracker requests process only the required 60-second average window and
  do not trigger a CPU boost.
- Long GPIO scans keep performance mode active until they actually finish;
  Eco mode subsequently returns to 80 MHz automatically.
- Read-only Shelly JSON-RPC requests are size-limited and use the common
  hardened response path.
- Added a read-only EcoTracker-compatible `/v1/json` API with current power,
  one-minute average, energy counters, optional phase readings and data age.
- Extended Shelly compatibility with device discovery, separate EM/EMData
  responses, three-phase live values and read-only JSON-RPC calls.
- Moved all self-contained firmware components below `src/app/`.
- SML and IEC 62056-21/D0 now implement one parser interface and produce a
  central `MeterData` reading model.
- Gzip-compressed browser assets are generated only in the build directory;
  `src/WebAssets.h` is no longer versioned source.
- GitHub CI now uses the current official Node 24 based actions.
- New USB installations now call the optional 64-kB debug partition `debugfs`.
  The same firmware prefers that label and automatically falls back to the
  legacy `coredump` label on existing OTA devices; offsets, OTA slots and
  history remain unchanged.
- `DebugStorage` separates partition labels from file paths, mounts debug
  storage independently from history, and accepts legacy `/coredump/...`
  paths alongside the preferred `/debug/...` path.
  Only a completely blank partition is initialized; unreadable non-empty
  existing data is never formatted automatically.
- Release and factory builds remove Arduino core debug output and unused unwind
  tables. Settings backups use compact JSON while gzip assets, LTO and feature
  build flags remain enabled.

## 1.3.0 — 2026-08-30

### Deutsch

- Erste stabile Veröffentlichung ohne Beta-Kennzeichnung.
- Eine Universal-Firmware unterstützt die bestehende WLAN-Hardware und optional
  einen W5500 über SPI; bei LAN-Ausfall bleibt WLAN als Rückfallweg aktiv.
- Ältere Stromzähler werden zusätzlich über passives und aktives
  IEC 62056-21/D0 unterstützt. Die aktive Abfrage verwendet `/?!` und `ACK 000`.
- Bleiben gültige Zählertelegramme aus, werden UART und Parser kontrolliert neu
  initialisiert, ohne den ESP32 neu zu starten.
- Jeder veröffentlichte Messwert enthält sein Alter, damit Empfänger veraltete
  Daten sicher erkennen können.
- Ein separater Werksprüfungs-Build prüft ESP32-C3, Flash, RAM, Historie, WLAN,
  W5500/LAN, IR-Loopback, Status-LED und die bestätigte PoE-Versorgung.
- Eco-Modus gehärtet: WLAN-Aufbau, LAN-Fallback, Firmwareupdate, GPIO-Suche,
  Export/Import und Werksprüfung erhalten automatisch einen zeitlich begrenzten
  160-MHz-Boost; normaler Messbetrieb bleibt bei 80 MHz.
- Nicht benötigte schreibende Energieverwaltung vollständig entfernt;
  IR-Sniffer und IR-Bridge verbleiben ausschließlich im Entwickler-Build.
- Weboberfläche, Assets und Navigation komprimiert und konsolidiert; Diagnose
  befindet sich unter Wartung, JSON unter Einstellungen.
- Release-Build und Werksprüfungs-Build werden in CI getrennt geprüft.
- Quellcode ohne Funktionsänderung in klar getrennte Core-, Meter-, Netzwerk-,
  Update-, Web- und Diagnosemodule aufgeteilt; Projektdokumentation unter
  `docs/` zusammengeführt.

### English

- First stable release without a beta suffix.
- One universal firmware supports the existing Wi-Fi hardware and an optional
  SPI W5500; Wi-Fi remains available as fallback when Ethernet fails.
- Legacy electricity meters are additionally supported through passive and
  active IEC 62056-21/D0. Active polling uses `/?!` and `ACK 000`.
- UART and parser are reinitialized in a controlled manner when valid meter
  telegrams stop, without rebooting the ESP32.
- Every published measurement includes its age so consumers can reject stale
  data safely.
- A separate factory-test build verifies ESP32-C3, flash, RAM, history, Wi-Fi,
  W5500/Ethernet, IR loopback, status LED and confirmed PoE-only operation.
- Hardened Eco mode: Wi-Fi association, Ethernet fallback, firmware updates,
  GPIO scans, import/export and factory testing automatically receive a timed
  160 MHz boost; normal metering remains at 80 MHz.
- Removed obsolete writable energy management completely; the IR sniffer and
  writable IR bridge remain exclusive to the developer build.
- Compressed and consolidated the web UI, assets and navigation; diagnostics
  now lives under Maintenance and JSON under Settings.
- CI verifies the release and factory-test builds separately.
- Split source code into clear core, meter, network, update, web and diagnostic
  modules without changing behavior; consolidated project documentation under
  `docs/`.

## 1.0.2-beta.1 — 2026-08-09

### Deutsch

- Installer-kompatible, geschützte GPIO-Erkennung für RX und die automatische optische TX-Rückkopplungsprüfung ergänzt.
- App-only-WLAN-Update gehärtet: Bootloader, Partitionstabelle, Einstellungen und Historie bleiben unberührt.
- Status-API um eindeutige Installer-Fähigkeiten und Diagnosewerte erweitert.
- Weboberfläche, mDNS, Speicherverwendung sowie deutsch/englische Texte und Dokumentation konsolidiert.

### English

- Added installer-compatible protected GPIO detection for RX and automatic optical TX loopback verification.
- Hardened app-only Wi-Fi updates: bootloader, partition table, settings, and history remain untouched.
- Extended the status API with explicit installer capabilities and diagnostic values.
- Consolidated the web UI, mDNS, memory use, bilingual text, and documentation.

## 1.0.1-beta.2 — 2026-08-06

- DE: Hauptnavigation auf fünf Bereiche reduziert; Diagnose ist nun als eigene
  Unteransicht in Wartung integriert und technische Zählerfunktionen sind
  übersichtlich einklappbar.
- EN: Reduced the main navigation to five sections; diagnostics is now a
  maintenance subview and technical meter functions are organized in
  collapsible sections.
- DE: JSON-/REST-Zugang aus der Hauptnavigation entfernt und als
  Expertenfunktion unter Einstellungen eingeordnet. Sichtbare Zählertexte und
  allgemeine Metadaten sind markenneutral.
- EN: Removed JSON/REST access from the main navigation and placed it under
  Settings as an expert feature. Visible meter text and generic metadata are
  vendor-neutral.
- DE: Gemeinsames Sprachpaket gzip-komprimiert, versionsabhängig im Browser
  zwischengespeichert und reproduzierbar für Windows/Linux eingebettet.
- EN: Gzip-compressed the shared language bundle, added versioned browser
  caching, and made embedding reproducible on Windows and Linux.

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
