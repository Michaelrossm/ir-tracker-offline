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

Firmware **1.3.1** stellt unter anderem folgende lokale Integrationen bereit:

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
| 7 | Hoymiles | HiBattery 1920 AC | 🟢 Kompatibel | **EcoTracker / Shelly Pro 3EM** | EcoTracker und Shelly Pro 3EM werden unterstützt. Durch die EcoTracker-API von Firmware 1.3.1 ist die technische Voraussetzung nun vorhanden. | ⬜ |
| 8 | Anker SOLIX | Solarbank 2 E1600 Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Shelly wird unterstützt, die offizielle Kopplung verwendet jedoch Shelly Cloud und ein Shelly-Konto. | ⬜ |
| 9 | Anker SOLIX | Solarbank 2 E1600 Plus | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Gleiche Einschränkung wie bei Solarbank 2 Pro: Cloud-/Account-Bindung muss noch geklärt werden. | ⬜ |
| 10 | Anker SOLIX | Solarbank 2 E1600 AC | 🟡 Kandidat | Shelly / Smart Meter | Fremdzähler-Unterstützung ist produkt- und firmwareabhängig; lokale Clone-Kompatibilität nicht ausreichend bestätigt. | ⬜ |
| 11 | Anker SOLIX | Solarbank 3 E2700 Pro | 🟡 Kandidat | Smart Meter / Shelly | Externe Messung vorhanden, aber keine ausreichend belegte lokale Shelly-Clone-Anbindung. | ⬜ |
| 12 | Anker SOLIX | Solarbank 4 Pro / E5000 Pro | 🟡 Kandidat | Smart Meter | Aktuelles System mit Smart-Meter-Regelung; Fremdzähler- und lokale Clone-Unterstützung müssen noch eindeutig bestätigt werden. | ⬜ |
| 13 | EcoFlow | STREAM Ultra | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Lokaler Smart-Meter-Betrieb und Local Mode grundsätzlich passend. | ⬜ |
| 14 | EcoFlow | STREAM Pro | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Lokale Smart-Meter-Kommunikation technisch passend. | ⬜ |
| 15 | EcoFlow | STREAM Max | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Unterstützte lokale Messgeräte können im Heimnetz verwendet werden. | ⬜ |
| 16 | EcoFlow | STREAM AC Pro | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Local Mode und lokale Smart-Meter-Kommunikation passen zur IR-Tracker-Emulation. | ⬜ |
| 17 | Zendure | SolarFlow Hyper 2000 | 🟢 Kompatibel | Shelly Pro 3EM | Direkte lokale CT-Kommunikation ist möglich. | ⬜ |
| 18 | Zendure | SolarFlow 800 Pro | 🟢 Kompatibel | **EcoTracker / Shelly** | EcoTracker, Shelly 3EM und Shelly Pro 3EM werden unterstützt. Durch die neue EcoTracker-API ist die technische Kompatibilität deutlich besser als bei älteren Firmwareständen. | ⬜ |
| 19 | Zendure | SolarFlow 1600 AC+ | 🟢 Kompatibel | EcoTracker / Shelly | EcoTracker und Shelly werden offiziell unterstützt; lokale Zählerkommunikation ist vorgesehen. | ⬜ |
| 20 | Zendure | SolarFlow 2400 AC+ | 🟢 Kompatibel | EcoTracker / Shelly | Offizielle Unterstützung für EcoTracker, Shelly 3EM und Shelly Pro 3EM. | ⬜ |
| 21 | Zendure | SolarFlow 2400 Pro | 🟢 Kompatibel | EcoTracker / Shelly | Gehört zur aktuellen HEMS-Familie mit Unterstützung entsprechender Fremdzähler. | ⬜ |
| 22 | Jackery | Navi 2000 | 🟢 Kompatibel | Shelly Pro 3EM / Pro EM-50 | Direkte Smart-Meter-Kopplung vorgesehen; Discovery/Identität mit IR Tracker muss noch real getestet werden. | ⬜ |

## Aktueller Gesamtstand

**22 relevante Speicher-/EMS-Systeme**

- 🟢 **Kompatibel:** 17
- 🟡 **Kandidaten:** 5
- ✅ **Auf realer Speicher-Hardware getestet:** 0

### Änderungen mit Firmware 1.3.1

**Neu auf 🟢 Kompatibel gesetzt:**

- Hoymiles HiBattery 1920 AC
- Zendure SolarFlow 800 Pro

Grund dafür ist insbesondere die neue **EcoTracker-kompatible `/v1/json`-Schnittstelle** sowie die erweiterte Shelly-Kompatibilität des IR Trackers.

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
