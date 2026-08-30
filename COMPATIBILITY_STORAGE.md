# Kompatibilität – Speicher / Energiemanagement

Stand: 2026-08-30

Diese Liste bewertet **direkt die Netzmessgeräte-Schnittstelle**, die ein Speicher bzw. sein integriertes EMS für Regelung/Eigenverbrauch verwendet.

## Bedeutung der Spalten

- **Kompatibel**: Der Speicher bzw. sein EMS besitzt eine Messgeräte-Schnittstelle, die mit den aktuell implementierten Schnittstellen des IR Trackers grundsätzlich nutzbar ist.
- **Schnittstelle / Hinweis**: Welche Messgeräte-Schnittstelle das System erwartet und warum es derzeit passt oder nicht passt.
- **Getestet**: Das konkrete System wurde mit IR Tracker Offline auf echter Hardware erfolgreich geprüft.

Status:
- 🟢 **Ja** – grundsätzlich passende lokale Schnittstelle vorhanden.
- 🟡 **Möglich / Bindung prüfen** – das System unterstützt z. B. Shelly, verlangt aber nach aktueller Dokumentation zusätzlich Hersteller-/Shelly-App, Account oder Gerätebindung. Die reine lokale IR-Tracker-Emulation ist deshalb noch nicht sicher ausreichend.
- ❌ **Nein, aktuell** – das System erwartet eine andere Schnittstelle, einen eigenen Zähler, RS485/Modbus/CT oder eine proprietäre Lösung, die der IR Tracker derzeit nicht emuliert.

> Wichtig: Der IR Tracker stellt aktuell HTTP/JSON, MQTT sowie Shelly-EM-/Shelly-Pro-EM-kompatible lokale Leseendpunkte bereit. Er ist derzeit **kein Modbus-RTU-/RS485-Zähler und kein allgemeiner Modbus-TCP-Smart-Meter-Server**.

## Speicherliste

