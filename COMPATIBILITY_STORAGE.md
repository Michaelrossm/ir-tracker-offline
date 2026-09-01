# Kompatibilität – Batteriespeicher & Energiemanagement

**Stand: 01.09.2026 · Firmware 1.3.2**

Diese Übersicht zeigt Batteriespeicher und Energiemanagementsysteme, die grundsätzlich mit **IR Tracker Offline** als externe Netzmessquelle verwendet werden können.

Die Bewertung basiert auf den aktuell implementierten lokalen Schnittstellen des IR Trackers sowie auf öffentlich dokumentierten Integrationsmöglichkeiten der jeweiligen Hersteller.

## Statusdefinition

| Status | Bedeutung |
|---|---|
| 🟢 **Kompatibel** | Die vom System benötigte lokale Zähler-Schnittstelle wird vom IR Tracker grundsätzlich bereitgestellt. Ein abschließender Hardware-Feldtest kann trotzdem noch ausstehen. |
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
| 1 | Solakon | ONE | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Lokale Shelly-Anbindung passt grundsätzlich zur aktuellen IR-Tracker-Emulation. | ⬜ |
| 2 | Growatt | NOAH 2000 | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Unterstützt externe Shelly-Zähler für die Eigenverbrauchsregelung. | ⬜ |
| 3 | Growatt | NEXA 2000 | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Lokale Fremdzähler-Anbindung technisch passend. | ⬜ |
| 4 | Hoymiles | HiBattery 4020 AC | 🟢 Kompatibel | Shelly / EcoTracker | Drittanbieter-Zähler werden ausdrücklich über lokales LAN unterstützt. | ⬜ |
| 5 | Hoymiles | HiBattery 4020 X | 🟢 Kompatibel | Shelly / EcoTracker | Lokale LAN-Anbindung von Drittanbieter-Zählern offiziell vorgesehen. | ⬜ |
| 6 | Hoymiles | MS-A2 | 🟢 Kompatibel | Shelly Pro 3EM | Shelly Pro 3EM wird offiziell unterstützt; praktische Kopplung mit IR Tracker noch nicht getestet. | ⬜ |
| 7 | Hoymiles | HiBattery 1920 AC | 🟢 Kompatibel | **EcoTracker / Shelly Pro 3EM** | EcoTracker und Shelly Pro 3EM werden unterstützt. Durch die EcoTracker-API ist die technische Voraussetzung vorhanden. | ⬜ |
| 8 | Anker SOLIX | Solarbank 2 E1600 Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Die laufende Shelly-Abfrage erfolgt lokal im LAN; die erstmalige Gerätezuordnung über Shelly Cloud/Anker-App ist für einen Clone jedoch noch nicht vollständig geklärt. | ⬜ |
| 9 | Anker SOLIX | Solarbank 2 E1600 Plus | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Lokale Messwertabfrage ist technisch plausibel, die initiale Cloud-/Account-Zuordnung bleibt der offene Punkt. | ⬜ |
| 10 | Anker SOLIX | Solarbank 2 E1600 AC | 🟡 Kandidat | Shelly / Smart Meter | Fremdzähler-Unterstützung ist produkt- und firmwareabhängig; lokale Clone-Kompatibilität noch nicht ausreichend bestätigt. | ⬜ |
| 11 | Anker SOLIX | Solarbank 3 E2700 Pro | 🟡 Kandidat | Shelly Pro 3EM | Feldberichte sprechen für lokale Shelly-Kommunikation auch ohne WAN; die initiale Gerätezuordnung mit einem IR-Tracker-Clone ist noch nicht praktisch bestätigt. | ⬜ |
| 12 | Anker SOLIX | Solarbank 4 Pro / E5000 Pro | 🟡 Kandidat | Smart Meter / Modbus | Lokale Modbus-Kommunikation ist vorhanden. Ob darüber eine direkte Netzmesswert- oder Leistungsregelungsintegration des IR Trackers möglich ist, muss noch protokollseitig geprüft werden. | ⬜ |
| 13 | EcoFlow | STREAM Ultra | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Lokaler Smart-Meter-Betrieb und Local Mode grundsätzlich passend. | ⬜ |
| 14 | EcoFlow | STREAM Pro | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Lokale Smart-Meter-Kommunikation technisch passend. | ⬜ |
| 15 | EcoFlow | STREAM Max | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Unterstützte lokale Messgeräte können im Heimnetz verwendet werden. | ⬜ |
| 16 | EcoFlow | STREAM AC Pro | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Local Mode und lokale Smart-Meter-Kommunikation passen zur IR-Tracker-Emulation. | ⬜ |
| 17 | Zendure | SolarFlow Hyper 2000 | 🟢 Kompatibel | Shelly Pro 3EM | Direkte lokale CT-Kommunikation ist möglich. | ⬜ |
| 18 | Zendure | SolarFlow 800 Pro | 🟢 Kompatibel | **EcoTracker / Shelly** | EcoTracker, Shelly 3EM und Shelly Pro 3EM werden unterstützt. Durch die EcoTracker-API ist die technische Kompatibilität deutlich besser als bei älteren Firmwareständen. | ⬜ |
| 19 | Zendure | SolarFlow 1600 AC+ | 🟢 Kompatibel | EcoTracker / Shelly | EcoTracker und Shelly werden offiziell unterstützt; lokale Zählerkommunikation ist vorgesehen. | ⬜ |
| 20 | Zendure | SolarFlow 2400 AC+ | 🟢 Kompatibel | EcoTracker / Shelly | Offizielle Unterstützung für EcoTracker, Shelly 3EM und Shelly Pro 3EM. | ⬜ |
| 21 | Zendure | SolarFlow 2400 Pro | 🟢 Kompatibel | EcoTracker / Shelly | Gehört zur aktuellen HEMS-Familie mit Unterstützung entsprechender Fremdzähler. | ⬜ |
| 22 | Jackery | Navi 2000 | 🟢 Kompatibel | Shelly Pro 3EM / Pro EM-50 | Direkte Smart-Meter-Kopplung vorgesehen; Discovery/Identität mit IR Tracker muss noch real getestet werden. | ⬜ |
| 23 | Maxxisun | Maxxicharge V1 / CCU V1 1800 W | 🟢 Kompatibel | EcoTracker | EcoTracker wird von der CCU V1 unterstützt. Maxxisun dokumentiert bei älteren Firmwareständen noch Optimierungsbedarf beim Regelverhalten, die grundlegende Schnittstellenkompatibilität ist jedoch vorhanden. | ⬜ |
| 24 | Maxxisun | Maxxicharge V2 / CCU V2 1200 W | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | CCU V2 unterstützt Smart Meter im Heimnetz und ist mit Maxxicharge 1.5, 3.0 und 5.0 kompatibel. Sehr guter Kandidat für die aktuellen EcoTracker-/Shelly-Endpunkte. | ⬜ |
| 25 | Maxxisun | Maxxicharge V2 / CCU V2 2300 W | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | Wie CCU V2 1200 W: lokale Smart-Meter-Anbindung; kompatibel mit Maxxicharge 1.5, 3.0 und 5.0. | ⬜ |
| 26 | Maxxisun | Maxxicharge V2+ / CCU V2+ 2300 W (M3.0 / M5.0) | 🟢 Kompatibel | **EcoTracker / Shelly 3EM / Shelly Pro 3EM** | Maxxisun unterstützt offiziell Shelly 3EM, Shelly Pro 3EM, Pro 3EM CT63, Everhome EcoTracker, IOMeter und Powerfox Poweropti. Die CCU V2+ arbeitet auch mit älteren Maxxicharge-Speichergenerationen über Adapter. | ⬜ |

## Aktueller Gesamtstand

**26 relevante Speicher-/EMS-Systeme**

- 🟢 **Kompatibel:** 21
- 🟡 **Kandidaten:** 5
- ✅ **Auf realer Speicher-Hardware getestet:** 0

### Änderungen seit Firmware 1.3.1

Neu bzw. aufgrund der aktuellen Schnittstellenlage als 🟢 **Kompatibel** aufgenommen:

- Hoymiles HiBattery 1920 AC
- Zendure SolarFlow 800 Pro
- Maxxisun Maxxicharge V1 / CCU V1 1800 W
- Maxxisun Maxxicharge V2 / CCU V2 1200 W
- Maxxisun Maxxicharge V2 / CCU V2 2300 W
- Maxxisun Maxxicharge V2+ / CCU V2+ 2300 W

Grundlage dafür sind insbesondere die **EcoTracker-kompatible `/v1/json`-Schnittstelle**, die erweiterte Shelly-Kompatibilität des IR Trackers sowie die von Maxxisun dokumentierte Unterstützung lokaler Fremdzähler.

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
