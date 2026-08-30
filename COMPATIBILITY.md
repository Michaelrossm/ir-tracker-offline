# IR Tracker Offline – Kompatibilität

Stand: 2026-08-30

Die Kompatibilität ist in zwei Listen aufgeteilt:

1. [Stromzähler](COMPATIBILITY_METERS.md)
2. [Speicher / Energiemanagement](COMPATIBILITY_STORAGE.md)

## Status

- ✅ **Getestet** – mit IR Tracker Offline auf echter Hardware erfolgreich geprüft.
- 🟢 **Kompatibel** – die erforderliche Schnittstelle bzw. das Protokoll ist mit dem IR Tracker technisch kompatibel; das konkrete Gerät kann trotzdem noch ungetestet sein.
- 🟡 **Kandidat** – das System unterstützt eine grundsätzlich passende Zählerfamilie, benötigt aber möglicherweise zusätzliche Geräte-/App-/Account-Bindung. Ein praktischer Test mit der IR-Tracker-Emulation steht noch aus.

Nicht kompatible Speicher-/EMS-Systeme werden in der öffentlichen Kompatibilitätsliste nicht aufgeführt.

## Firmware-Schnittstellen

Der IR Tracker unterstützt unter anderem SML, IEC 62056-21/D0, MQTT, HTTP/JSON sowie lokale Shelly-EM-/Shelly-Pro-EM-kompatible Leseendpunkte. Welche Messwerte verfügbar sind, hängt beim Stromzähler von dessen optischer Schnittstelle und Freischaltung ab.

Die Spalte **Getestet** unterscheidet technische Kompatibilität von einem bereits durchgeführten Feldtest.
