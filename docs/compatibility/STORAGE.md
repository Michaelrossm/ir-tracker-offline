# Kompatibilität – Batteriespeicher & Energiemanagement

**Stand: 02.09.2026 · Firmware 1.3.2**

Diese Übersicht zeigt Batteriespeicher und Energiemanagementsysteme, die grundsätzlich mit **IR Tracker Offline** als externe Netzmessquelle verwendet werden können.

Die Bewertung basiert auf den aktuell implementierten lokalen Schnittstellen des IR Trackers sowie auf öffentlich dokumentierten Integrationsmöglichkeiten der jeweiligen Hersteller und etablierten Smart-Meter-Anbieter.

## Statusdefinition

| Status | Bedeutung |
|---|---|
| 🟢 **Kompatibel** | Die benötigte lokale Zähler-Schnittstelle wird vom IR Tracker bereitgestellt und es ist keine relevante Originalmodell-, Cloud- oder Account-Prüfung erkennbar. Ein abschließender Hardware-Feldtest kann trotzdem noch ausstehen. |
| 🟡 **Kandidat** | Shelly, EcoTracker oder eine andere grundsätzlich passende Fremdzähler-Schnittstelle wird unterstützt, aber Originalmodell-, Discovery-, Cloud-, Account- oder Onboarding-Prüfung ist noch nicht sicher ausgeschlossen. |
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

> **Wichtig:** „Kompatibel“ bedeutet nicht automatisch „auf realer Hardware getestet“. Erst ein erfolgreicher Feldtest erhält den Status ✅. Grün wird nur vergeben, wenn zusätzlich keine relevante Originalgeräte-/Modellprüfung erkennbar ist.

## Kompatibilitätsübersicht

