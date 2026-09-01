# Kompatibilität – Batteriespeicher & Energiemanagement

**Stand: 01.09.2026 · Firmware 1.3.2**

Diese Übersicht zeigt Batteriespeicher und Energiemanagementsysteme, die grundsätzlich mit **IR Tracker Offline** als externe Netzmessquelle verwendet werden können.

Die Bewertung basiert auf den aktuell implementierten lokalen Schnittstellen des IR Trackers sowie auf öffentlich dokumentierten Integrationsmöglichkeiten der jeweiligen Hersteller und etablierten Smart-Meter-Anbieter.

## Statusdefinition

| Status | Bedeutung |
|---|---|
| 🟢 **Kompatibel** | Die vom System benötigte Zähler-Schnittstelle wird vom IR Tracker grundsätzlich bereitgestellt. Ein abschließender Hardware-Feldtest kann trotzdem noch ausstehen. |
| 🟡 **Kandidat** | Eine grundsätzlich passende Fremdzähler-Unterstützung ist vorhanden, jedoch bestehen noch offene Punkte bei Cloud-Bindung, Discovery, Authentifizierung oder dem verwendeten Protokoll. |
| ✅ **Getestet** | Das konkrete Speicher-/EMS-System wurde mit IR Tracker Offline auf realer Hardware erfolgreich geprüft. |
| ⬜ **Nicht getestet** | Technische Kompatibilität ist gegeben oder wahrscheinlich, ein realer Feldtest steht jedoch noch aus. |

## Vom IR Tracker bereitgestellte Schnittstellen

Firmware **1.3.2** stellt unter anderem folgende lokale Integrationen bereit:

- Shelly-EM-kompatible Endpunkte
- Shelly-Pro-EM-/Pro-3EM-kompatible Endpunkte
- Shelly-Geräteerkennung und nur lesende JSON-RPC-Zugriffe
- EcoTracker-kompatible API unter `/v1/json`
- HTTP / JSON
- MQTT
- Home Assistant MQTT Discovery
- Prometheus / OpenMetrics
- Influx Line Protocol

> **Wichtig:** „Kompatibel“ bedeutet nicht automatisch „auf realer Hardware getestet“. Erst ein erfolgreicher Feldtest erhält den Status ✅.

## Kompatibilitätsübersicht

