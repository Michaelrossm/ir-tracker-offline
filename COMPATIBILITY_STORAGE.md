# Kompatibilität – Speicher / Energiemanagement

Stand: 2026-08-30

Diese Liste enthält relevante Speicher- und Energiemanagementsysteme, bei denen ein Einsatz mit dem IR Tracker technisch sinnvoll ist. Systeme bleiben auch dann als Kandidat enthalten, wenn für die aktuelle Firmware noch einzelne Discovery-, Authentifizierungs- oder Push-Funktionen ergänzt werden müssten.

## Bedeutung der Spalten

- **Kompatibel**: Bewertung gegen die aktuell vorhandenen IR-Tracker-Schnittstellen.
- **Schnittstelle / Hinweis**: Erwartete Smart-Meter-Anbindung und aktueller Kenntnisstand.
- **Getestet**: Das konkrete System wurde mit IR Tracker Offline auf echter Hardware erfolgreich geprüft.

Status:
- 🟢 **Ja** – lokale Fremdzähler-Anbindung ist dokumentiert und passt grundsätzlich zu den aktuellen IR-Tracker-Funktionen.
- 🟡 **Kandidat** – das System unterstützt relevante Smart Meter wie Shelly/EcoTracker, aber Discovery, Account-Bindung oder Zusatzfunktionen müssen noch praktisch bzw. protokollseitig geklärt werden.

> Der IR Tracker stellt aktuell HTTP/JSON, MQTT sowie lokale Shelly-EM-/Shelly-Pro-EM-kompatible Leseendpunkte bereit. Eine EcoTracker-spezifische Emulation ist derzeit noch nicht implementiert.

## Speicherliste