| # | Hersteller | Modell / Familie | Status | Bevorzugte Schnittstelle | Bewertung | Feldtest |
|---:|---|---|---|---|---|:---:|
| 1 | Solakon | ONE | 🟢 Kompatibel | Shelly 3EM / Pro 3EM | Lokale Shelly-Abfrage; ESP32-/Tasmota-Shelly-Emulation wurde am ONE bereits praktisch verwendet. Eine harte Originalmodellprüfung ist nicht erkennbar. | ⬜ |
| 2 | Growatt | NOAH 2000 | 🟡 Kandidat | Shelly / EcoTracker | Fremdzähler-Unterstützung ist vorhanden, aber noch nicht ausreichend bestätigt, ob ein reiner IR-Tracker-Clone ohne zusätzliche Geräte-/App-Zuordnung akzeptiert wird. | ⬜ |
| 3 | Growatt | NEXA 2000 | 🟡 Kandidat | Shelly / EcoTracker | Lokale Fremdzähler-Anbindung ist grundsätzlich passend; Originalmodell-/Onboarding-Prüfung mit einer Emulation ist noch nicht ausgeschlossen. | ⬜ |
| 4 | Growatt | Aura 5000 | 🟡 Kandidat | EcoTracker | Als EcoTracker-kompatibles Energiesystem geführt; ob ausschließlich `/v1/json` genügt oder zusätzliche Geräteidentität genutzt wird, ist noch nicht bestätigt. | ⬜ |
| 5 | Hoymiles | HiBattery 4020 AC | 🟢 Kompatibel | Shelly / EcoTracker | Drittanbieter-Zähler werden ausdrücklich lokal im LAN unterstützt; keine harte Originalmodellbindung ist erkennbar. | ⬜ |
| 6 | Hoymiles | HiBattery 4020 X | 🟢 Kompatibel | Shelly / EcoTracker | Lokale LAN-Anbindung von Drittanbieter-Zählern ist vorgesehen; keine relevante Originalmodellprüfung ist erkennbar. | ⬜ |
| 7 | Hoymiles | MS-A2 | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | Lokale EcoTracker-/Shelly-Anbindung; die EcoTracker-Messwert-API selbst enthält keine Modellkennung. Gute Voraussetzungen für IR-Tracker-Emulation. | ⬜ |
| 8 | Hoymiles | HiBattery 1920 AC | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | EcoTracker und Shelly Pro 3EM werden unterstützt; lokale Messwertabfrage ohne erkennbare harte Modellprüfung. | ⬜ |
| 9 | Anker SOLIX | Solarbank 2 E1600 Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Die laufende Shelly-Abfrage erfolgt lokal im LAN; die erstmalige Gerätezuordnung über Shelly Cloud/Anker-App ist für einen Clone jedoch noch nicht vollständig geklärt. | ⬜ |
| 10 | Anker SOLIX | Solarbank 2 E1600 Plus | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Lokale Messwertabfrage ist technisch plausibel, die initiale Cloud-/Account-Zuordnung bleibt der offene Punkt. | ⬜ |
| 11 | Anker SOLIX | Solarbank 2 E1600 AC | 🟡 Kandidat | Shelly / Smart Meter | Fremdzähler-Unterstützung ist produkt- und firmwareabhängig; lokale Clone-Kompatibilität noch nicht ausreichend bestätigt. | ⬜ |
| 12 | Anker SOLIX | Solarbank 3 E2700 Pro | 🟡 Kandidat | Shelly Pro 3EM | Lokale Shelly-Kommunikation ist möglich, die initiale Gerätezuordnung mit einem IR-Tracker-Clone ist aber noch nicht praktisch bestätigt. | ⬜ |
| 13 | Anker SOLIX | Solarbank 4 Pro / E5000 Pro | 🟡 Kandidat | EcoTracker / Smart Meter / Modbus | EcoTracker bzw. weitere Smart-Meter-Wege sind interessant, aber eine reine lokale IR-Tracker-Emulation ohne zusätzliche Geräteidentität ist noch nicht bestätigt. | ⬜ |
| 14 | EcoFlow | STREAM AC | 🟡 Kandidat | EcoTracker / Shelly | Externe Smart Meter werden unterstützt, jedoch sind Modell-/App-/Account-Bindungen innerhalb der STREAM-Familie noch nicht sicher ausgeschlossen. | ⬜ |
| 15 | EcoFlow | STREAM AC Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM | EcoFlow nennt konkrete Shelly-Modelle; daher ist noch offen, ob eine reine Shelly-API-Emulation ohne Originalidentität akzeptiert wird. | ⬜ |
| 16 | EcoFlow | STREAM Pro | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Lokale Smart-Meter-Kommunikation ist grundsätzlich passend, eine Modell-/Account-Prüfung ist aber noch nicht ausgeschlossen. | ⬜ |
| 17 | EcoFlow | STREAM Max | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Unterstützt externe Smart Meter, aber die Akzeptanz eines nicht originalen Shelly-Geräts ist noch nicht sicher bestätigt. | ⬜ |
| 18 | EcoFlow | STREAM Ultra | 🟡 Kandidat | Shelly 3EM / Pro 3EM | Local Mode ist vorhanden; konkrete Shelly-Modellunterstützung lässt eine zusätzliche Geräteprüfung möglich erscheinen. | ⬜ |
| 19 | EcoFlow | STREAM Ultra X | 🟡 Kandidat | EcoTracker / Shelly | EcoTracker/Shelly sind grundsätzlich passend, aber das Onboarding einer reinen Emulation ohne Originalgerät ist noch nicht bestätigt. | ⬜ |
| 20 | Zendure | SolarFlow Hyper 2000 | 🟡 Kandidat | Shelly Pro 3EM / EcoTracker | Die spätere Shelly-Kommunikation läuft lokal, das erstmalige Onboarding kann jedoch Shelly-Konto/Cloud bzw. Gerätezuordnung voraussetzen. | ⬜ |
| 21 | Zendure | SolarFlow 800 | 🟡 Kandidat | EcoTracker | EcoTracker wird unterstützt; ob beim Hinzufügen zusätzlich Geräte-Discovery oder Everhome-Identität geprüft wird, ist noch nicht ausreichend bestätigt. | ⬜ |
| 22 | Zendure | SolarFlow 800 Pro | 🟡 Kandidat | EcoTracker / Shelly | EcoTracker und Shelly werden unterstützt, aber eine zusätzliche Geräte-/Onboarding-Prüfung ist noch nicht ausgeschlossen. | ⬜ |
| 23 | Zendure | SolarFlow 1600 AC+ | 🟡 Kandidat | EcoTracker / Shelly | Lokale Zählerkommunikation ist vorgesehen; die Akzeptanz eines IR-Tracker-Clones ohne Originalgerätekennung ist noch nicht bestätigt. | ⬜ |
| 24 | Zendure | SolarFlow 2400 AC+ | 🟡 Kandidat | EcoTracker / Shelly | Offizielle EcoTracker-/Shelly-Unterstützung ist vorhanden, aber Discovery/Onboarding mit einer reinen Emulation bleibt noch offen. | ⬜ |
| 25 | Zendure | SolarFlow 2400 Pro | 🟡 Kandidat | EcoTracker / Shelly | Fremdzähler-Unterstützung passt technisch, eine zusätzliche Modell-/Accountprüfung ist jedoch noch nicht sicher ausgeschlossen. | ⬜ |
| 26 | Jackery | Navi 2000 | 🟡 Kandidat | Shelly Pro 3EM / Pro EM-50 | Direkte Smart-Meter-Kopplung ist vorgesehen; Discovery/Identität mit IR Tracker muss noch real bestätigt werden. | ⬜ |
| 27 | Jackery | HomePower 2000 Ultra | 🟡 Kandidat | EcoTracker / Shelly Pro 3EM / Pro EM-50 | Unterstützt passende Drittanbieter-Zähler, aber eine reine API-Emulation ohne Originalgeräteidentität ist noch nicht bestätigt. | ⬜ |
| 28 | Jackery | SolarVault 3 Pro | 🟡 Kandidat | EcoTracker / Shelly Pro 3EM / Pro EM-50 | Drittanbieter-Smart-Meter werden unterstützt; Originalmodell-/Discovery-Prüfung mit IR Tracker ist noch nicht ausgeschlossen. | ⬜ |
| 29 | Jackery | SolarVault 3 Pro Max | 🟡 Kandidat | EcoTracker / Shelly Pro 3EM / Pro EM-50 | Gleiche Drittanbieter-Smart-Meter-Familie; Clone-Onboarding ohne Originalidentität noch nicht praktisch bestätigt. | ⬜ |
| 30 | Jackery | SolarVault 3 Pro Max AC | 🟡 Kandidat | EcoTracker / Shelly Pro 3EM / Pro EM-50 | Technisch passende Schnittstellen vorhanden, aber Geräte-/Onboarding-Prüfung mit einer Emulation noch offen. | ⬜ |
| 31 | Maxxisun | Maxxicharge V1 / CCU V1 1800 W | 🟢 Kompatibel | EcoTracker | EcoTracker wird lokal von der CCU V1 unterstützt; Maxxisun ist nicht auf einen einzigen Zählerhersteller festgelegt. | ⬜ |
| 32 | Maxxisun | Maxxicharge V2 / CCU V2 1200 W | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | CCU V2 unterstützt verschiedene Smart Meter im Heimnetz; keine harte Bindung an ein einzelnes Originalmodell ist erkennbar. | ⬜ |
| 33 | Maxxisun | Maxxicharge V2 / CCU V2 2300 W | 🟢 Kompatibel | EcoTracker / Shelly Pro 3EM | Lokale Smart-Meter-Anbindung mit mehreren unterstützten Fremdzählern; keine relevante Originalmodellbindung erkennbar. | ⬜ |
| 34 | Maxxisun | Maxxicharge V2+ / CCU V2+ 2300 W (M3.0 / M5.0) | 🟢 Kompatibel | EcoTracker / Shelly 3EM / Shelly Pro 3EM | Maxxisun unterstützt mehrere unterschiedliche Fremdzähler und ist ausdrücklich nicht auf einen einzelnen Zählertyp festgelegt. | ⬜ |
| 35 | Marstek | Jupiter-C | 🟡 Kandidat | EcoTracker / Shelly Pro 3EM | Unterstützt passende Fremdzähler; ob eine reine IR-Tracker-Emulation ohne Originalmodellkennung akzeptiert wird, ist noch nicht bestätigt. | ⬜ |
| 36 | Marstek | Jupiter-C Plus | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität ist dokumentiert, das konkrete Discovery-/Onboarding-Verhalten mit einer Emulation ist jedoch noch offen. | ⬜ |
| 37 | Marstek | Jupiter-E | 🟡 Kandidat | EcoTracker / Shelly Pro 3EM | Passende Schnittstellen vorhanden; Originalmodell-/Discovery-Prüfung mit IR Tracker noch nicht ausgeschlossen. | ⬜ |
| 38 | Marstek | Saturn B2500 | 🟡 Kandidat | EcoTracker | EcoTracker wird unterstützt; ob ausschließlich die lokale `/v1/json`-API genügt, ist noch nicht bestätigt. | ⬜ |
| 39 | Marstek | Venus-A | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität vorhanden; Geräteerkennung/Onboarding einer Emulation noch nicht ausreichend geklärt. | ⬜ |
| 40 | Marstek | Venus-C | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität vorhanden; Geräteerkennung/Onboarding einer Emulation noch nicht ausreichend geklärt. | ⬜ |
| 41 | Marstek | Venus-D | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität vorhanden; Geräteerkennung/Onboarding einer Emulation noch nicht ausreichend geklärt. | ⬜ |
| 42 | Marstek | Venus-E | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität vorhanden; Geräteerkennung/Onboarding einer Emulation noch nicht ausreichend geklärt. | ⬜ |
| 43 | sunpura | S2400 | 🟡 Kandidat | EcoTracker | Als EcoTracker-kompatibel geführt; ob der Speicher ausschließlich lokale Messwerte nutzt oder zusätzliche Identität prüft, ist noch offen. | ⬜ |
| 44 | APsystems | EZHI | 🟡 Kandidat | EcoTracker / Shelly 3EM / Pro 3EM | APsystems unterstützt mehrere passende Fremdzähler, die Akzeptanz einer reinen IR-Tracker-Emulation ist jedoch noch nicht praktisch bestätigt. | ⬜ |
| 45 | Fox ESS | Avocado 22 Pro | 🟡 Kandidat | EcoTracker | EcoTracker-Kopplung ist belegt; ein Feldtest mit einem emulierten EcoTracker statt Originalhardware fehlt noch. | ⬜ |
| 46 | GoodWe | ESA Athena | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität ist gelistet, aber Geräteidentitäts-/Onboarding-Verhalten mit IR Tracker ist noch nicht bestätigt. | ⬜ |
| 47 | Indevolt | BK1600 | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität vorhanden; reine lokale Clone-Kompatibilität ohne Originalgeräteprüfung noch nicht bestätigt. | ⬜ |
| 48 | Indevolt | BK1600 Ultra | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität vorhanden; reine lokale Clone-Kompatibilität ohne Originalgeräteprüfung noch nicht bestätigt. | ⬜ |
| 49 | Indevolt | PowerFlex 2000 | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität vorhanden; reine lokale Clone-Kompatibilität ohne Originalgeräteprüfung noch nicht bestätigt. | ⬜ |
| 50 | Indevolt | SolidFlex 2000 | 🟡 Kandidat | EcoTracker | EcoTracker-Kompatibilität vorhanden; reine lokale Clone-Kompatibilität ohne Originalgeräteprüfung noch nicht bestätigt. | ⬜ |
| 51 | Spaun | Energy Master 1600 | 🟡 Kandidat | EcoTracker | Als EcoTracker-kompatibel geführt; Geräteerkennung/Onboarding einer Emulation ist noch nicht ausreichend geklärt. | ⬜ |
| 52 | YOULIQ | one | 🟡 Kandidat | EcoTracker | Der Speicher nutzt EcoTracker, aber die dokumentierte Lösung basiert auf Original-Everhome-Hardware; IR-Tracker-Clone noch nicht bestätigt. | ⬜ |