| # | Hersteller | Modell / Familie | Status | Bevorzugte Schnittstelle | Bewertung | Feldtest |
|---:|---|---|---|---|---|:---:|
| 1 | Solakon | ONE | 🟢 Kompatibel | Shelly 3EM / Pro 3EM / EcoTracker | Lokale Fremdzähler-Anbindung passt grundsätzlich zu den aktuellen IR-Tracker-Profilen. | ⬜ |
| 2 | Growatt | NOAH 2000 | 🟢 Kompatibel | Shelly / EcoTracker | Unterstützt externe Smart Meter für die Eigenverbrauchsregelung. | ⬜ |
| 3 | Growatt | NEXA 2000 | 🟢 Kompatibel | Shelly / EcoTracker | Lokale Fremdzähler-Anbindung technisch passend. | ⬜ |
| 4 | Growatt | Aura 5000 | 🟢 Kompatibel | EcoTracker | Von Everhome als EcoTracker-kompatibles Energiesystem geführt. | ⬜ |
| 5 | Hoymiles | HiBattery 4020 AC | 🟢 Kompatibel | Shelly / EcoTracker | Drittanbieter-Zähler werden ausdrücklich über lokales LAN unterstützt. | ⬜ |
| 6 | Hoymiles | HiBattery 4020 X | 🟢 Kompatibel | Shelly / EcoTracker | Lokale LAN-Anbindung von Drittanbieter-Zählern offiziell vorgesehen. | ⬜ |
| 7 | Hoymiles | MS-A2 | 🟢 Kompatibel | Shelly Pro 3EM | Shelly Pro 3EM wird offiziell unterstützt; praktische Kopplung mit IR Tracker noch nicht getestet. | ⬜ |
| 8 | Hoymiles | HiBattery 1920 AC | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | EcoTracker und Shelly Pro 3EM werden unterstützt. Durch die EcoTracker-API ist die technische Voraussetzung vorhanden. | ⬜ |
| 9 | Anker SOLIX | Solarbank 2 E1600 Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Die laufende Shelly-Abfrage erfolgt lokal im LAN; die erstmalige Gerätezuordnung über Shelly Cloud/Anker-App ist für einen Clone jedoch noch nicht vollständig geklärt. | ⬜ |
| 10 | Anker SOLIX | Solarbank 2 E1600 Plus | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Lokale Messwertabfrage ist technisch plausibel, die initiale Cloud-/Account-Zuordnung bleibt der offene Punkt. | ⬜ |
| 11 | Anker SOLIX | Solarbank 2 E1600 AC | 🟡 Kandidat | Shelly / Smart Meter | Fremdzähler-Unterstützung ist produkt- und firmwareabhängig; lokale Clone-Kompatibilität noch nicht ausreichend bestätigt. | ⬜ |
| 12 | Anker SOLIX | Solarbank 3 E2700 Pro | 🟡 Kandidat | Shelly Pro 3EM | Feldberichte sprechen für lokale Shelly-Kommunikation auch ohne WAN; die initiale Gerätezuordnung mit einem IR-Tracker-Clone ist noch nicht praktisch bestätigt. | ⬜ |
| 13 | Anker SOLIX | Solarbank 4 Pro / E5000 Pro | 🟡 Kandidat | Smart Meter / Modbus | Lokale Modbus-Kommunikation ist vorhanden. Ob darüber eine direkte Netzmesswert- oder Leistungsregelungsintegration des IR Trackers möglich ist, muss noch protokollseitig geprüft werden. | ⬜ |
| 14 | EcoFlow | STREAM AC | 🟢 Kompatibel | EcoTracker / Shelly | Von Everhome als kompatibles Energiesystem geführt; STREAM-Familie unterstützt externe Smart Meter. | ⬜ |
| 15 | EcoFlow | STREAM AC Pro | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Local Mode und lokale Smart-Meter-Kommunikation passen zur IR-Tracker-Emulation. | ⬜ |
| 16 | EcoFlow | STREAM Pro | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Lokale Smart-Meter-Kommunikation technisch passend. | ⬜ |
| 17 | EcoFlow | STREAM Max | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Unterstützte lokale Messgeräte können im Heimnetz verwendet werden. | ⬜ |
| 18 | EcoFlow | STREAM Ultra | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Lokaler Smart-Meter-Betrieb und Local Mode grundsätzlich passend. | ⬜ |
| 19 | EcoFlow | STREAM Ultra X | 🟢 Kompatibel | EcoTracker / Shelly | Von Everhome als kompatibles STREAM-System geführt; gleiche Smart-Meter-Produktfamilie. | ⬜ |
| 20 | Zendure | SolarFlow Hyper 2000 | 🟢 Kompatibel | Shelly Pro 3EM / EcoTracker | Direkte lokale CT-/Zählerkommunikation ist möglich. | ⬜ |
| 21 | Zendure | SolarFlow 800 | 🟢 Kompatibel | EcoTracker | Von Everhome ausdrücklich als EcoTracker-kompatibles Energiesystem geführt. | ⬜ |
| 22 | Zendure | SolarFlow 800 Pro | 🟢 Kompatibel | EcoTracker / Shelly | EcoTracker, Shelly 3EM und Shelly Pro 3EM werden unterstützt. | ⬜ |
| 23 | Zendure | SolarFlow 1600 AC+ | 🟢 Kompatibel | EcoTracker / Shelly | EcoTracker und Shelly werden offiziell unterstützt; lokale Zählerkommunikation ist vorgesehen. | ⬜ |
| 24 | Zendure | SolarFlow 2400 AC+ | 🟢 Kompatibel | EcoTracker / Shelly | Offizielle Unterstützung für EcoTracker, Shelly 3EM und Shelly Pro 3EM. | ⬜ |
| 25 | Zendure | SolarFlow 2400 Pro | 🟢 Kompatibel | EcoTracker / Shelly | Gehört zur aktuellen HEMS-Familie mit Unterstützung entsprechender Fremdzähler. | ⬜ |
| 26 | Jackery | Navi 2000 | 🟢 Kompatibel | Shelly Pro 3EM / Pro EM-50 | Direkte Smart-Meter-Kopplung vorgesehen; Discovery/Identität mit IR Tracker muss noch real getestet werden. | ⬜ |
| 27 | Jackery | HomePower 2000 Ultra | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM / Pro EM-50 | Jackery bestätigt EcoTracker IR/P1 sowie Shelly Pro 3EM und Pro EM-50 als kompatible Smart-Geräte. | ⬜ |
| 28 | Jackery | SolarVault 3 Pro | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM / Pro EM-50 | Jackery nennt EcoTracker IR/P1 sowie Shelly Pro 3EM und Pro EM-50 ausdrücklich als Drittanbieter-Kompatibilität. | ⬜ |
| 29 | Jackery | SolarVault 3 Pro Max | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM / Pro EM-50 | Gehört zur SolarVault-3-Familie mit derselben dokumentierten Drittanbieter-Smart-Meter-Unterstützung. | ⬜ |
| 30 | Jackery | SolarVault 3 Pro Max AC | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM / Pro EM-50 | Gehört zur SolarVault-3-Familie mit derselben dokumentierten Drittanbieter-Smart-Meter-Unterstützung. | ⬜ |
| 31 | Maxxisun | Maxxicharge V1 / CCU V1 1800 W | 🟢 Kompatibel | EcoTracker | EcoTracker wird von der CCU V1 unterstützt. | ⬜ |
| 32 | Maxxisun | Maxxicharge V2 / CCU V2 1200 W | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | CCU V2 unterstützt Smart Meter im Heimnetz und ist mit Maxxicharge 1.5, 3.0 und 5.0 kompatibel. | ⬜ |
| 33 | Maxxisun | Maxxicharge V2 / CCU V2 2300 W | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | Lokale Smart-Meter-Anbindung; kompatibel mit Maxxicharge 1.5, 3.0 und 5.0. | ⬜ |
| 34 | Maxxisun | Maxxicharge V2+ / CCU V2+ 2300 W (M3.0 / M5.0) | 🟢 Kompatibel | EcoTracker / Shelly 3EM / Shelly Pro 3EM | Maxxisun unterstützt offiziell Shelly 3EM, Shelly Pro 3EM, Pro 3EM CT63 und Everhome EcoTracker. | ⬜ |
| 35 | Marstek | Jupiter-C | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | Marstek bestätigt Shelly Pro 3EM; Everhome führt das Modell zusätzlich als EcoTracker-kompatibel. | ⬜ |
| 36 | Marstek | Jupiter-C Plus | 🟢 Kompatibel | EcoTracker | Von Everhome ausdrücklich als EcoTracker-kompatibel geführt. | ⬜ |
| 37 | Marstek | Jupiter-E | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | Marstek bestätigt für Jupiter C/E Shelly Pro 3EM; Everhome führt Jupiter-E als EcoTracker-kompatibel. | ⬜ |
| 38 | Marstek | Saturn B2500 | 🟢 Kompatibel | EcoTracker | Von Everhome ausdrücklich als EcoTracker-kompatibel geführt. | ⬜ |
| 39 | Marstek | Venus-A | 🟢 Kompatibel | EcoTracker | Von Everhome ausdrücklich als EcoTracker-kompatibel geführt. | ⬜ |
| 40 | Marstek | Venus-C | 🟢 Kompatibel | EcoTracker | Von Everhome ausdrücklich als EcoTracker-kompatibel geführt. | ⬜ |
| 41 | Marstek | Venus-D | 🟢 Kompatibel | EcoTracker | Von Everhome ausdrücklich als EcoTracker-kompatibel geführt. | ⬜ |
| 42 | Marstek | Venus-E | 🟢 Kompatibel | EcoTracker | Von Everhome ausdrücklich als EcoTracker-kompatibel geführt. | ⬜ |
| 43 | sunpura | S2400 | 🟢 Kompatibel | EcoTracker | Von Everhome ausdrücklich als EcoTracker-kompatibler Speicher geführt. | ⬜ |
| 44 | APsystems | EZHI | 🟢 Kompatibel | EcoTracker / Shelly 3EM / Pro 3EM | APsystems bestätigt selbst EcoTracker IR sowie Shelly 3EM, Pro 3EM und Pro 3EM63 für die dynamische Leistungsregelung. | ⬜ |
| 45 | Fox ESS | Avocado 22 Pro | 🟢 Kompatibel | EcoTracker | Everhome führt den Avocado 22 Pro als EcoTracker-kompatibel; Feldberichte bestätigen die Kopplung für Nulleinspeisung. | ⬜ |
| 46 | GoodWe | ESA Athena | 🟢 Kompatibel | EcoTracker | In der aktuellen Everhome-Kompatibilitätsliste für EcoTracker enthalten; GoodWe positioniert ESA Athena als Plug-and-Play-Mikrospeicher. | ⬜ |
| 47 | Indevolt | BK1600 | 🟢 Kompatibel | EcoTracker | Von Everhome als EcoTracker-kompatibler Speicher geführt. | ⬜ |
| 48 | Indevolt | BK1600 Ultra | 🟢 Kompatibel | EcoTracker | Von Everhome als EcoTracker-kompatibler Speicher geführt. | ⬜ |
| 49 | Indevolt | PowerFlex 2000 | 🟢 Kompatibel | EcoTracker | Von Everhome als EcoTracker-kompatibler Speicher geführt. | ⬜ |
| 50 | Indevolt | SolidFlex 2000 | 🟢 Kompatibel | EcoTracker | Von Everhome als EcoTracker-kompatibler Speicher geführt. | ⬜ |
| 51 | Spaun | Energy Master 1600 | 🟢 Kompatibel | EcoTracker | Von Everhome ausdrücklich als EcoTracker-kompatibles Energiesystem geführt. | ⬜ |
| 52 | YOULIQ | one | 🟢 Kompatibel | EcoTracker | Hersteller liefert den Speicher mit Everhome EcoTracker aus und beschreibt die direkte Übertragung der Stromzählerdaten an den Speicher. | ⬜ |

