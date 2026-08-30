# IR Tracker Offline – Kompatibilität

Stand: 2026-08-30

Die Kompatibilität ist in drei Listen aufgeteilt:

1. [Stromzähler](COMPATIBILITY_METERS.md)
2. [Speicher / Energiemanagement](COMPATIBILITY_STORAGE.md)
3. [PV-Wechselrichter / Mikrowechselrichter](COMPATIBILITY_INVERTERS.md)

## Status

- ✅ **Getestet** – mit IR Tracker Offline auf echter Hardware erfolgreich geprüft.
- 🟢 **Grundsätzlich kompatibel / ungetestet** – die erforderliche lokale Schnittstelle bzw. das Protokoll ist mit dem IR Tracker technisch abbildbar; das konkrete Gerät wurde noch nicht praktisch getestet.
- 🟡 **Prüfung erforderlich** – technische Kompatibilität ist noch nicht ausreichend sicher geklärt.
- ❌ **Nicht kompatibel** – mit den derzeit implementierten Schnittstellen nicht direkt nutzbar.

Bei Speichern und Wechselrichtern wird `🟢 grundsätzlich kompatibel / ungetestet` nur verwendet, wenn eine passende **lokale** Messgeräte-/EMS-Schnittstelle vorhanden ist, die der IR Tracker mit seinen implementierten Schnittstellen nachbilden bzw. bedienen kann. Eine reine Hersteller-Cloud-Integration reicht dafür nicht.

## Firmware-Schnittstellen

Der IR Tracker unterstützt unter anderem SML, IEC 62056-21/D0, MQTT, HTTP/JSON sowie lokale Shelly-EM-/Shelly-Pro-EM-kompatible Leseendpunkte. Welche Messwerte verfügbar sind, hängt beim Stromzähler von dessen optischer Schnittstelle und Freischaltung ab.

Die Listen unterscheiden bewusst zwischen **technisch kompatibel** und **praktisch getestet**. Dadurch können geeignete Geräte bereits als grundsätzlich kompatibel gekennzeichnet werden, ohne einen noch nicht durchgeführten Feldtest vorzutäuschen.
