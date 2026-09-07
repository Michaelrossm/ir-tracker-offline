# IR Tracker Offline 1.3.7

## Deutsch

Dieses Release macht normale signierte Firmwareupdates robuster: Ein bereits
vorhandener, vollständig geprüfter älterer Webasset-Container bleibt nutzbar.
Dadurch startet der Tracker nach einem `.irfw`-Update weiterhin mit seiner
normalen Oberfläche, auch wenn das separate 64-kB-Asset-Image erst später
aktualisiert wird. Fehlende, manipulierte oder unvollständige benötigte Dateien
führen weiterhin sicher zur Recovery-Oberfläche.

Die Konfiguration aller lokalen Integrationen ist jetzt unter
„Schnittstellen“ zusammengefasst: API-Zugriff, Shelly-/EcoTracker-Kompatibilität,
Modbus TCP, MQTT/Home Assistant und optionaler dauerhafter Ereignisspeicher.
Alle Schnittstellen bleiben lokal und ausschließlich lesend; sie können keine
Lade-, Entlade- oder Sollwerte an Speicher und Wechselrichter senden.

LAN/PoE ist weiterhin nicht auf echter Hardware getestet.

## English

This release makes normal signed firmware updates more robust: an existing,
fully verified older web-asset container remains usable. The tracker therefore
continues with its normal UI after a `.irfw` update even when the separate
64-kB asset image is updated later. Missing, modified or incomplete required
files still safely enter the recovery UI.

All local integration settings are now grouped under “Interfaces”: API access,
Shelly/EcoTracker compatibility, Modbus TCP, MQTT/Home Assistant and optional
persistent event storage. Every interface remains local and read-only; none
can send charge, discharge or setpoint commands to a battery or inverter.

LAN/PoE has still not been tested on physical hardware.