| # | Hersteller | Modell/Familie | Kompatibel | Schnittstelle / Hinweis | Getestet |
|---:|---|---|---|---|---|
| 1 | Solakon | ONE | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM; lokale Kommunikation im Heimnetz ist vorgesehen. Sehr guter Kandidat für die aktuelle Shelly-Emulation. | ☐ |
| 2 | Growatt | NOAH 2000 | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM werden für intelligenten Eigenverbrauch unterstützt. Praktischer IR-Tracker-Test steht noch aus. | ☐ |
| 3 | Growatt | NEXA 2000 | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM als Lastmessung. Direkte lokale Nutzung im selben WLAN ist realistisch. | ☐ |
| 4 | Hoymiles | HiBattery 4020 AC | 🟢 Ja | Hoymiles dokumentiert ausdrücklich Shelly, EcoTracker und Linky **über lokales LAN** für die Null-Einspeisungsregelung. Zusätzlich Open MQTT; Cloud kann nach der Einrichtung deaktiviert werden. | ☐ |
| 5 | Hoymiles | HiBattery 4020 X | 🟢 Ja | Wie 4020 AC: Drittanbieter-Zähler wie Shelly, EcoTracker und Linky über **lokales LAN**, plus Open MQTT. Sehr starker Kandidat für den IR Tracker. | ☐ |
| 6 | Hoymiles | MS-A2 | 🟡 Kandidat | Shelly Pro 3EM offiziell unterstützt; Speicher und Shelly müssen am selben WLAN hängen. „Link Shelly“ spricht für lokale Kommunikation, aber Discovery/Identitätsprüfung muss noch gegen die IR-Tracker-Emulation getestet werden. | ☐ |
| 7 | Hoymiles | HiBattery 1920 AC | 🟡 Kandidat | Drittanbieter-Smart-Meter und Open MQTT offiziell unterstützt. Reale Feldberichte zeigen: EcoTracker-Emulation funktioniert stabil; einfache Shelly-Pro-3EM-Emulation benötigt zusätzliche Anpassungen. Mit EcoTracker-Profil wäre die Freigabe sehr wahrscheinlich. | ☐ |
| 8 | Anker SOLIX | Solarbank 2 E1600 Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt. Ankers dokumentierter Setup-Weg nutzt derzeit Shelly Cloud + Shelly-Konto; daher noch nicht direkt grün für eine reine lokale Emulation. | ☐ |
| 9 | Anker SOLIX | Solarbank 2 E1600 Plus | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt; dokumentierte Kopplung über Shelly Cloud/Konto. | ☐ |
| 10 | Anker SOLIX | Solarbank 2 E1600 AC | 🟡 Kandidat | Shelly-Unterstützung ist vorgesehen/produktabhängig verfügbar; genaue lokale Kopplung noch nicht ausreichend bestätigt. | ☐ |
| 11 | Anker SOLIX | Solarbank 3 E2700 Pro | 🟡 Kandidat | Drittanbieter-Smart-Meter-Integration vorhanden; für die aktuelle IR-Tracker-Shelly-Emulation fehlt noch ein belastbarer lokaler Kopplungsnachweis. | ☐ |
| 12 | Anker SOLIX | Solarbank 4 Pro / E5000 Pro | 🟡 Kandidat | Relevante neue Solarbank-Familie; Drittanbieter-Meter-Unterstützung vorhanden bzw. vorgesehen, lokale Clone-Kompatibilität noch offen. | ☐ |
| 13 | EcoFlow | STREAM Ultra | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt. Handbuch verlangt derzeit gleiches WLAN **und dasselbe EcoFlow-Konto**; deshalb ist reine lokale Shelly-Emulation noch nicht bestätigt. | ☐ |
| 14 | EcoFlow | STREAM Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt; gleicher EcoFlow-Account wird im Handbuch gefordert. | ☐ |
| 15 | EcoFlow | STREAM Max | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt; Account-/Bindungsmechanismus muss für Clone-Betrieb geprüft werden. | ☐ |
| 16 | EcoFlow | STREAM AC Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt; Handbuch fordert gleiches WLAN und gleiches EcoFlow-Konto. | ☐ |
| 17 | Zendure | SolarFlow Hyper 2000 | 🟡 Kandidat | Shelly-Integration vorhanden. Für ältere HEMS-/App-Versionen läuft die Kopplung über Zendure/Shelly-Autorisierung; lokale Clone-Kompatibilität noch nicht sicher. | ☐ |
| 18 | Zendure | SolarFlow 800 Pro | 🟡 Kandidat | Shelly-Unterstützung vorhanden. Aktuelle Community-Hinweise zeigen lokale Kommunikation ist mit neueren HEMS-Funktionen möglich, aber das genaue Enrollment eines Shelly-Klons muss geprüft werden. | ☐ |
| 19 | Zendure | SolarFlow 1600 AC+ | 🟡 Kandidat | Shelly 3EM / Pro 3EM unterstützt; für eine grüne Freigabe fehlt noch der Nachweis, dass ein emulierter Shelly ohne Hersteller-Cloud-Identität eingebunden werden kann. | ☐ |
| 20 | Zendure | SolarFlow 2400 AC+ | 🟡 Kandidat | Shelly Pro 3EM, Shelly 3EM, EcoTracker IR und HomeWizard P1 werden offiziell unterstützt. Neuere HEMS-Versionen können lokale Meter-Kommunikation nutzen; Enrollment/Discovery des IR Trackers noch prüfen. | ☐ |
| 21 | Zendure | SolarFlow 2400 Pro | 🟡 Kandidat | Shelly-Unterstützung vorhanden; lokale Kommunikation ist grundsätzlich Teil der neueren Zendure-HEMS-Architektur, genaue Clone-Kopplung bleibt zu testen. | ☐ |
| 22 | Jackery | Navi 2000 | 🟡 Kandidat | Shelly Pro 3EM / Pro EM-50 werden unterstützt. Jackery erlaubt direkte Shelly-Einbindung in der eigenen App, aber echte Shellys werden dabei teilweise mit MQTT/RPC-Funktionen konfiguriert. Diese Zusatzfunktionen emuliert der IR Tracker derzeit noch nicht vollständig. | ☐ |

## Besonders aussichtsreiche Kandidaten

Am stärksten sind aktuell die Hoymiles-Modelle **HiBattery 4020 AC** und **HiBattery 4020 X**, weil Hoymiles explizit Drittanbieter-Zähler wie Shelly und EcoTracker über **lokales LAN** nennt und zusätzlich Open MQTT anbietet. Diese beiden sind deshalb auf `🟢 Ja` gesetzt.

Bei **HiBattery 1920 AC** gibt es bereits einen sehr wertvollen Feldhinweis aus der Praxis: eine EcoTracker-Emulation auf einem ESP-Lesekopf wurde erfolgreich mit der Batterie gekoppelt und lief stabil, während eine einfache Shelly-Pro-3EM-Emulation ohne zusätzliche Anpassungen nicht funktionierte. Für den IR Tracker wäre deshalb eine EcoTracker-kompatible Emulation wahrscheinlich der direkteste Weg von `🟡` zu `🟢`.

## Feldtest

Ein Haken in **Getestet** wird erst gesetzt, wenn das konkrete Speicher-/EMS-System den IR Tracker tatsächlich als Netzmessquelle übernimmt und Bezug, Einspeisung, Vorzeichen, Lastsprünge, Verbindungsverlust und Wiederanlauf korrekt verarbeitet.
