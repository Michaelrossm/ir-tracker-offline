# Änderungsprotokoll / Changelog

## Unveröffentlicht / Unreleased

## 1.3.5-beta.2 — 2026-09-03

### Deutsch

- Alle neun statischen HTML-/CSS-/JavaScript-Assets werden reproduzierbar
  minifiziert, gzip-komprimiert und in einem verifizierten Rohdatencontainer
  innerhalb der vorhandenen 64-kB-Partition abgelegt. Es gibt keine Änderung
  an Partitionstabelle, OTA-Slots oder History.
- Vollständige gzip-Doppelkopien wurden aus der App entfernt. Eine kleine,
  eigenständige Recovery-Seite ermöglicht weiterhin Status, Diagnose,
  signiertes Firmwareupdate, Asset-Sicherung/-Wiederherstellung und Neustart.
- Der Container prüft Magic, Schema, exakte Firmwareversion, vollständigen
  Dateisatz, Grenzen, Duplikate, Größen und SHA-256. Bei jedem Fehler bleiben
  Messung, History und Schnittstellen aktiv und die Recovery-Seite erreichbar.
- Offline-Flash sinkt gegenüber dem lokalen Ausgangsstand um 30.922 Byte; die
  OTA-Reserve wächst um 30.928 Byte. Das statische RAM bleibt unverändert.
- Auf echter ESP32-C3-Hardware wurden vollständiger Assetbetrieb, beschädigter
  Container, Recovery, Wiederherstellung, alle neun Assets und die vorhandenen
  WLAN-Funktions-/Sicherheitstests geprüft.

### English

- All nine static HTML, CSS and JavaScript assets are reproducibly minified,
  gzip-compressed and stored in a verified raw container within the existing
  64-kB partition. The partition table, OTA slots and history are unchanged.
- Full gzip duplicates were removed from the application. A small standalone
  recovery page retains status, diagnostics, signed firmware update, asset
  backup/restore and restart capabilities.
- The container validates magic, schema, exact firmware version, the complete
  file set, bounds, duplicates, sizes and SHA-256. Meter acquisition, history
  and interfaces remain active on every error while recovery stays reachable.
- Offline flash is reduced by 30,922 bytes compared with the local baseline;
  OTA reserve grows by 30,928 bytes. Static RAM is unchanged.
- Full asset operation, a damaged container, recovery, restoration, all nine
  assets and the existing Wi-Fi functional/security tests were verified on
  real ESP32-C3 hardware.

## 1.3.5-beta.1 — 2026-09-03

### Deutsch

- Der optionale Modbus-TCP-Dienst zählt Verbindungen sowie gültige und
  ungültige Requests und zeigt den letzten Client in Status-API, Diagnose und
  Supportbericht. Die Werte bleiben ausschließlich im RAM.
- Das JavaScript der Diagnoseansicht wird als eigenes, reproduzierbar
  minifiziertes und gzip-komprimiertes Webasset eingebettet. Dadurch sinkt der
  Flashbedarf, ohne die Bedienoberfläche funktional zu verändern.
- Der Lizenzhinweis für den Arduino Core for ESP32 wurde auf
  LGPL-2.1-or-later korrigiert und der zugehörige Lizenztext ergänzt.
- Die bestehende `DebugStorage`-Abstraktion prüft optionale Webassets jetzt über
  Manifest, exakte Firmwareversion, Dateigröße und SHA-256. Fehlende, falsche
  oder beschädigte Assets verwenden automatisch den eingebetteten Fallback.
- Der 64-kB-Prototyp lagert ausschließlich `maintenance.js.gz` aus. Alle neun
  Einzeldateien benötigen wegen LittleFS-Block- und Metadatenkosten mehr als
  64 kB; History und Partitionsgrößen wurden deshalb nicht verändert.

### English

- The optional Modbus TCP service counts connections, valid and invalid
  requests, and exposes the last client through the status API, diagnostics,
  and support report. These values remain in RAM only.
- The diagnostics JavaScript is embedded as a separate, reproducibly minified
  and gzip-compressed web asset, reducing flash use without changing UI
  behavior.