## Aktueller Gesamtstand

**52 relevante Balkonkraftwerk-Speicher-/EMS-Systeme**

- 🟢 **Kompatibel:** 9
- 🟡 **Kandidaten:** 43
- ✅ **Auf realer Speicher-Hardware mit IR Tracker getestet:** 0

## Quellenlage und Bewertungsprinzip

Ein grüner Eintrag wird nur gesetzt, wenn mindestens eine belastbare Quelle eine für den IR Tracker relevante **lokale** Fremdzähler-Schnittstelle bestätigt und zusätzlich keine relevante Originalmodell-, Cloud-, Account- oder Geräteidentitätsprüfung erkennbar ist.

Die bloße Unterstützung von Shelly oder EcoTracker reicht deshalb nicht automatisch für Grün. Wenn zwar eine passende API vorhanden ist, aber Discovery, Onboarding oder Geräteidentität noch ungeklärt sind, bleibt das System gelb.

Die lokale EcoTracker-API `/v1/json` selbst enthält im dokumentierten Messwertformat keine Modell-, Seriennummer- oder MAC-Kennung. Trotzdem kann ein Speicher beim erstmaligen Hinzufügen zusätzliche Discovery-, Cloud- oder App-Informationen verwenden; deshalb werden solche Systeme erst dann grün, wenn dieser Punkt ausreichend geklärt ist.

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
