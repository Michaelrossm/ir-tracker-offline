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
| 6 | Hoymiles | MS-A2 | 🟢 Ja | Shelly Pro 3EM wird offiziell unterstützt; Hoymiles fordert ausdrücklich denselben WLAN-Router und bietet in der Hoymiles-Home-App „Link Shelly“. Das spricht klar für lokale Shelly-Kommunikation. Discovery/Pairing bleibt noch ungetestet, deshalb kein Haken in „Getestet“. | ☐ |
| 7 | Hoymiles | HiBattery 1920 AC | 🟡 Kandidat | Shelly Pro 3EM und EcoTracker werden offiziell unterstützt. Reale Feldberichte zeigen: EcoTracker-Emulation funktioniert stabil; eine einfache Shelly-Pro-3EM-Emulation benötigte zusätzliche Anpassungen. Mit EcoTracker-Profil wäre eine grüne Freigabe sehr wahrscheinlich. | ☐ |
| 8 | Anker SOLIX | Solarbank 2 E1600 Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt. Ankers dokumentierter Setup-Weg wählt ausdrücklich **Shelly Cloud**, verlangt Shelly-Login und Gerätefreigabe. Daher nicht so einfach lokal wie bei Growatt/Hoymiles. | ☐ |
| 9 | Anker SOLIX | Solarbank 2 E1600 Plus | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt; dokumentierte Kopplung über **Shelly Cloud + Shelly-Konto**. Eine reine lokale Clone-Emulation ist damit nicht bestätigt. | ☐ |
| 10 | Anker SOLIX | Solarbank 2 E1600 AC | 🟡 Kandidat | Relevantes Solarbank-2-System; Shelly-Unterstützung ist produkt-/firmwareabhängig. Für dieses Modell fehlt noch ein belastbarer Nachweis einer rein lokalen Shelly-Kopplung. | ☐ |
| 11 | Anker SOLIX | Solarbank 3 E2700 Pro | 🟡 Kandidat | Anker bestätigt Shelly 3EM und Shelly Pro 3EM als kompatible Smart Meter. Da Anker die Shelly-Integration weiterhin über die Anker-App/Plattform verwaltet, ist lokale Clone-Kompatibilität noch nicht sicher. | ☐ |
| 12 | Anker SOLIX | Solarbank 4 Pro / E5000 Pro | 🟡 Kandidat | Relevante neue Solarbank-Familie. Shelly-Kompatibilität ist für die Produktfamilie relevant, aber es gibt noch keinen belastbaren Nachweis, dass ein lokaler Shelly-Klon ohne Anker-/Shelly-Cloud-Bindung akzeptiert wird. | ☐ |
| 13 | EcoFlow | STREAM Ultra | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt. EcoFlow verlangt ausdrücklich **dasselbe WLAN und dasselbe EcoFlow-Konto** wie beim STREAM-System. Damit ist die Kopplung nicht nur ein einfacher lokaler HTTP-Abruf. | ☐ |
| 14 | EcoFlow | STREAM Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt; laut Handbuch müssen Zähler und STREAM im selben WLAN liegen und an dasselbe EcoFlow-Konto gebunden sein. | ☐ |
| 15 | EcoFlow | STREAM Max | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt; auch hier fordert EcoFlow dasselbe WLAN und dasselbe EcoFlow-Konto. Clone-Betrieb ohne Account-Bindung nicht bestätigt. | ☐ |
| 16 | EcoFlow | STREAM AC Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM offiziell unterstützt; EcoFlow-Handbuch fordert gleiches WLAN und gleiche Account-Bindung. | ☐ |
| 17 | Zendure | SolarFlow Hyper 2000 | 🟡 Kandidat | Shelly-Integration vorhanden, aber Zendure hat für Hyper 2000 in HEMS keine echte lokale Kommunikation zugesagt. Community/Moderator-Hinweise bestätigen weiterhin Cloud-Abhängigkeit in HEMS. | ☐ |
| 18 | Zendure | SolarFlow 800 Pro | 🟡 Kandidat | Shelly-Unterstützung vorhanden. Lokale Shelly-Nutzung ist über Zendure-App/HEMS nicht durchgehend verfügbar; Community-Hinweise zeigen je nach Modus weiterhin Cloud-Abhängigkeit. | ☐ |
| 19 | Zendure | SolarFlow 1600 AC+ | 🟢 Ja | Offiziell unterstützt: Shelly 3EM, Shelly Pro 3EM, Everhome EcoTracker IR und HomeWizard P1. Neuere HEMS-Funktionen können bei einem einzelnen unterstützten SolarFlow-Gerät lokale Zählerkommunikation herstellen. Damit passt das Prinzip zur IR-Tracker-Shelly-Emulation. | ☐ |
| 20 | Zendure | SolarFlow 2400 AC+ | 🟢 Ja | Offiziell unterstützt: Shelly 3EM, Shelly Pro 3EM, Everhome EcoTracker IR und HomeWizard P1. Neuere HEMS-Architektur unterstützt lokale Zählerkommunikation; daher technisch sehr guter Kandidat für den IR Tracker. | ☐ |
| 21 | Zendure | SolarFlow 2400 Pro | 🟢 Ja | Offiziell unterstützt: Shelly 3EM, Shelly Pro 3EM und Everhome EcoTracker IR. HEMS-2.0-Produktfamilie mit lokaler Meter-Kommunikation; technisch passend, Feldtest mit IR Tracker steht aus. | ☐ |
| 22 | Jackery | Navi 2000 | 🟡 Kandidat | Shelly Pro 3EM / Pro EM-50 werden offiziell unterstützt. Jackery koppelt die Shelly-Geräte über die Jackery-Home-App; genaue lokale Discovery-/RPC-/MQTT-Anforderungen eines emulierten Shelly sind noch nicht ausreichend bestätigt. | ☐ |