- The Arduino Core for ESP32 license notice was corrected to
  LGPL-2.1-or-later and the corresponding license text was added.
- The existing `DebugStorage` abstraction now verifies optional web assets by
  manifest, exact firmware version, file size and SHA-256. Missing,
  incompatible or damaged assets automatically use the embedded fallback.
- The 64-kB prototype externalizes `maintenance.js.gz` only. All nine separate
  files exceed 64 kB once LittleFS block and metadata overhead are included,
  so history and partition sizes remain unchanged.

## 1.3.3 — 2026-09-02

### Deutsch

- `/api/v1/meter` und MQTT `irtracker/<id>/meter` verwenden gemeinsam das
  stabile herstellerneutrale Schema `irtracker.meter.v1`. Optional steht ein
  standardmäßig ausgeschalteter, nur lesender Modbus-TCP-Dienst mit eigenem
  dokumentiertem IR-Tracker-Registerschema bereit.
- mDNS kündigt zusätzlich `_irtracker._tcp` und beim aktivierten Modbus-Dienst
  `_modbus._tcp` an. `_everhome._tcp` enthält die neutrale Produktkennung
  `IRT1000`; die Shelly-Protokoll-ID ist strikt vom frei änderbaren Hostnamen
  getrennt.
- EcoTracker-kompatible Antworten enthalten die dokumentierten Tariffelder
  `energyCounterInT1` und `energyCounterInT2` als `null`, solange der Zähler
  keine echten Tarifwerte liefert.

- Wartungs-, Setup- und zugehöriges JavaScript liegen als eigenständige,
  reproduzierbar gzip-komprimierte Webassets vor. Die C++-Handler erzeugen nur
  noch die kleine dynamische Gerätekonfiguration und verwenden einen
  gemeinsamen, gehärteten Asset-Antwortpfad.
- Der Eco-Modus verwendet ausschließlich 80 MHz im normalen Betrieb und
  160 MHz für zeitlich begrenzte Rechen-, Verbindungs- und Updateaufgaben. Die
  auf realer Hardware instabile 40-MHz-Stufe wurde vollständig entfernt.
- Optionales `WIFI_PS_MIN_MODEM` ist getrennt konfigurierbar und standardmäßig
  deaktiviert. Verbindungsaufbau, Wiederverbindung und Setup-Hotspot erzwingen
  weiterhin den sicheren Modus ohne Modem-Sleep.
- Die Historien-API wählt die feinste Stufe, die den gesamten angefragten
  Kalenderzeitraum abdeckt. Dadurch werden ältere Tage nicht mehr aus einer
  bereits teilweise überschriebenen Minutenstufe dargestellt, wenn ihre
  vollständige 15-Minuten-Historie noch vorhanden ist.
- Dashboard und Historienansicht verwenden eine gemeinsame Lückenerkennung,
  die zusätzlich fehlende Bereiche am Anfang und Ende markiert. Der zukünftige
  Teil des aktuellen Zeitraums gilt nicht als Ausfall.
- Eine zentrale, neutrale Geräteidentität verwendet ausschließlich
  `IRTRACKER-C3`, `IRTRACKER-C3-3EM`, `IRT-XXXXXX`, `irtracker-XXXXXX` und die
  echte ESP32-MAC. Fremde Produkt-, Serien- oder Herstellerkennungen werden
  nicht nachgebildet.
- mDNS kündigt HTTP sowie die kompatiblen Dienste `_shelly._tcp` und
  `_everhome._tcp` mit der aktiven WLAN- oder W5500-IP an und wird bei einem
  Schnittstellenwechsel kontrolliert neu gestartet.
- Ein eigener Speicher-Kompatibilitätsmodus öffnet nur die ausgewählten
  lokalen Leseendpunkte. Er ist bei Neuinstallationen standardmäßig aus;
  bestehende Installationen mit zuvor lokal offener API werden beim OTA
  rückwärtskompatibel übernommen. Schreib-, Wartungs-, OTA-, GPIO- und
  Diagnosewege bleiben geschützt.
