# IR Tracker Offline – Kompatibilität

Stand: 2026-09-16 · Firmware 2.0.0

Die Kompatibilität ist in zwei Listen aufgeteilt:

1. [Stromzähler](METERS.md)
2. [Speicher / Energiemanagement](STORAGE.md)

## Status

- ✅ **Getestet** – das konkrete Gerät wurde mit IR Tracker Offline auf echter Hardware erfolgreich als Gesamtsystem geprüft. Das ist der stärkste Status in diesen Listen.
- 🟢 **Kompatibel** – grundsätzlich technisch kompatibel: IR Tracker stellt die benötigte lokale Schnittstelle bzw. das benötigte Protokoll bereit und es gibt belastbare Hinweise, Herstellerangaben oder Fremdgeräte-/Emulationstests, dass der Integrationsweg funktioniert. Das konkrete Modell muss noch nicht mit IR Tracker selbst getestet worden sein.
- 🟡 **Kandidat** – eine grundsätzlich passende Schnittstelle ist vorhanden, aber Discovery, Onboarding, Cloud-/Accountbindung, Geräteidentität oder Stabilität sind noch nicht ausreichend geklärt.
- ⬜ **Nicht getestet** – kennzeichnet in den Tabellen, dass noch kein realer Feldtest dieses konkreten Geräts mit IR Tracker durchgeführt wurde.

**Wichtig:** „Kompatibel“ und „Getestet“ sind bewusst zwei verschiedene Aussagen. Grün bedeutet technische bzw. grundsätzlich nachgewiesene Kompatibilität; erst ✅ kennzeichnet einen erfolgreichen IR-Tracker-Feldtest. Hersteller-Firmware und Apps können sich später ändern, daher ist auch ein Feldtest immer auf den getesteten Software-/Firmwarestand bezogen.

Nicht kompatible Speicher-/EMS-Systeme werden in der öffentlichen Kompatibilitätsliste nicht aufgeführt.

## Firmware-Schnittstellen

Firmware 2.0.0 unterstützt unter anderem SML, IEC 62056-21/D0, MQTT, HTTP/JSON, die neutrale `/api/v1/meter`-Schnittstelle sowie lokale read-only Shelly-EM-/Shelly-Pro-EM- und EcoTracker-kompatible Endpunkte. Optional steht ein read-only Modbus-TCP-Registerschema zur Verfügung. Welche Messwerte verfügbar sind, hängt beim Stromzähler von dessen optischer Schnittstelle und Freischaltung ab.
