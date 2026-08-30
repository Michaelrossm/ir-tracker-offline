# Kompatibilitäts-Testmatrix – Speicher / Energiemanagement

Stand: 2026-08-30

Diese Datei ist ein **priorisierter Test-Backlog**, keine Kompatibilitätszusage. Die Auswahl priorisiert in Deutschland verbreitete bzw. aktuell stark vermarktete Systeme; erst danach wurden weitere EU-relevante Familien ergänzt. Exakte öffentliche Stückzahl-Ranglisten für 100 Einzelmodelle existieren nicht, deshalb ist die Reihenfolge eine Testpriorisierung und **kein behauptetes Verkaufsranking**.

Status: ✅ verifiziert · 🟢 lokal kompatibel · 🟡 Teiltest · ⬜ ungetestet · ❌ nicht kompatibel.

## Freigaberegel
Ein Gerät darf erst als kompatibel bezeichnet werden, wenn es den IR Tracker als Netzmessquelle tatsächlich nutzt und Leistungsänderungen korrekt übernimmt. Eine Herstellerangabe wie „Shelly unterstützt“ reicht alleine nicht als Nachweis.

## Testkriterien
Zu dokumentieren sind mindestens: Gerätemodell und Firmware, lokale oder Cloud-basierte Kopplung, verwendetes Protokoll/API, Discovery, Gesamtleistung und Vorzeichen, Bezug/Einspeisung, Phasenwerte soweit benötigt, Reaktionszeit, Verhalten bei veralteten Werten, Verbindungsverlust und Wiederanlauf. Bei Regelung zusätzlich Lastsprünge in beide Richtungen und mindestens 60 Minuten stabiler Betrieb.

## 100 Testkandidaten