## Aktueller Gesamtstand

**52 relevante Balkonkraftwerk-Speicher-/EMS-Systeme**

- 🟢 **Kompatibel:** 47
- 🟡 **Kandidaten:** 5
- ✅ **Auf realer Speicher-Hardware mit IR Tracker getestet:** 0

## Quellenlage und Bewertungsprinzip

Ein grüner Eintrag wird nur gesetzt, wenn mindestens eine belastbare Quelle eine für den IR Tracker relevante Fremdzähler-Schnittstelle bestätigt. Bevorzugt werden Herstellerangaben. Ergänzend werden Kompatibilitätsangaben von Everhome herangezogen, wenn ein Gerät ausdrücklich für EcoTracker IR/P1 gelistet ist.

Eine bloße allgemeine Smart-Home-, MQTT- oder Cloud-Anbindung reicht nicht für den grünen Status.

## Anforderungen für einen erfolgreichen Feldtest

Ein Modell erhält erst den Status ✅ **Getestet**, wenn auf echter Hardware mindestens folgende Punkte erfolgreich geprüft wurden:

1. IR Tracker wird vom Speicher/EMS als Netzmessgerät erkannt.
2. Netzbezug wird mit korrektem Vorzeichen übertragen.
3. Einspeisung wird korrekt erkannt.
4. Schnelle Laständerungen werden plausibel nachgeregelt.
5. Lade- und Entladeleistung reagieren korrekt auf die Messwerte.
6. Verbindungsverlust wird sicher behandelt.
7. Nach Wiederherstellung der Verbindung startet die Regelung automatisch erneut.
8. Dauerbetrieb über mehrere Stunden verursacht keine Kommunikationsabbrüche.

Damit wird klar zwischen **technischer Protokollkompatibilität** und **praktisch bestätigter Produktkompatibilität** unterschieden.
