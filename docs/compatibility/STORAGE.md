# Kompatibilität – Batteriespeicher & Energiemanagement

**Stand: 02.09.2026 · Firmware 1.3.2**

Diese Übersicht zeigt Batteriespeicher und Energiemanagementsysteme, die grundsätzlich mit **IR Tracker Offline** als externe Netzmessquelle verwendet werden können.

Die Bewertung basiert auf den aktuell implementierten lokalen Schnittstellen des IR Trackers sowie auf öffentlich dokumentierten Integrationsmöglichkeiten, Herstellerangaben und praktischen Tests mit Shelly-/EcoTracker-Emulationen auf ESP32/Tasmota/uni-meter bzw. kompatiblen Drittanbieter-Zählern.

## Statusdefinition

| Status | Bedeutung |
|---|---|
| 🟢 **Kompatibel** | Die benötigte lokale Zähler-Schnittstelle wird vom IR Tracker bereitgestellt und es gibt belastbare Hinweise bzw. Praxistests, dass keine relevante Originalmodell-, Cloud- oder Account-Prüfung die Nutzung einer Emulation verhindert. Ein abschließender Feldtest mit IR Tracker kann trotzdem noch ausstehen. |
| 🟡 **Kandidat** | Shelly, EcoTracker oder eine andere grundsätzlich passende Fremdzähler-Schnittstelle wird unterstützt, aber Originalmodell-, Discovery-, Cloud-, Account-, Onboarding- oder Stabilitätsprobleme sind noch nicht sicher ausgeschlossen. |
| ✅ **Getestet** | Das konkrete Speicher-/EMS-System wurde mit IR Tracker Offline auf realer Hardware erfolgreich geprüft. |
| ⬜ **Nicht getestet** | Technische Kompatibilität ist gegeben oder wahrscheinlich, ein realer Feldtest mit IR Tracker steht jedoch noch aus. |

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

> **Wichtig:** „Kompatibel“ bedeutet nicht automatisch „auf realer Hardware mit IR Tracker getestet“. Grün wird nur vergeben, wenn zusätzlich keine relevante Originalgeräte-/Modellprüfung erkennbar ist oder eine Fremdgeräte-/Emulationslösung praktisch nachgewiesen wurde.

## Kompatibilitätsübersicht