- Die gemeinsame Einzeilenprüfung behandelt den normalen C-String-Abschluss
  nicht länger als eingebettetes Nullbyte. Einstellungen und Sicherungen mit
  gültigen WLAN-Daten lassen sich dadurch wieder zuverlässig speichern.
- EcoTracker-kompatible Antworten liefern das Messwertalter in Sekunden und
  stabile numerische/null-Felder. Shelly-kompatible EM-/EMData-Antworten wurden
  um neutrale Identität, Gesamtleistung und verfügbare 3-Phasen-Daten ergänzt.
- Auf echter ESP32-C3-Hardware per signiertem OTA geprüft: stabiler Übergang
  von 160 auf 80 MHz, frische SML-Daten ohne CRC- oder Taktfehler sowie
  vollständige Wiederanzeige eines älteren Kalendertags aus der 15-Minuten-
  Historie.

### English

- `/api/v1/meter` and MQTT `irtracker/<id>/meter` share the stable
  vendor-neutral `irtracker.meter.v1` schema. An optional, default-disabled,
  read-only Modbus TCP service exposes IR Tracker's own documented register map.
- mDNS additionally advertises `_irtracker._tcp` and, while Modbus is enabled,
  `_modbus._tcp`. `_everhome._tcp` contains the neutral product ID `IRT1000`;
  the Shelly protocol ID is strictly separated from the user-editable hostname.
- EcoTracker-compatible responses include the documented tariff fields
  `energyCounterInT1` and `energyCounterInT2` as `null` until genuine tariff
  readings are available.

- Maintenance, setup and related JavaScript are maintained as separate,
  reproducibly gzip-compressed Web assets. C++ handlers now generate only the
  small dynamic device configuration and use one shared hardened asset
  response path.
- Eco mode uses only 80 MHz during normal operation and 160 MHz for temporary
  compute, connection and update work. The 40 MHz stage that proved unstable
  on real hardware has been removed completely.
- Optional `WIFI_PS_MIN_MODEM` is configured independently and remains
  disabled by default. Association, reconnection and the setup hotspot always
  use the safe no-modem-sleep mode.
- The history API selects the finest tier that covers the complete requested
  calendar period. Older days are no longer rendered from a partially
  overwritten minute tier while complete quarter-hour history is available.
- Dashboard and history views share one gap detector that also marks missing
  ranges at the beginning and end. Future time in the current period is not
  treated as an outage.
- A central neutral identity uses only `IRTRACKER-C3`,
  `IRTRACKER-C3-3EM`, `IRT-XXXXXX`, `irtracker-XXXXXX`, and the genuine ESP32
  MAC. No third-party product, serial, or manufacturer identity is imitated.
- mDNS advertises HTTP and the compatible `_shelly._tcp` and
  `_everhome._tcp` services with the active Wi-Fi or W5500 address and is
  restarted cleanly when the preferred interface changes.
- A dedicated storage compatibility mode opens only the selected local read
  endpoints. It defaults to off on new installations; OTA upgrades preserve
  deliberately open legacy API installations. Write, maintenance, OTA, GPIO,
  and diagnostic paths remain protected.
- The shared single-line validator no longer mistakes the normal C-string
  terminator for an embedded null byte, so settings and backups containing
  valid Wi-Fi data can be saved reliably again.
- EcoTracker-compatible responses report reading age in seconds and stable
  numeric/null fields. Shelly-compatible EM/EMData responses now include the
  neutral identity, total power, and available three-phase readings.
- Validated by signed OTA on real ESP32-C3 hardware: stable transition from
  160 to 80 MHz, fresh SML readings without CRC or clock errors, and complete
  reconstruction of an older calendar day from quarter-hour history.

## 1.3.2 — 2026-09-01

### Deutsch

- `statusJson()` reserviert seinen Ausgabepuffer einmalig; der regelmäßige
  MQTT-Publishpfad verwendet feste Topic- und Zahlenpuffer. Home Assistant,
  Homie und bestehende MQTT-Topics bleiben kompatibel.