| # | Hersteller | Modell/Familie | Klasse | Priorität | Status |
|---:|---|---|---|:---:|---|
| 1 | Solakon | ONE | BKW/All-in-One | A | ⬜ ungetestet |
| 2 | Growatt | NOAH 2000 | BKW/DC | A | ⬜ ungetestet |
| 3 | Growatt | NEXA 2000 | BKW | A | ⬜ ungetestet |
| 4 | Anker SOLIX | Solarbank 2 E1600 Pro | BKW | A | ⬜ ungetestet |
| 5 | Anker SOLIX | Solarbank 2 E1600 Plus | BKW | A | ⬜ ungetestet |
| 6 | Anker SOLIX | Solarbank 2 E1600 AC | BKW/AC | A | ⬜ ungetestet |
| 7 | Anker SOLIX | Solarbank 3 E2700 Pro | BKW | A | ⬜ ungetestet |
| 8 | Anker SOLIX | Solarbank 4 Pro | BKW | A | ⬜ ungetestet |
| 9 | EcoFlow | STREAM Ultra | BKW/All-in-One | A | ⬜ ungetestet |
| 10 | EcoFlow | STREAM Pro | BKW/All-in-One | A | ⬜ ungetestet |
| 11 | EcoFlow | STREAM Max | BKW/All-in-One | A | ⬜ ungetestet |
| 12 | EcoFlow | STREAM AC Pro | BKW/AC | A | ⬜ ungetestet |
| 13 | EcoFlow | PowerOcean | Home | A | ⬜ ungetestet |
| 14 | EcoFlow | PowerOcean Plus | Home | A | ⬜ ungetestet |
| 15 | EcoFlow | PowerOcean DC Fit | Home retrofit | A | ⬜ ungetestet |
| 16 | Zendure | SolarFlow Hyper 2000 | BKW | A | ⬜ ungetestet |
| 17 | Zendure | SolarFlow 800 Pro | BKW | A | ⬜ ungetestet |
| 18 | Zendure | SolarFlow 1600 AC+ | BKW/AC | A | ⬜ ungetestet |
| 19 | Zendure | SolarFlow 2400 AC+ | BKW/AC | A | ⬜ ungetestet |
| 20 | Zendure | SolarFlow 2400 Pro | BKW | A | ⬜ ungetestet |
| 21 | Marstek | Venus E 2.0 | BKW/AC | A | ⬜ ungetestet |
| 22 | Marstek | Venus E 3.0 | BKW/AC | A | ⬜ ungetestet |
| 23 | Marstek | B2500-D | BKW | A | ⬜ ungetestet |
| 24 | Hoymiles | MS-A2 | BKW/AC | A | ⬜ ungetestet |
| 25 | Hoymiles | HiBattery 1920 AC | BKW/AC | A | ⬜ ungetestet |
| 26 | Hoymiles | HiBattery 4020 X | BKW/Hybrid | A | ⬜ ungetestet |
| 27 | Hoymiles | HiBattery 4020 AC | BKW/AC | A | ⬜ ungetestet |
| 28 | Jackery | Navi 2000 | BKW/All-in-One | A | ⬜ ungetestet |
| 29 | SMA | Home Storage | Home HV | A | ⬜ ungetestet |
| 30 | SMA | Home Storage 3.2 | Home HV | A | ⬜ ungetestet |
| 31 | SMA | Home Storage 6.5 | Home HV | A | ⬜ ungetestet |
| 32 | SMA | Home Storage 9.8 | Home HV | A | ⬜ ungetestet |
| 33 | SMA | Home Storage 13.1 | Home HV | A | ⬜ ungetestet |
| 34 | SMA | Home Storage 16.4 | Home HV | A | ⬜ ungetestet |
| 35 | BYD | Battery-Box Premium HVS 5.1 | Home HV | A | ⬜ ungetestet |
| 36 | BYD | Battery-Box Premium HVS 7.7 | Home HV | A | ⬜ ungetestet |
| 37 | BYD | Battery-Box Premium HVS 10.2 | Home HV | A | ⬜ ungetestet |
| 38 | BYD | Battery-Box Premium HVS 12.8 | Home HV | A | ⬜ ungetestet |
| 39 | BYD | Battery-Box Premium HVM 8.3 | Home HV | A | ⬜ ungetestet |
| 40 | BYD | Battery-Box Premium HVM 11.0 | Home HV | A | ⬜ ungetestet |
| 41 | BYD | Battery-Box Premium HVM 13.8 | Home HV | A | ⬜ ungetestet |
| 42 | BYD | Battery-Box Premium HVM 16.6 | Home HV | A | ⬜ ungetestet |
| 43 | BYD | Battery-Box Premium HVM 19.3 | Home HV | A | ⬜ ungetestet |
| 44 | BYD | Battery-Box Premium HVM 22.1 | Home HV | A | ⬜ ungetestet |
| 45 | Huawei | LUNA2000-5-S0 | Home HV | A | ⬜ ungetestet |
| 46 | Huawei | LUNA2000-10-S0 | Home HV | A | ⬜ ungetestet |
| 47 | Huawei | LUNA2000-15-S0 | Home HV | A | ⬜ ungetestet |
| 48 | Huawei | LUNA2000-7-S1 | Home HV | A | ⬜ ungetestet |
| 49 | Huawei | LUNA2000-14-S1 | Home HV | A | ⬜ ungetestet |
| 50 | Huawei | LUNA2000-21-S1 | Home HV | A | ⬜ ungetestet |
| 51 | Sungrow | SBR064 | Home HV | A | ⬜ ungetestet |
| 52 | Sungrow | SBR096 | Home HV | A | ⬜ ungetestet |
| 53 | Sungrow | SBR128 | Home HV | A | ⬜ ungetestet |
| 54 | Sungrow | SBR160 | Home HV | A | ⬜ ungetestet |
| 55 | Sungrow | SBR192 | Home HV | A | ⬜ ungetestet |
| 56 | Sungrow | SBR224 | Home HV | A | ⬜ ungetestet |
| 57 | Sungrow | SBR256 | Home HV | A | ⬜ ungetestet |
| 58 | Sungrow | SBH100 | Home HV | A | ⬜ ungetestet |
| 59 | Sungrow | SBH150 | Home HV | A | ⬜ ungetestet |
| 60 | Sungrow | SBH200 | Home HV | A | ⬜ ungetestet |
| 61 | Fox ESS | ECS2900 | Home HV | A | ⬜ ungetestet |
| 62 | Fox ESS | ECS4100 | Home HV | A | ⬜ ungetestet |
| 63 | Fox ESS | EP5 | Home | A | ⬜ ungetestet |
| 64 | Fox ESS | EP11 | Home | A | ⬜ ungetestet |
| 65 | Fox ESS | EQ3300-5 | Home HV | A | ⬜ ungetestet |
| 66 | sonnen | sonnenBatterie 10 | Home AC | A | ⬜ ungetestet |
| 67 | sonnen | sonnenBatterie 10 performance | Home AC | A | ⬜ ungetestet |
| 68 | E3/DC | S10 E | Home All-in-One | A | ⬜ ungetestet |
| 69 | E3/DC | S10 E PRO | Home All-in-One | A | ⬜ ungetestet |
| 70 | E3/DC | S10 SE | Home All-in-One | A | ⬜ ungetestet |
| 71 | E3/DC | S20 X PRO | Home All-in-One | A | ⬜ ungetestet |
| 72 | RCT Power | Power Battery 5.7 | Home HV | A | ⬜ ungetestet |
| 73 | RCT Power | Power Battery 7.6 | Home HV | A | ⬜ ungetestet |
| 74 | RCT Power | Power Battery 9.6 | Home HV | A | ⬜ ungetestet |
| 75 | RCT Power | Power Battery 11.5 | Home HV | A | ⬜ ungetestet |
| 76 | FENECON | Home 10 | Home | A | ⬜ ungetestet |
| 77 | FENECON | Home 20 | Home | A | ⬜ ungetestet |
| 78 | FENECON | Home 30 | Home | A | ⬜ ungetestet |
| 79 | SENEC | Home 4 | Home All-in-One | A | ⬜ ungetestet |
| 80 | SENEC | Home P4 | Home All-in-One | A | ⬜ ungetestet |
| 81 | VARTA | pulse neo | Home AC | A | ⬜ ungetestet |
| 82 | VARTA | element backup | Home AC | A | ⬜ ungetestet |
| 83 | VARTA | VARTA.wall | Home HV | A | ⬜ ungetestet |
| 84 | VARTA | VARTA.hybrid.wall | Home hybrid | A | ⬜ ungetestet |
| 85 | Kostal/BYD | PLENTICORE + Battery-Box Premium HVS | Home hybrid | A | ⬜ ungetestet |
| 86 | Fronius | Reserva 6.3 | Home HV | A | ⬜ ungetestet |
| 87 | Fronius | Reserva 9.5 | Home HV | A | ⬜ ungetestet |
| 88 | Fronius | Reserva 12.6 | Home HV | A | ⬜ ungetestet |
| 89 | Fronius | Reserva 15.8 | Home HV | A | ⬜ ungetestet |
| 90 | SolarEdge | Home Battery 400V | Home HV | A | ⬜ ungetestet |
| 91 | SolarEdge | Home Battery 48V | Home LV | A | ⬜ ungetestet |
| 92 | GoodWe | Lynx Home F Plus+ | Home HV | A | ⬜ ungetestet |
| 93 | GoodWe | Lynx Home U | Home LV | A | ⬜ ungetestet |
| 94 | GoodWe | Lynx Home F G2 | Home HV | A | ⬜ ungetestet |
| 95 | Pylontech | Force H2 | Home HV | B | ⬜ ungetestet |
| 96 | Pylontech | Force H3 | Home HV | B | ⬜ ungetestet |
| 97 | Pylontech | US5000 | Home LV | B | ⬜ ungetestet |
| 98 | Sigenergy | SigenStor BAT 5.0 | Home HV | A | ⬜ ungetestet |
| 99 | Sigenergy | SigenStor BAT 8.0 | Home HV | A | ⬜ ungetestet |
| 100 | SAX Power | Home Plus | Home AC | A | ⬜ ungetestet |

## Priorisierung
`A` = zuerst in Deutschland testen. `B` = danach EU-/Bestandsmarkt.

Die Liste soll laufend anhand echter Feldtests und belastbarer Marktdaten gepflegt werden. **Nicht getestete Geräte dürfen nicht als kompatibel beworben werden.**

## Ziel des IR Trackers
Der IR Tracker bleibt ein lesendes Grid-Meter-Gateway. Er übernimmt keine Schutzfunktion des Speichers und ersetzt keine vom Hersteller zwingend vorgeschriebene zertifizierte Mess- oder Schutzeinrichtung.
