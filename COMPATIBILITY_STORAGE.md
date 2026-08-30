# Kompatibilität – Speicher / Energiemanagement

Stand: 2026-08-30

Diese Liste enthält nur Systeme, bei denen nach genauerer Prüfung ein realistischer Weg mit den **aktuell vorhandenen IR-Tracker-Schnittstellen** besteht.

Nicht kompatible oder nur über Hersteller-/Shelly-Cloud autorisierbare Systeme werden hier nicht aufgeführt.

## Bedeutung der Spalten

- **Kompatibel**: Die Messgeräte-Schnittstelle passt grundsätzlich zur aktuellen IR-Tracker-Firmware.
- **Schnittstelle / Hinweis**: Warum das System als kompatibel bzw. als enger Kandidat gilt.
- **Getestet**: Das konkrete System wurde mit IR Tracker Offline auf echter Hardware erfolgreich geprüft.

Status:
- 🟢 **Ja** – nach aktueller Dokumentation ist eine lokale Anbindung ohne zwingende Shelly-Cloud-Identität realistisch.
- 🟡 **enger Kandidat** – lokale Fremdzähler-Anbindung ist dokumentiert, aber Discovery/Handshake bzw. das genaue MQTT-/Shelly-Profil muss noch gegen die IR-Tracker-Implementierung geprüft werden.

> Der IR Tracker stellt aktuell HTTP/JSON, MQTT sowie lokale Shelly-EM-/Shelly-Pro-EM-kompatible Leseendpunkte bereit. Er sendet derzeit keine herstellerspezifischen Shelly-Skripte, keine Shelly-Cloud-Identität und keine speziellen RPC-over-UDP-Push-Telegramme an Speicher.

## Speicherliste

| # | Hersteller | Modell/Familie | Kompatibel | Schnittstelle / Hinweis | Getestet |
|---:|---|---|---|---|---|
| 1 | Solakon | ONE | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM; lokale Kommunikation im selben Heimnetz ist dokumentiert. Die Shelly-IP kann bei Bedarf manuell eingetragen werden. Damit passt das Prinzip sehr gut zur lokalen IR-Tracker-Emulation. | ☐ |
| 2 | Growatt | NOAH 2000 | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM werden offiziell unterstützt. Growatt dokumentiert die Einbindung in ShinePhone; lokale bzw. direkte Shelly-Nutzung ist für NOAH/NEXA vorgesehen. Praktischer IR-Tracker-Test steht noch aus. | ☐ |
| 3 | Growatt | NEXA 2000 | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM; reale Nutzerberichte bestätigen lokale Einbindung im selben WLAN ohne zwingende Kopplung des kompletten Shelly-Kontos. Sehr guter Kandidat für die IR-Tracker-Shelly-Emulation. | ☐ |
| 4 | Hoymiles | MS-A2 | 🟡 enger Kandidat | Shelly Pro 3EM wird offiziell unterstützt. Hoymiles verlangt, dass MS-A2 und Shelly im selben WLAN liegen und der Shelly über „Link Shelly“ hinzugefügt wird. Das spricht für lokale Kommunikation; offen ist, welche Shelly-Discovery-/Identitäts-Endpunkte beim Hinzufügen zwingend erwartet werden. | ☐ |
| 5 | Hoymiles | HiBattery 1920 AC | 🟡 enger Kandidat | Hoymiles wirbt mit Drittanbieter-Smart-Metern und offener MQTT-Konnektivität für Null-Einspeisung. Für eine direkte Freigabe muss noch das erwartete MQTT-/Smart-Meter-Datenprofil gegen die IR-Tracker-MQTT-Ausgabe geprüft werden. | ☐ |

## Warum andere zuvor aufgeführte Systeme entfernt wurden

Bei der tieferen Prüfung zeigte sich, dass mehrere Hersteller zwar `Shelly 3EM / Pro 3EM` unterstützen, die Kopplung aber nicht nur durch Lesen der lokalen Shelly-HTTP-Endpunkte erfolgt:

- **Anker SOLIX**: die dokumentierte Drittanbieter-Einrichtung läuft über `Shelly Cloud` und Shelly-Konto. Ein lokaler Shelly-Klon ohne echte Cloud-Identität kann damit derzeit nicht sauber hinzugefügt werden.
- **EcoFlow STREAM**: Shelly 3EM/Pro 3EM werden unterstützt, die praktische Kopplung läuft jedoch über die EcoFlow-/Shelly-Account-Integration; Nutzerberichte zeigen Cloud-Abhängigkeit der Meter-Verbindung. Deshalb derzeit keine sichere IR-Tracker-Kompatibilität.
- **Zendure SolarFlow**: Shelly wird unterstützt, aber die Einrichtung verwendet Shelly-Autorisierung/HEMS. Für echte lokale Regelung werden bei mehreren Geräten zusätzlich Shelly-RPC-Verbindungen zum Speicher verwendet. Diese Push-Funktion bildet der IR Tracker derzeit nicht nach.
- **Jackery**: neuere lokale Kopplungen konfigurieren am echten Shelly unter anderem MQTT sowie `RPC over UDP` zum Jackery-Gerät. Diese ausgehenden Shelly-Funktionen emuliert der IR Tracker derzeit nicht.

Diese Systeme können später wieder aufgenommen werden, wenn die jeweils benötigten Profile in der Firmware ergänzt wurden.

## Feldtest

Ein Haken in **Getestet** wird erst gesetzt, wenn das konkrete Speicher-/EMS-System den IR Tracker tatsächlich als Netzmessquelle übernimmt und Bezug, Einspeisung, Vorzeichen, Lastsprünge, Verbindungsverlust und Wiederanlauf korrekt verarbeitet.
