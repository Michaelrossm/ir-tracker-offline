# Kompatibilität – Speicher / Energiemanagement

Stand: 2026-08-30

Diese Liste enthält **nur Speicher- und Energiemanagementsysteme, die mit den aktuell vorhandenen Schnittstellen des IR Trackers grundsätzlich kompatibel sind oder aufgrund einer unterstützten Shelly-Schnittstelle als direkte Kandidaten geprüft werden können**.

Nicht kompatible Systeme werden hier bewusst nicht aufgeführt.

## Bedeutung der Spalten

- **Kompatibel**: Die benötigte Messgeräte-Schnittstelle passt grundsätzlich zu den aktuellen IR-Tracker-Schnittstellen.
- **Schnittstelle / Hinweis**: Vom Speicher/EMS unterstützte Messgeräte-Anbindung.
- **Getestet**: Das konkrete System wurde mit IR Tracker Offline auf echter Hardware erfolgreich geprüft.

`🟢 Ja` bedeutet technische Grundkompatibilität. `🟡 Kandidat` bedeutet, dass das System einen passenden Shelly-Zähler unterstützt, die dokumentierte Geräte-/App-/Account-Bindung aber noch praktisch mit der IR-Tracker-Emulation geprüft werden muss.

## Speicherliste

| # | Hersteller | Modell/Familie | Kompatibel | Schnittstelle / Hinweis | Getestet |
|---:|---|---|---|---|---|
| 1 | Solakon | ONE | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM; Kommunikation nach Einrichtung lokal im Heimnetz | ☐ |
| 2 | Growatt | NOAH 2000 | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM für intelligenten Eigenverbrauch | ☐ |
| 3 | Growatt | NEXA 2000 | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM als Lastmessung | ☐ |
| 4 | Anker SOLIX | Solarbank 2 E1600 Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM; dokumentierte Einrichtung über Shelly Cloud/Konto | ☐ |
| 5 | Anker SOLIX | Solarbank 2 E1600 Plus | 🟡 Kandidat | Shelly 3EM / Pro 3EM; Shelly-Cloud-Bindung | ☐ |
| 6 | Anker SOLIX | Solarbank 2 E1600 AC | 🟡 Kandidat | Shelly-Unterstützung; Geräte-/Cloud-Bindung praktisch prüfen | ☐ |
| 7 | Anker SOLIX | Solarbank 3 E2700 Pro | 🟡 Kandidat | Drittanbieter-Smart-Meter-Integration; lokale Clone-Kompatibilität noch nicht bestätigt | ☐ |
| 8 | Anker SOLIX | Solarbank 4 Pro | 🟡 Kandidat | Drittanbieter-Smart-Meter-Integration; lokale Clone-Kompatibilität noch nicht bestätigt | ☐ |
| 9 | EcoFlow | STREAM Ultra | 🟡 Kandidat | Shelly 3EM / Pro 3EM unterstützt; Geräte-/Account-Bindung praktisch prüfen | ☐ |
| 10 | EcoFlow | STREAM Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM unterstützt; Geräte-/Account-Bindung praktisch prüfen | ☐ |
| 11 | EcoFlow | STREAM Max | 🟡 Kandidat | STREAM-Smart-Meter-System unterstützt Shelly; Bindung praktisch prüfen | ☐ |
| 12 | EcoFlow | STREAM AC Pro | 🟡 Kandidat | STREAM-Smart-Meter-System unterstützt Shelly; Bindung praktisch prüfen | ☐ |
| 13 | Zendure | SolarFlow Hyper 2000 | 🟡 Kandidat | Shelly-Unterstützung über Zendure HEMS; Autorisierung/Bindung praktisch prüfen | ☐ |
| 14 | Zendure | SolarFlow 800 Pro | 🟡 Kandidat | Zendure HEMS/Shelly-Anbindung; lokale Clone-Kompatibilität prüfen | ☐ |
| 15 | Zendure | SolarFlow 1600 AC+ | 🟡 Kandidat | Shelly 3EM / Pro 3EM unterstützt | ☐ |
| 16 | Zendure | SolarFlow 2400 AC+ | 🟡 Kandidat | Shelly 3EM / Pro 3EM unterstützt | ☐ |
| 17 | Zendure | SolarFlow 2400 Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM unterstützt | ☐ |
| 18 | Hoymiles | MS-A2 | 🟡 Kandidat | Shelly Pro 3EM unterstützt; Aufnahme über S-Miles Home praktisch prüfen | ☐ |
| 19 | Hoymiles | HiBattery 1920 AC | 🟡 Kandidat | Shelly Pro 3EM / EcoTracker als Smart Meter unterstützt; Kopplungsweg prüfen | ☐ |
| 20 | Jackery | Navi 2000 | 🟡 Kandidat | Shelly Pro 3EM / Pro EM-50 unterstützt; App-Bindung praktisch prüfen | ☐ |

## Feldtest

Ein Haken in **Getestet** wird erst gesetzt, wenn das konkrete Speicher-/EMS-System den IR Tracker tatsächlich als Netzmessquelle übernimmt und Bezug, Einspeisung, Vorzeichen, Lastsprünge, Verbindungsverlust und Wiederanlauf korrekt verarbeitet.

Bei `🟡 Kandidat` ist die passende Zählerfamilie zwar vorhanden, aber eine Hersteller-App, ein Account oder eine Geräteautorisierung kann verhindern, dass eine reine lokale Shelly-Emulation akzeptiert wird. Erst nach erfolgreicher Prüfung wird der Eintrag auf `🟢 Ja` gesetzt.