| # | Hersteller | Modell / Familie | Status | Bevorzugte Schnittstelle | Bewertung | Feldtest |
|---:|---|---|---|---|---|:---:|
| 1 | Solakon | ONE | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | ESP32-/Tasmota-Shelly-Emulation wurde am ONE bereits praktisch verwendet; keine harte Originalmodellprüfung erkennbar. | ⬜ |
| 2 | Growatt | NOAH 2000 | 🟢 Kompatibel | Shelly Pro 3EM | Shelly-Emulation auf Tasmota/IR-Lesekopf wird erkannt und regelt praktisch; Fremdgerät wird akzeptiert. EcoTracker-Emulation ist weniger stabil, daher Shelly bevorzugen. | ⬜ |
| 3 | Growatt | NEXA 2000 | 🟡 Kandidat | EcoTracker / Shelly | EcoTracker-Emulation wird erkannt und regelt, kann nach WLAN-/Speicherneustarts jedoch aus der App gelöscht werden. Stabilität noch nicht ausreichend. | ⬜ |
| 4 | Growatt | Aura 5000 | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität ist dokumentiert; Emulations-Onboarding und Stabilität noch nicht ausreichend bestätigt. | ⬜ |
| 5 | Hoymiles | HiBattery 4020 AC | 🟢 Kompatibel | Shelly / EcoTracker | Drittanbieter-Zähler werden ausdrücklich lokal im LAN unterstützt; keine harte Originalmodellbindung erkennbar. | ⬜ |
| 6 | Hoymiles | HiBattery 4020 X | 🟢 Kompatibel | Shelly / EcoTracker | Lokale LAN-Anbindung von Drittanbieter-Zählern ist vorgesehen; keine relevante Originalmodellprüfung erkennbar. | ⬜ |
| 7 | Hoymiles | MS-A2 | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | Tasmota-/EcoTracker-/Shelly-Emulationen werden praktisch verwendet; lokale API-Abfrage ohne Originalgerätezwang bestätigt. | ⬜ |
| 8 | Hoymiles | HiBattery 1920 AC | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | Praktische Tests mit Tasmota-EcoTracker-Emulation liegen vor; Fremdgerät wird akzeptiert. | ⬜ |
| 9 | Anker SOLIX | Solarbank 2 E1600 Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Laufende Shelly-Abfrage lokal möglich, die erstmalige Zuordnung über Shelly Cloud/Anker-App bleibt für Emulationen problematisch. | ⬜ |
| 10 | Anker SOLIX | Solarbank 2 E1600 Plus | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Lokale Messwertabfrage technisch plausibel, initiale Cloud-/Account-Zuordnung bleibt offen. | ⬜ |
| 11 | Anker SOLIX | Solarbank 2 E1600 AC | 🟡 Kandidat | Shelly / Smart Meter | Fremdzähler-Unterstützung ist produkt- und firmwareabhängig; lokale Emulations-Kompatibilität nicht ausreichend bestätigt. | ⬜ |
| 12 | Anker SOLIX | Solarbank 3 E2700 Pro | 🟡 Kandidat | Shelly Pro 3EM | Lokale Shelly-Kommunikation möglich, initiale Gerätezuordnung mit Emulation aber nicht zuverlässig bestätigt. | ⬜ |
| 13 | Anker SOLIX | Solarbank 4 Pro / E5000 Pro | 🟡 Kandidat | EcoTracker / Smart Meter / Modbus | Passende Wege vorhanden, aber Drittanbieter-Tests zeigen weiterhin Einschränkungen bei lokaler Kompatibilitäts-Emulation. | ⬜ |
| 14 | EcoFlow | STREAM AC | 🟡 Kandidat | EcoTracker / Shelly | Externe Smart Meter werden unterstützt; ein belastbarer Emulationsnachweis speziell für STREAM AC fehlt noch. | ⬜ |
| 15 | EcoFlow | STREAM AC Pro | 🟢 Kompatibel | EcoTracker | uni-meter-EcoTracker-Emulation wurde von der EcoFlow-App erkannt und praktisch mit STREAM AC Pro betrieben. | ⬜ |
| 16 | EcoFlow | STREAM Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Lokale Smart-Meter-Kommunikation ist passend, ein konkreter Emulations-Praxistest für dieses Modell fehlt. | ⬜ |
| 17 | EcoFlow | STREAM Max | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Unterstützt externe Smart Meter, Akzeptanz einer Emulation speziell für dieses Modell noch nicht bestätigt. | ⬜ |
| 18 | EcoFlow | STREAM Ultra | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Local Mode vorhanden; modellbezogener Emulationsnachweis fehlt noch. | ⬜ |
| 19 | EcoFlow | STREAM Ultra X | 🟡 Kandidat | EcoTracker / Shelly | Schnittstellen passen, ein konkreter Fremdgeräte-/Emulations-Feldtest für Ultra X fehlt. | ⬜ |
| 20 | Zendure | SolarFlow Hyper 2000 | 🟡 Kandidat | Shelly Pro 3EM / EcoTracker | Spätere Kommunikation lokal, Onboarding kann Shelly-Konto/Cloud bzw. Gerätezuordnung voraussetzen; Drittanbieter-Kompatibilitätsmodus derzeit nicht sicher. | ⬜ |
| 21 | Zendure | SolarFlow 800 | 🟡 Kandidat | EcoTracker | EcoTracker wird unterstützt; Drittanbieter-Kompatibilitätsmodus ist derzeit nicht zuverlässig als rein lokale Emulation bestätigt. | ⬜ |
| 22 | Zendure | SolarFlow 800 Pro | 🟡 Kandidat | EcoTracker / Shelly | Passende Schnittstellen vorhanden, zusätzliche Geräte-/Onboarding-Prüfung nicht ausgeschlossen. | ⬜ |
| 23 | Zendure | SolarFlow 1600 AC+ | 🟡 Kandidat | EcoTracker / Shelly | Lokale Zählerkommunikation vorgesehen; Akzeptanz einer reinen Emulation noch nicht bestätigt. | ⬜ |
| 24 | Zendure | SolarFlow 2400 AC+ | 🟡 Kandidat | EcoTracker / Shelly | Offizielle EcoTracker-/Shelly-Unterstützung vorhanden, Discovery/Onboarding mit Emulation bleibt offen. | ⬜ |
| 25 | Zendure | SolarFlow 2400 Pro | 🟡 Kandidat | EcoTracker / Shelly | Fremdzähler-Unterstützung passt technisch, zusätzliche Modell-/Accountprüfung nicht sicher ausgeschlossen. | ⬜ |
| 26 | Jackery | Navi 2000 | 🟡 Kandidat | Shelly Pro 3EM / Pro EM-50 | Direkte Smart-Meter-Kopplung vorgesehen; belastbarer Emulations-Praxistest fehlt. | ⬜ |
| 27 | Jackery | HomePower 2000 Ultra | 🟢 Kompatibel | EcoTracker | Virtueller EcoTracker aus uni-meter wird von der Jackery-App erkannt und lässt sich einbinden; Nutzer betreiben die Regelung damit praktisch. | ⬜ |
| 28 | Jackery | SolarVault 3 Pro | 🟢 Kompatibel | EcoTracker | uni-meter führt die Jackery SolarVault Serie 3 ausdrücklich als unterstützten Speicher; virtueller EcoTracker ist damit praktisch nutzbar. | ⬜ |
| 29 | Jackery | SolarVault 3 Pro Max | 🟢 Kompatibel | EcoTracker | Gehört zur von uni-meter ausdrücklich unterstützten SolarVault-Serie 3; kein Original-EcoTracker erforderlich. | ⬜ |
| 30 | Jackery | SolarVault 3 Pro Max AC | 🟢 Kompatibel | EcoTracker | Gehört zur von uni-meter ausdrücklich unterstützten SolarVault-Serie 3; virtueller EcoTracker wird als Fremdzählerweg unterstützt. | ⬜ |
| 31 | Maxxisun | Maxxicharge V1 / CCU V1 1800 W | 🟢 Kompatibel | EcoTracker | EcoTracker wird lokal von der CCU V1 unterstützt; Maxxisun ist nicht auf einen einzelnen Zählerhersteller festgelegt. | ⬜ |
| 32 | Maxxisun | Maxxicharge V2 / CCU V2 1200 W | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | CCU V2 unterstützt verschiedene Smart Meter im Heimnetz; keine harte Originalmodellbindung erkennbar. | ⬜ |
| 33 | Maxxisun | Maxxicharge V2 / CCU V2 2300 W | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | Lokale Smart-Meter-Anbindung mit mehreren Fremdzählern; keine relevante Originalmodellbindung erkennbar. | ⬜ |
| 34 | Maxxisun | Maxxicharge V2+ / CCU V2+ 2300 W (M3.0 / M5.0) | 🟢 Kompatibel | EcoTracker / Shelly 3EM / Shelly Pro 3EM | Mehrere unterschiedliche Fremdzähler offiziell unterstützt; keine Bindung an ein einzelnes Originalmodell. | ⬜ |
| 35 | Marstek | Jupiter-C | 🟡 Kandidat | EcoTracker / Shelly Pro 3EM | Passende Fremdzähler unterstützt; konkreter modellbezogener Emulations-Feldtest noch nicht eindeutig genug. | ⬜ |
| 36 | Marstek | Jupiter-C Plus | 🟢 Kompatibel | EcoTracker / Shelly | EcoTracker-Emulation wurde auf ESP32-C3/bitShake mit Jupiter C Plus erfolgreich getestet; auch Shelly-/CT-Emulation wird praktisch verwendet. | ⬜ |
| 37 | Marstek | Jupiter-E | 🟡 Kandidat | EcoTracker / Shelly Pro 3EM | Passende Schnittstellen vorhanden; konkreter Emulationsnachweis für Jupiter-E noch nicht ausreichend. | ⬜ |
| 38 | Marstek | Saturn B2500 | 🟢 Kompatibel | Shelly / EcoTracker | B2500 wird praktisch mit Tasmota-Shelly- bzw. EcoTracker-Emulation betrieben; Fremdgerät wird akzeptiert. | ⬜ |
| 39 | Marstek | Venus-A | 🟢 Kompatibel | Shelly / EcoTracker | Venus A wurde praktisch mit Shelly-/EcoTracker-Emulation bzw. IOmeter-Kompatibilitätsmodus betrieben. | ⬜ |
| 40 | Marstek | Venus-C | 🟢 Kompatibel | EcoTracker | IOmeter-Kompatibilitätsmodus führt Venus C als bestätigt; keine zwingende Original-EcoTracker-Hardware erforderlich. | ⬜ |
| 41 | Marstek | Venus-D | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität vorhanden, aber ein eindeutiger Emulations-Praxistest speziell für Venus-D fehlt. | ⬜ |
| 42 | Marstek | Venus-E | 🟢 Kompatibel | EcoTracker / Shelly | IOmeter-Kompatibilitätsmodus führt Venus E als bestätigt; zusätzlich existieren praktische Emulationsversuche auf ESP32. | ⬜ |
| 43 | sunpura | S2400 | 🟡 Kandidat | EcoTracker | EcoTracker-kompatibel, aber belastbarer Nachweis einer Fremdgeräte-/Emulationskopplung fehlt. | ⬜ |
| 44 | APsystems | EZHI | 🟡 Kandidat | EcoTracker / Shelly 3EM / Pro 3EM | Passende Zähler werden unterstützt; Berichte weisen jedoch auf Cloud-basierte Gerätesuche hin, wodurch reine lokale Emulation problematisch sein kann. | ⬜ |
| 45 | Fox ESS | Avocado 22 Pro | 🟢 Kompatibel | EcoTracker / IOmeter | IOmeter nennt für FoxESS eine direkte Batteriespeicher-Integration; damit ist die Nutzung eines Nicht-Original-EcoTrackers grundsätzlich praktisch vorgesehen. | ⬜ |
| 46 | GoodWe | ESA Athena | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität gelistet, aber Emulations-/Drittanbieter-Onboarding noch nicht belastbar bestätigt. | ⬜ |
| 47 | Indevolt | BK1600 | 🟡 Kandidat | EcoTracker / Shelly | Hersteller bestätigt vollständige lokale Bedienung und viele Drittanbieter-Zähler; ein expliziter Emulations-Feldtest für BK1600 fehlt noch. | ⬜ |
| 48 | Indevolt | BK1600 Ultra | 🟡 Kandidat | EcoTracker / Shelly | Hersteller bestätigt lokale Bedienung und mehrere Fremdzähler; expliziter Emulationsnachweis für BK1600 Ultra noch offen. | ⬜ |
| 49 | Indevolt | PowerFlex 2000 | 🟢 Kompatibel | Shelly Pro 3EM | uni-meter-Shelly-Pro-3EM-Ausgabe wurde praktisch mit PowerFlex 2000 verwendet; Nicht-Original-Shelly wird akzeptiert. | ⬜ |
| 50 | Indevolt | SolidFlex 2000 | 🟢 Kompatibel | EcoTracker / Shelly | uni-meter führt SolidFlex 2000 ausdrücklich als unterstützten Speicher; außerdem wird EcoTracker im Betrieb per fester IP eingebunden. | ⬜ |
| 51 | Spaun | Energy Master 1600 | 🟡 Kandidat | EcoTracker | EcoTracker-kompatibel; Geräteerkennung/Onboarding einer Emulation noch nicht ausreichend geklärt. | ⬜ |
| 52 | YOULIQ | one | 🟢 Kompatibel | EcoTracker / IOmeter | YOULIQ nutzt externe Zählerdaten und IOmeter nennt YOUL als direkte Batteriespeicher-Integration; Original-Everhome-Hardware ist damit nicht zwingend. | ⬜ |