| # | Hersteller | Modell/Familie | Kompatibel | Schnittstelle / Hinweis | Getestet |
|---:|---|---|---|---|---|
| 1 | Solakon | ONE | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM; Kommunikation nach Einrichtung lokal im Heimnetz dokumentiert | ☐ |
| 2 | Growatt | NOAH 2000 | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM werden offiziell für intelligenten Eigenverbrauch unterstützt | ☐ |
| 3 | Growatt | NEXA 2000 | 🟢 Ja | Shelly 3EM / Shelly Pro 3EM offiziell als Lastmessung genannt | ☐ |
| 4 | Anker SOLIX | Solarbank 2 E1600 Pro | 🟡 Möglich / Bindung prüfen | Shelly 3EM / Pro 3EM, aber dokumentierte Einrichtung über Shelly Cloud und Shelly-Konto | ☐ |
| 5 | Anker SOLIX | Solarbank 2 E1600 Plus | 🟡 Möglich / Bindung prüfen | Shelly 3EM / Pro 3EM, dokumentierte Shelly-Cloud-Bindung | ☐ |
| 6 | Anker SOLIX | Solarbank 2 E1600 AC | 🟡 Möglich / Bindung prüfen | Shelly-Unterstützung vorhanden; Geräte-/Cloud-Bindung muss mit IR Tracker verifiziert werden | ☐ |
| 7 | Anker SOLIX | Solarbank 3 E2700 Pro | 🟡 Möglich / Bindung prüfen | Drittanbieter-Smart-Meter-Integration; lokale Clone-Kompatibilität nicht bestätigt | ☐ |
| 8 | Anker SOLIX | Solarbank 4 Pro | 🟡 Möglich / Bindung prüfen | Drittanbieter-Smart-Meter-Integration; lokale Clone-Kompatibilität nicht bestätigt | ☐ |
| 9 | EcoFlow | STREAM Ultra | 🟡 Möglich / Bindung prüfen | Shelly 3EM / Pro 3EM unterstützt; Handbuch verlangt Bindung im EcoFlow-System/Account | ☐ |
| 10 | EcoFlow | STREAM Pro | 🟡 Möglich / Bindung prüfen | Shelly 3EM / Pro 3EM unterstützt; Account-/Gerätebindung prüfen | ☐ |
| 11 | EcoFlow | STREAM Max | 🟡 Möglich / Bindung prüfen | STREAM-Smart-Meter-System unterstützt Shelly; Account-/Gerätebindung prüfen | ☐ |
| 12 | EcoFlow | STREAM AC Pro | 🟡 Möglich / Bindung prüfen | STREAM-Smart-Meter-System unterstützt Shelly; Account-/Gerätebindung prüfen | ☐ |
| 13 | EcoFlow | PowerOcean | ❌ Nein, aktuell | Hersteller-Energiemesssystem; keine direkt passende IR-Tracker-Shelly-Meter-Schnittstelle bestätigt | ☐ |
| 14 | EcoFlow | PowerOcean Plus | ❌ Nein, aktuell | Hersteller-Energiemesssystem; keine direkt passende IR-Tracker-Schnittstelle bestätigt | ☐ |
| 15 | EcoFlow | PowerOcean DC Fit | ❌ Nein, aktuell | Hersteller-Energiemesssystem; keine direkt passende IR-Tracker-Schnittstelle bestätigt | ☐ |
| 16 | Zendure | SolarFlow Hyper 2000 | 🟡 Möglich / Bindung prüfen | Zendure unterstützt Shelly in HEMS; Shelly-Autorisierung/Account-Bindung ist dokumentiert | ☐ |
| 17 | Zendure | SolarFlow 800 Pro | 🟡 Möglich / Bindung prüfen | Zendure HEMS/Shelly-Anbindung; lokale Clone-Kompatibilität muss geprüft werden | ☐ |
| 18 | Zendure | SolarFlow 1600 AC+ | 🟡 Möglich / Bindung prüfen | Shelly 3EM / Pro 3EM offiziell unterstützt; Zendure-App-Bindung prüfen | ☐ |
| 19 | Zendure | SolarFlow 2400 AC+ | 🟡 Möglich / Bindung prüfen | Shelly 3EM / Pro 3EM offiziell unterstützt; Zendure-App-Bindung prüfen | ☐ |
| 20 | Zendure | SolarFlow 2400 Pro | 🟡 Möglich / Bindung prüfen | Shelly 3EM / Pro 3EM offiziell unterstützt; Zendure-App-Bindung prüfen | ☐ |
| 21 | Marstek | Venus E 2.0 | 🟡 Prüfung erforderlich | Smart-Meter-Regelung vorhanden; genaue lokale Fremdzähler-Schnittstelle für IR Tracker nicht ausreichend bestätigt | ☐ |
| 22 | Marstek | Venus E 3.0 | 🟡 Prüfung erforderlich | Smart-Meter-Regelung vorhanden; genaue lokale Fremdzähler-Schnittstelle für IR Tracker nicht ausreichend bestätigt | ☐ |
| 23 | Marstek | B2500-D | 🟡 Prüfung erforderlich | Smart-Meter/EMS-Anbindung modellabhängig; keine bestätigte IR-Tracker-kompatible lokale Schnittstelle | ☐ |
| 24 | Hoymiles | MS-A2 | 🟡 Möglich / Bindung prüfen | Shelly Pro 3EM offiziell unterstützt; Aufnahme erfolgt über S-Miles Home, daher Gerätebindung praktisch prüfen | ☐ |
| 25 | Hoymiles | HiBattery 1920 AC | 🟡 Möglich / Bindung prüfen | Shelly Pro 3EM / EcoTracker offiziell als Smart Meter genannt; Kopplungsweg praktisch prüfen | ☐ |
| 26 | Hoymiles | HiBattery 4020 X | 🟡 Prüfung erforderlich | Smart-Meter-Integration vorhanden; konkrete Shelly-/lokale Schnittstelle für dieses Modell noch bestätigen | ☐ |
| 27 | Hoymiles | HiBattery 4020 AC | 🟡 Prüfung erforderlich | Smart-Meter-Integration vorhanden; konkrete Shelly-/lokale Schnittstelle für dieses Modell noch bestätigen | ☐ |
| 28 | Jackery | Navi 2000 | 🟡 Möglich / Bindung prüfen | Jackery nennt Shelly Pro 3EM / Pro EM-50; App-Bindung muss mit IR Tracker verifiziert werden | ☐ |
| 29 | SMA | Home Storage | ❌ Nein, aktuell | Batterie selbst ist kein Grid-Meter-Client; Messung erfolgt über SMA-System/Home Manager/Energy Meter | ☐ |
| 30 | SMA | Home Storage 3.2 | ❌ Nein, aktuell | SMA-Systemzähler/Home Manager, nicht IR-Tracker-Shelly als Netzanschlusspunktzähler | ☐ |
| 31 | SMA | Home Storage 6.5 | ❌ Nein, aktuell | SMA-Systemzähler/Home Manager erforderlich | ☐ |
| 32 | SMA | Home Storage 9.8 | ❌ Nein, aktuell | SMA-Systemzähler/Home Manager erforderlich | ☐ |
| 33 | SMA | Home Storage 13.1 | ❌ Nein, aktuell | SMA-Systemzähler/Home Manager erforderlich | ☐ |
| 34 | SMA | Home Storage 16.4 | ❌ Nein, aktuell | SMA-Systemzähler/Home Manager erforderlich | ☐ |
| 35 | BYD | Battery-Box Premium HVS 5.1 | ❌ Nein, aktuell | Batterie kommuniziert mit kompatiblem Wechselrichter; Netzmeter-Schnittstelle liegt beim Wechselrichter/EMS | ☐ |
| 36 | BYD | Battery-Box Premium HVS 7.7 | ❌ Nein, aktuell | abhängig vom Wechselrichter/EMS | ☐ |
| 37 | BYD | Battery-Box Premium HVS 10.2 | ❌ Nein, aktuell | abhängig vom Wechselrichter/EMS | ☐ |
| 38 | BYD | Battery-Box Premium HVS 12.8 | ❌ Nein, aktuell | abhängig vom Wechselrichter/EMS | ☐ |
| 39 | BYD | Battery-Box Premium HVM 8.3 | ❌ Nein, aktuell | abhängig vom Wechselrichter/EMS | ☐ |
| 40 | BYD | Battery-Box Premium HVM 11.0 | ❌ Nein, aktuell | abhängig vom Wechselrichter/EMS | ☐ |
| 41 | BYD | Battery-Box Premium HVM 13.8 | ❌ Nein, aktuell | abhängig vom Wechselrichter/EMS | ☐ |
| 42 | BYD | Battery-Box Premium HVM 16.6 | ❌ Nein, aktuell | abhängig vom Wechselrichter/EMS | ☐ |
| 43 | BYD | Battery-Box Premium HVM 19.3 | ❌ Nein, aktuell | abhängig vom Wechselrichter/EMS | ☐ |
| 44 | BYD | Battery-Box Premium HVM 22.1 | ❌ Nein, aktuell | abhängig vom Wechselrichter/EMS | ☐ |
| 45 | Huawei | LUNA2000-5-S0 | ❌ Nein, aktuell | Netzmessung über Huawei Smart Power Sensor / Wechselrichter-System, typ. RS485/Modbus | ☐ |
| 46 | Huawei | LUNA2000-10-S0 | ❌ Nein, aktuell | Huawei Smart Power Sensor / Wechselrichter-System | ☐ |
| 47 | Huawei | LUNA2000-15-S0 | ❌ Nein, aktuell | Huawei Smart Power Sensor / Wechselrichter-System | ☐ |
| 48 | Huawei | LUNA2000-7-S1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / Wechselrichter-System | ☐ |
| 49 | Huawei | LUNA2000-14-S1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / Wechselrichter-System | ☐ |
| 50 | Huawei | LUNA2000-21-S1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / Wechselrichter-System | ☐ |
| 51 | Sungrow | SBR064 | ❌ Nein, aktuell | Netzmeter wird vom Sungrow-Wechselrichter per RS485 erwartet | ☐ |
| 52 | Sungrow | SBR096 | ❌ Nein, aktuell | abhängig vom Sungrow-Wechselrichter/RS485-Meter | ☐ |
| 53 | Sungrow | SBR128 | ❌ Nein, aktuell | abhängig vom Sungrow-Wechselrichter/RS485-Meter | ☐ |
| 54 | Sungrow | SBR160 | ❌ Nein, aktuell | abhängig vom Sungrow-Wechselrichter/RS485-Meter | ☐ |
| 55 | Sungrow | SBR192 | ❌ Nein, aktuell | abhängig vom Sungrow-Wechselrichter/RS485-Meter | ☐ |
| 56 | Sungrow | SBR224 | ❌ Nein, aktuell | abhängig vom Sungrow-Wechselrichter/RS485-Meter | ☐ |
| 57 | Sungrow | SBR256 | ❌ Nein, aktuell | abhängig vom Sungrow-Wechselrichter/RS485-Meter | ☐ |
| 58 | Sungrow | SBH100 | ❌ Nein, aktuell | abhängig vom Sungrow-Wechselrichter/RS485-Meter | ☐ |
| 59 | Sungrow | SBH150 | ❌ Nein, aktuell | abhängig vom Sungrow-Wechselrichter/RS485-Meter | ☐ |
| 60 | Sungrow | SBH200 | ❌ Nein, aktuell | abhängig vom Sungrow-Wechselrichter/RS485-Meter | ☐ |
| 61 | Fox ESS | ECS2900 | ❌ Nein, aktuell | Batterie abhängig vom Fox-Wechselrichter; dieser erwartet typ. DTSU666 über RS485 | ☐ |
| 62 | Fox ESS | ECS4100 | ❌ Nein, aktuell | abhängig vom Fox-Wechselrichter/RS485-Meter | ☐ |
| 63 | Fox ESS | EP5 | ❌ Nein, aktuell | abhängig vom Fox-Wechselrichter/RS485-Meter | ☐ |
| 64 | Fox ESS | EP11 | ❌ Nein, aktuell | abhängig vom Fox-Wechselrichter/RS485-Meter | ☐ |
| 65 | Fox ESS | EQ3300-5 | ❌ Nein, aktuell | abhängig vom Fox-Wechselrichter/RS485-Meter | ☐ |
| 66 | sonnen | sonnenBatterie 10 | ❌ Nein, aktuell | sonnen-System nutzt eigenes Mess-/Energiemanagement; keine IR-Tracker-Shelly-Meter-Schnittstelle bestätigt | ☐ |
| 67 | sonnen | sonnenBatterie 10 performance | ❌ Nein, aktuell | eigenes sonnen-Mess-/Energiemanagement | ☐ |
| 68 | E3/DC | S10 E | ❌ Nein, aktuell | integriertes E3/DC-System mit eigener Leistungsmessung/EMS | ☐ |
| 69 | E3/DC | S10 E PRO | ❌ Nein, aktuell | eigenes E3/DC-Mess-/Energiemanagement | ☐ |
| 70 | E3/DC | S10 SE | ❌ Nein, aktuell | eigenes E3/DC-Mess-/Energiemanagement | ☐ |
| 71 | E3/DC | S20 X PRO | ❌ Nein, aktuell | eigenes E3/DC-Mess-/Energiemanagement | ☐ |
| 72 | RCT Power | Power Battery 5.7 | ❌ Nein, aktuell | Batterie abhängig vom RCT-Wechselrichter/Power Sensor bzw. Modbus-Meter | ☐ |
| 73 | RCT Power | Power Battery 7.6 | ❌ Nein, aktuell | abhängig vom RCT-Wechselrichter/Meter | ☐ |
| 74 | RCT Power | Power Battery 9.6 | ❌ Nein, aktuell | abhängig vom RCT-Wechselrichter/Meter | ☐ |
| 75 | RCT Power | Power Battery 11.5 | ❌ Nein, aktuell | abhängig vom RCT-Wechselrichter/Meter | ☐ |
| 76 | FENECON | Home 10 | ❌ Nein, aktuell | FEMS/FENECON-System erwartet eigenes bzw. freigegebenes Messkonzept; kein direkter IR-Tracker-Meter-Adapter bestätigt | ☐ |
| 77 | FENECON | Home 20 | ❌ Nein, aktuell | FEMS/FENECON-Messkonzept | ☐ |
| 78 | FENECON | Home 30 | ❌ Nein, aktuell | FEMS/FENECON-Messkonzept | ☐ |
| 79 | SENEC | Home 4 | ❌ Nein, aktuell | SENEC-System mit eigener Messhardware/EMS | ☐ |
| 80 | SENEC | Home P4 | ❌ Nein, aktuell | SENEC-System mit eigener Messhardware/EMS | ☐ |
| 81 | VARTA | pulse neo | ❌ Nein, aktuell | VARTA-Systemmessung; keine passende IR-Tracker-Shelly-Meter-Schnittstelle bestätigt | ☐ |
| 82 | VARTA | element backup | ❌ Nein, aktuell | VARTA-Systemmessung | ☐ |
| 83 | VARTA | VARTA.wall | ❌ Nein, aktuell | abhängig vom VARTA-System/Hybridwechselrichter | ☐ |
| 84 | VARTA | VARTA.hybrid.wall | ❌ Nein, aktuell | VARTA-Systemmessung/Hybrid-EMS | ☐ |
| 85 | Kostal/BYD | PLENTICORE + Battery-Box Premium HVS | ❌ Nein, aktuell | KOSTAL Smart Energy Meter über Modbus RTU/RS485 | ☐ |
| 86 | Fronius | Reserva 6.3 | ❌ Nein, aktuell | GEN24-System nutzt Fronius Smart Meter über Modbus RTU/TCP bzw. Fronius-MQTT-Meterprofil | ☐ |
| 87 | Fronius | Reserva 9.5 | ❌ Nein, aktuell | Fronius Smart Meter / Modbus | ☐ |
| 88 | Fronius | Reserva 12.6 | ❌ Nein, aktuell | Fronius Smart Meter / Modbus | ☐ |
| 89 | Fronius | Reserva 15.8 | ❌ Nein, aktuell | Fronius Smart Meter / Modbus | ☐ |
| 90 | SolarEdge | Home Battery 400V | ❌ Nein, aktuell | SolarEdge Energy Meter über RS485/Modbus | ☐ |
| 91 | SolarEdge | Home Battery 48V | ❌ Nein, aktuell | SolarEdge Energy Meter über RS485/Modbus | ☐ |
| 92 | GoodWe | Lynx Home F Plus+ | ❌ Nein, aktuell | abhängig vom GoodWe-Hybridwechselrichter; GM3000/Smart Meter über RS485 | ☐ |
| 93 | GoodWe | Lynx Home U | ❌ Nein, aktuell | abhängig vom GoodWe-Hybridwechselrichter/RS485-Meter | ☐ |
| 94 | GoodWe | Lynx Home F G2 | ❌ Nein, aktuell | abhängig vom GoodWe-Hybridwechselrichter/RS485-Meter | ☐ |
| 95 | Pylontech | Force H2 | ❌ Nein, aktuell | Batterie besitzt keine unabhängige Grid-Meter-Schnittstelle; abhängig vom Wechselrichter | ☐ |
| 96 | Pylontech | Force H3 | ❌ Nein, aktuell | abhängig vom Wechselrichter | ☐ |
| 97 | Pylontech | US5000 | ❌ Nein, aktuell | abhängig vom Wechselrichter | ☐ |
| 98 | Sigenergy | SigenStor BAT 5.0 | ❌ Nein, aktuell | SigenStor-System nutzt eigenes Energy Gateway/Meter-Konzept | ☐ |
| 99 | Sigenergy | SigenStor BAT 8.0 | ❌ Nein, aktuell | SigenStor-System nutzt eigenes Energy Gateway/Meter-Konzept | ☐ |
| 100 | SAX Power | Home Plus | ❌ Nein, aktuell | eigenes AC-Speicher-/Messkonzept; keine passende lokale IR-Tracker-Meter-Schnittstelle bestätigt | ☐ |

## Feldtest

Ein Haken in **Getestet** wird erst gesetzt, wenn das konkrete Speicher-/EMS-System den IR Tracker tatsächlich als Netzmessquelle übernimmt und Bezug, Einspeisung, Vorzeichen, Lastsprünge, Verbindungsverlust und Wiederanlauf korrekt verarbeitet.

`🟡 Möglich / Bindung prüfen` ist bewusst nicht gleichbedeutend mit `🟢 Ja`: Bei mehreren Herstellern ist zwar Shelly-Unterstützung dokumentiert, die Einrichtung erfolgt jedoch über Hersteller-App, Shelly-Account oder Cloud-Autorisierung. Ob ein lokal emulierter Shelly ohne echte Herstelleridentität akzeptiert wird, muss praktisch geprüft werden.
