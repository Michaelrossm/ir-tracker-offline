# Firmware-Architektur / Firmware architecture

## Deutsch

Die Firmware ist nach Verantwortlichkeiten unter `src/app/` gegliedert:

| Ordner | Inhalt |
|---|---|
| `core/` | Konfiguration, gemeinsame Hilfsfunktionen und Sicherheit |
| `meter/` | SML/D0-Empfang und IR-Steuerung |
| `network/` | WLAN/LAN-Fallback und MQTT |
| `update/` | signierte manuelle und GitHub-Updates |
| `web/` | Oberfläche, REST-/Messendpunkte, Historie und Shelly-Emulation |
| `diagnostics/` | Selbsttest, GPIO-Suche und Werksprüfung |

`main.cpp` enthält nur noch den gemeinsamen Zustand sowie `setup()` und
`loop()`. Die Module werden in festgelegter Reihenfolge in
dieselbe private C++-Übersetzungseinheit eingebunden. PlatformIO schließt ihre
separate Kompilierung ausdrücklich aus. Dadurch bleiben Binärverhalten,
Speicherlayout und die bestehenden internen Zustände bei dieser rein
strukturellen Aufteilung unverändert. Neue Protokolle können anschließend in
einem eigenen Modul ergänzt werden, ohne `main.cpp` wieder aufzublähen.

## English

Firmware responsibilities are organized below `src/app/`:

| Directory | Responsibility |
|---|---|
| `core/` | configuration, shared helpers and security |
| `meter/` | SML/D0 reception and IR control |
| `network/` | Wi-Fi/Ethernet fallback and MQTT |
| `update/` | signed manual and GitHub updates |
| `web/` | UI, REST/measurement endpoints, history and Shelly emulation |
| `diagnostics/` | self-test, GPIO scan and factory test |

`main.cpp` now retains only shared state, `setup()` and `loop()`. The modules
are included in a defined order into the same private C++
translation unit, while PlatformIO explicitly excludes standalone compilation.
This preserves binary behavior, memory layout and existing internal state for
this structural-only split. Future protocols can be added as dedicated modules
without growing `main.cpp` again.