## Besonders aussichtsreiche Kandidaten

Sehr stark sind aktuell die Hoymiles-Modelle **HiBattery 4020 AC**, **HiBattery 4020 X** und **MS-A2**. Bei MS-A2 nennt Hoymiles ausdrücklich denselben WLAN-Router und „Link Shelly“, weshalb die technische Grundkompatibilität jetzt auf `🟢 Ja` angehoben wurde.

Bei **Zendure SolarFlow 1600 AC+**, **2400 AC+** und **2400 Pro** ist die Lage ebenfalls besser als zunächst angenommen: Zendure unterstützt dort ausdrücklich Shelly und EcoTracker; neuere HEMS-Funktionen sehen lokale Zählerkommunikation vor. Diese Modelle sind deshalb jetzt ebenfalls `🟢 Ja`.

**Anker SOLIX** und **EcoFlow STREAM** bleiben dagegen gelb, obwohl Shelly offiziell unterstützt wird. Bei Anker ist der dokumentierte Weg ausdrücklich Shelly-Cloud + Shelly-Konto; EcoFlow verlangt eine Bindung an dasselbe EcoFlow-Konto. Das ist nicht dieselbe einfache lokale Shelly-Nutzung wie bei den grünen Systemen.

Bei **HiBattery 1920 AC** gibt es bereits einen wertvollen Praxisnachweis für EcoTracker-Emulation. Ein EcoTracker-kompatibler `/v1/json`-Endpunkt im IR Tracker wäre hier sehr wahrscheinlich der direkteste Weg von `🟡` zu `🟢`.

## Feldtest

Ein Haken in **Getestet** wird erst gesetzt, wenn das konkrete Speicher-/EMS-System den IR Tracker tatsächlich als Netzmessquelle übernimmt und Bezug, Einspeisung, Vorzeichen, Lastsprünge, Verbindungsverlust und Wiederanlauf korrekt verarbeitet.
