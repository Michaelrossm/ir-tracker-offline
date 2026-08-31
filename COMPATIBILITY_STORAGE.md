# Kompatibilität – Speicher / Energiemanagement

Stand: 2026-08-31

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
| 6 | Hoymiles | MS-A2 | 🟢 Ja | Shelly Pro 3EM wird offiziell unterstützt; Hoymiles fordert ausdrücklich denselben WLAN-Router und bietet in der Hoymiles-Home-App „Link Shelly“. Das spricht klar für lokale Shelly-Kommunikation. Discovery/Pairing bleibt noch ungetestet, deshalb kein Haken in „Getestet“. | ☐ |
| 7 | Hoymiles | HiBattery 1920 AC | 🟡 Kandidat | Shelly Pro 3EM und EcoTracker werden offiziell unterstützt. Reale Feldberichte zeigen: EcoTracker-Emulation funktioniert stabil; eine einfache Shelly-Pro-3EM-Emulation benötigte zusätzliche Anpassungen. Mit EcoTracker-Profil wäre eine grüne Freigabe sehr wahrscheinlich. | ☐ |
| 8 | Anker SOLIX | Solarbank 2 E1600 Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt. Ankers dokumentierter Setup-Weg wählt ausdrücklich **Shelly Cloud**, verlangt Shelly-Login und Gerätefreigabe. Daher nicht so einfach lokal wie bei Growatt/Hoymiles. | ☐ |
| 9 | Anker SOLIX | Solarbank 2 E1600 Plus | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt; dokumentierte Kopplung über **Shelly Cloud + Shelly-Konto**. Eine reine lokale Clone-Emulation ist damit nicht bestätigt. | ☐ |
| 10 | Anker SOLIX | Solarbank 2 E1600 AC | 🟡 Kandidat | Relevantes Solarbank-2-System; Shelly-Unterstützung ist produkt-/firmwareabhängig. Für dieses Modell fehlt noch ein belastbarer Nachweis einer rein lokalen Shelly-Kopplung. | ☐ |
| 11 | Anker SOLIX | Solarbank 3 E2700 Pro | 🟡 Kandidat | Anker bestätigt Shelly 3EM und Shelly Pro 3EM als kompatible Smart Meter. Da Anker die Shelly-Integration weiterhin über die Anker-App/Plattform verwaltet, ist lokale Clone-Kompatibilität noch nicht sicher. | ☐ |
| 12 | Anker SOLIX | Solarbank 4 Pro / E5000 Pro | 🟡 Kandidat | Relevante neue Solarbank-Familie. Shelly-Kompatibilität ist für die Produktfamilie relevant, aber es gibt noch keinen belastbaren Nachweis, dass ein lokaler Shelly-Klon ohne Anker-/Shelly-Cloud-Bindung akzeptiert wird. | ☐ |
| 13 | EcoFlow | STREAM Ultra | 🟢 Ja | Shelly 3EM / Pro 3EM offiziell unterstützt. STREAM unterstützt einen Local Mode, in dem System, Zubehör und lokal angebundene Messgeräte ausschließlich über das Heimnetz kommunizieren und keine Messdaten in die Cloud hochladen. Ersteinrichtung/Bindung bleibt erforderlich; IR-Tracker-Feldtest steht aus. | ☐ |
| 14 | EcoFlow | STREAM Pro | 🟢 Ja | Shelly 3EM / Pro 3EM offiziell unterstützt. Die STREAM-Serie unterstützt lokale Smart-Meter-Kommunikation und einen Offline-/Local-Mode. Die technische Grundkompatibilität zur lokalen Shelly-Emulation ist damit gegeben; Feldtest steht aus. | ☐ |
| 15 | EcoFlow | STREAM Max | 🟢 Ja | Shelly 3EM / Pro 3EM offiziell unterstützt. STREAM kann kompatible Messgeräte lokal im Heimnetz nutzen; die Account-Bindung bei der Einrichtung bedeutet nicht, dass die laufende Messwertübertragung zwingend über die Cloud erfolgt. | ☐ |
| 16 | EcoFlow | STREAM AC Pro | 🟢 Ja | Shelly 3EM / Pro 3EM offiziell unterstützt. EcoFlow dokumentiert für STREAM einen Local Mode mit ausschließlich lokaler LAN-Kommunikation zu unterstütztem Zubehör und Messgeräten. Damit technisch passend zur IR-Tracker-Shelly-Emulation; Feldtest steht aus. | ☐ |
| 17 | Zendure | SolarFlow Hyper 2000 | 🟢 Ja | Shelly Pro 3EM kann im **CT-Modus direkt lokal** mit dem Hyper kommunizieren, ohne HEMS und auch bei Internetausfall weiterregeln. Für Problemfälle ist aus der Praxis Shelly `RPC over UDP` zur Hyper-IP auf Port 8006 dokumentiert. Wichtig: In HEMS selbst bleibt der Hyper cloudgebunden; für die lokale Nutzung daher Hyper direkt im CT-Modus betreiben. | ☐ |
| 18 | Zendure | SolarFlow 800 Pro | 🟡 Kandidat | Shelly-Unterstützung vorhanden, aber die lokale Shelly-Regelung über Zendure-App/HEMS ist nicht so eindeutig und konsistent dokumentiert wie beim Hyper. Lokaler Betrieb über EcoTracker bzw. Home Assistant/ZenSDK ist aus der Praxis belegt; für die aktuelle IR-Tracker-Shelly-Emulation bleibt ein Feldtest nötig. | ☐ |
| 19 | Zendure | SolarFlow 1600 AC+ | 🟢 Ja | Offiziell unterstützt: Shelly 3EM, Shelly Pro 3EM und Everhome EcoTracker IR. Zendure bestätigt, dass bei einem unterstützten einzelnen SolarFlow-Gerät in HEMS die lokale Kommunikation mit CT/Zählerleser automatisch hergestellt werden kann; bei Internetausfall funktioniert die lokale Kommunikation im gemeinsamen WLAN weiter. | ☐ |
| 20 | Zendure | SolarFlow 2400 AC+ | 🟢 Ja | Offiziell unterstützt: Shelly 3EM, Shelly Pro 3EM und Everhome EcoTracker IR. Praxisberichte bestätigen lokale Shelly-Kommunikation und lokale Regelung im HEMS; Zendures neuere HEMS-Funktion stellt bei einem einzelnen unterstützten SolarFlow-Gerät die lokale Zählerkommunikation automatisch her. | ☐ |
| 21 | Zendure | SolarFlow 2400 Pro | 🟢 Ja | Offiziell unterstützt: Shelly 3EM, Shelly Pro 3EM und Everhome EcoTracker IR. Gehört zur aktuellen HEMS-2.0-Produktfamilie; lokale Zählerkommunikation für unterstützte Einzelgeräte ist vorgesehen. Die exakte lokale Regelung über alle Lade-/Entladezustände sollte im Feldtest noch bestätigt werden. | ☐ |
| 22 | Jackery | Navi 2000 | 🟢 Ja | Shelly Pro 3EM / Pro EM-50 werden offiziell unterstützt. Die Jackery-Dokumentation zeigt eine **direkte lokale Kopplung**: Smart Meter können über die Jackery Home App direkt mit dem Navi 2000 verbunden werden; das Navi stellt dafür ein eigenes WLAN bereit, mit dem sich Smart-CT/Smart-Meter verbinden. Ein Jackery-/Shelly-Cloud-Pfad ist damit für die laufende Messwertübertragung nicht zwingend. Die genaue Shelly-Discovery/Identität bleibt noch praktisch zu testen. | ☐ |

## Feldtest

Ein Haken in **Getestet** wird erst gesetzt, wenn das konkrete Speicher-/EMS-System den IR Tracker tatsächlich als Netzmessquelle übernimmt und Bezug, Einspeisung, Vorzeichen, Lastsprünge, Verbindungsverlust und Wiederanlauf korrekt verarbeitet.