- Der Zähler-UART wird vor und nach synchronen Netzwerk-, MQTT- und Webarbeiten
  bedient. `HistoryStore::forEach()` liest den Ringpuffer in höchstens zwei
  zusammenhängenden Bereichen statt mit einem `seek()` je Datensatz.
- Der SML-Parser sammelt alle unterstützten OBIS-Werte in einem Durchlauf und
  vermeidet temporäre Zahlen-Vektoren im Hotpath.
- Ein Legacy-Vergleich schützt die Parserkompatibilität: 32 übereinstimmende
  Telegramme qualifizieren One-Pass; danach wird jedes 512. Telegramm erneut
  verglichen. Bei einer Abweichung bleibt der Legacy-Pfad bis zum Parser-Reset
  aktiv.
- SML-CRC- und D0-BCC-Ereignisse werden getrennt ausgewiesen. Der bisherige
  `crc_errors`-Wert bleibt als kompatibler Gesamtzähler erhalten.
- Der Auto-Modus pausiert nach einem frischen gültigen SML-Telegramm den
  D0-Parser beziehungsweise nach passivem D0 den SML-Parser. Nach dem
  vorhandenen 15-Sekunden-Stale-Timeout werden beide Parser automatisch wieder
  zur Erkennung freigegeben; aktive D0-Abfrage und UART-Recovery bleiben aktiv.
- Gemeinsame Zählerdiagnosen und normale/technische Supportberichte bewerten
  nur die Fehler des aktiven Protokolls. Frische vollständige Werte bleiben
  trotz alter kumulativer Ereignisse korrekt als stabil markiert.
- `/api/v1/raw` ist ausschließlich für angemeldete Administratoren verfügbar.
  Speicherdiagnose und technischer Bericht enthalten zusätzlich größten freien
  Heapblock, Stackreserve, Reset-Ursache und SML-Sicherheitsstatus.
- Release-Builds ohne ungenutzte C++-Exception-Pfade sparen gegenüber der
  lokalen Beta-3-Basis 20.616 Byte Flash, ohne RAM-, History-, EventLog- oder
  Pufferkapazitäten zu reduzieren.
- Auf echter ESP32-C3-Hardware per signiertem OTA geprüft: 104 aufeinander-
  folgende SML-Telegramme ohne CRC-, BCC- oder Parserfehler. Projekt-, HTTP-,
  Offline-, Factory- und Developer-Tests waren erfolgreich.

### English

- `statusJson()` reserves its output buffer once, while the recurring MQTT
  publish path uses fixed topic and numeric buffers. Home Assistant, Homie and
  existing MQTT topics remain compatible.
- The meter UART is serviced before and after synchronous network, MQTT and Web
  work. `HistoryStore::forEach()` reads the ring buffer in at most two
  contiguous ranges instead of performing one `seek()` per record.
- The SML parser collects all supported OBIS readings in one pass without
  temporary numeric vectors in the hot path.
- A legacy comparison protects parser compatibility: 32 matching telegrams
  qualify one-pass operation, followed by a comparison every 512 telegrams.
  Any mismatch latches the legacy path until the parser is reset.
- SML CRC and D0 BCC events are exposed separately. The existing `crc_errors`
  value remains available as a backward-compatible aggregate counter.
- In Auto mode, a fresh valid SML telegram pauses D0 parsing, while fresh
  passive D0 pauses SML parsing. The existing 15-second stale timeout releases
  both parsers for discovery; active D0 polling and UART recovery remain intact.
- Shared meter diagnostics and normal/technical support reports assess only
  errors belonging to the active protocol. Fresh complete readings remain
  correctly marked stable despite older cumulative events.
- `/api/v1/raw` is restricted to authenticated administrators. Memory
  diagnostics and the technical report additionally include largest free heap
  block, stack reserve, reset reason and SML safety state.
- Removing unused C++ exception paths saves 20,616 bytes of flash compared with
  the local Beta 3 baseline without reducing RAM, history, EventLog or buffer
  capacities.
- Validated by signed OTA on real ESP32-C3 hardware: 104 consecutive SML
  telegrams without CRC, BCC or parser errors. Project, HTTP, offline, factory
  and developer tests passed.

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
