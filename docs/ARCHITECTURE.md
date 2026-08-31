# Firmware-Architektur / Firmware architecture

## Deutsch

Die Firmware ist nach Verantwortlichkeiten unter `src/app/` gegliedert:

| Ordner | Inhalt |
|---|---|
| `core/` | Konfiguration, Ereignisprotokoll, Hilfsfunktionen und Sicherheit |
| `hardware/` | verbindliches Hardwareprofil und reservierte GPIOs |
| `meter/` | gemeinsames Messwertmodell, SML-/D0-Parser und IR-Steuerung |
| `network/` | WLAN/LAN-Fallback und MQTT |
| `storage/` | kompakte, mehrstufige lokale Historie |
| `update/` | signierte manuelle und GitHub-Updates |
| `web/` | Oberfläche, REST-/Messendpunkte, Historie und Shelly-Emulation |
| `diagnostics/` | Selbsttest, GPIO-Suche und Werksprüfung |

`main.cpp` enthält nur noch gemeinsamen Anwendungszustand sowie `setup()` und
`loop()`. Zustandsnahe Module werden in festgelegter Reihenfolge in dieselbe
private C++-Übersetzungseinheit eingebunden. PlatformIO schließt deren separate
Kompilierung ausdrücklich aus.

Eigenständige Klassen wie `EthernetManager`, `EventLog` und `HistoryStore`
werden als getrennte Übersetzungseinheiten kompiliert. `SmlParser` und
`D0Parser` bleiben getrennte Quelldateien, werden für den kleinen OTA-Slot aber
zusammen mit `main.cpp` übersetzt. Beide implementieren `MeterParser` und
liefern das zentrale `MeterData`-Modell. Link-Time Optimization (LTO) fasst
gemeinsamen Code über Modulgrenzen hinweg zusammen. Dadurch gelten Vorzeichen,
Einheiten, Plausibilitätsprüfung und Aktualitätszeiten für WebUI, MQTT,
Shelly, EcoTracker, Prometheus, Influx und Historie einheitlich. Lesende
HTTP-Integrationen laufen zusätzlich über `web/IntegrationApi.cpp`, das
Zugriffsschutz, Cache-Regeln und Versionsmetadaten zentral setzt. Normale
Messwertabfragen bleiben im Eco-Takt; nur ausdrücklich rechenintensive und
laufende Wartungsaufgaben verlängern den zeitlich begrenzten CPU-Boost.

Webquellen werden ausschließlich unter `web/` gepflegt. Der reproduzierbare,
Gzip-komprimierte C++-Header entsteht bei jedem Build im jeweiligen
PlatformIO-Buildverzeichnis und wird nicht versioniert.

## English

Firmware responsibilities are organized below `src/app/`:

| Directory | Responsibility |
|---|---|
| `core/` | configuration, event log, shared helpers and security |
| `hardware/` | authoritative hardware profile and reserved GPIOs |
| `meter/` | shared reading model, SML/D0 parsers and IR control |
| `network/` | Wi-Fi/Ethernet fallback and MQTT |
| `storage/` | compact multi-tier local history |
| `update/` | signed manual and GitHub updates |
| `web/` | UI, REST/measurement endpoints, history and Shelly emulation |
| `diagnostics/` | self-test, GPIO scan and factory test |

`main.cpp` retains only shared application state, `setup()` and `loop()`.
State-coupled modules are included in a defined order into the same private C++
translation unit, while PlatformIO explicitly excludes their standalone
compilation.

Self-contained classes such as `EthernetManager`, `EventLog` and `HistoryStore`
compile as separate translation units. `SmlParser` and `D0Parser` remain
separate source files but compile together with `main.cpp` for the small OTA
slot. Both implement `MeterParser` and produce the central `MeterData` model.
Link-time optimization (LTO) folds common code across module boundaries. This
keeps signs, units, plausibility checks and freshness timestamps consistent
across the Web UI, MQTT, Shelly, EcoTracker, Prometheus, Influx and history.
Read-only HTTP integrations additionally pass through
`web/IntegrationApi.cpp`, which centrally applies access control, cache rules
and version metadata. Normal reading requests stay at the Eco clock; only
explicitly compute-intensive, active maintenance tasks extend the temporary
CPU boost.

Browser sources are maintained exclusively below `web/`. Every build creates
the reproducible Gzip-compressed C++ header in its own PlatformIO build
directory; generated code is not versioned.