## Aktueller Gesamtstand

**52 relevante Balkonkraftwerk-Speicher-/EMS-Systeme**

- 🟢 **Kompatibel:** 24
- 🟡 **Kandidaten:** 28
- ✅ **Auf realer Speicher-Hardware mit IR Tracker getestet:** 0

## Quellenlage und Bewertungsprinzip

Ein grüner Eintrag wird nur gesetzt, wenn mindestens eine belastbare Quelle eine für den IR Tracker relevante **lokale** Fremdzähler-Schnittstelle bestätigt und zusätzlich keine relevante Originalmodell-, Cloud-, Account- oder Geräteidentitätsprüfung erkennbar ist **oder** wenn eine nicht originale Shelly-/EcoTracker-Emulation bzw. ein kompatibler Drittanbieter-Zähler am betreffenden Speicher praktisch nachgewiesen ist.

Besonders aussagekräftig sind Tests mit ESP32/Tasmota, uni-meter und IOmeter-Kompatibilitätsmodus, weil diese Lösungen genau wie IR Tracker Messwerte über nachgebildete lokale Fremdzähler-Schnittstellen bereitstellen.

Die bloße Unterstützung von Shelly oder EcoTracker reicht nicht automatisch für Grün. Wenn zwar eine passende API vorhanden ist, aber Discovery, Onboarding, Cloud-/Account-Zuordnung oder Stabilität noch ungeklärt sind, bleibt das System gelb.

Die lokale EcoTracker-API `/v1/json` selbst enthält im dokumentierten Messwertformat keine Modell-, Seriennummer- oder MAC-Kennung. Ein Speicher kann beim erstmaligen Hinzufügen dennoch mDNS, Hostname, MAC/Serial, Bluetooth oder Cloud-/App-Informationen verwenden.

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

Damit wird klar zwischen **technischer Protokollkompatibilität**, **nachgewiesener Fremdgeräte-/Emulationsfähigkeit** und **praktisch bestätigter IR-Tracker-Produktkompatibilität** unterschieden.