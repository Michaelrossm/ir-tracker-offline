# Kompatibilitäts-Testmatrix – PV-Wechselrichter / Mikrowechselrichter

Stand: 2026-08-30

Diese Datei ist ein **priorisierter Test-Backlog**, keine Kompatibilitätszusage. Deutschland wird zuerst priorisiert; danach folgen weitere in der EU verbreitete Systeme. Exakte öffentliche Stückzahl-Ranglisten für 100 Einzelmodelle existieren nicht. Die Reihenfolge ist deshalb eine Testpriorisierung, kein behauptetes Verkaufsranking.

Status: ✅ verifiziert · 🟢 lokal kompatibel · 🟡 Teiltest · ⬜ ungetestet · ❌ nicht kompatibel.

## Wichtige Abgrenzung
Der IR Tracker ist ein lesendes Grid-Meter-Gateway. Ein Wechselrichter gilt nur dann als kompatibel, wenn dessen Wechselrichter/EMS den IR Tracker tatsächlich als Netzmessquelle akzeptiert. Dass ein Hersteller grundsätzlich externe Smart Meter unterstützt, genügt nicht.

## Testkriterien
Gerätemodell/Firmware, verwendete lokale Schnittstelle, Discovery, Leistungswert/Vorzeichen, Bezug/Einspeisung, Phasenwerte, Reaktionszeit, stale/age-Verhalten, Netzwerkunterbrechung, Wiederanlauf und – falls damit geregelt wird – Lastsprünge in beide Richtungen dokumentieren.

## 100 Testkandidaten

| # | Hersteller | Modell/Familie | Klasse | Priorität | Status |
|---:|---|---|---|:---:|---|
| 1 | SMA | Sunny Boy 3.0 | String 1ph | A | ⬜ ungetestet |
| 2 | SMA | Sunny Boy 3.6 | String 1ph | A | ⬜ ungetestet |
| 3 | SMA | Sunny Boy 4.0 | String 1ph | A | ⬜ ungetestet |
| 4 | SMA | Sunny Boy 5.0 | String 1ph | A | ⬜ ungetestet |
| 5 | SMA | Sunny Boy 6.0 | String 1ph | A | ⬜ ungetestet |
| 6 | SMA | Sunny Boy Smart Energy 3.6 | Hybrid 1ph | A | ⬜ ungetestet |
| 7 | SMA | Sunny Boy Smart Energy 4.0 | Hybrid 1ph | A | ⬜ ungetestet |
| 8 | SMA | Sunny Boy Smart Energy 5.0 | Hybrid 1ph | A | ⬜ ungetestet |
| 9 | SMA | Sunny Boy Smart Energy 6.0 | Hybrid 1ph | A | ⬜ ungetestet |
| 10 | SMA | Sunny Tripower 5.0 | String 3ph | A | ⬜ ungetestet |
| 11 | SMA | Sunny Tripower 6.0 | String 3ph | A | ⬜ ungetestet |
| 12 | SMA | Sunny Tripower 8.0 | String 3ph | A | ⬜ ungetestet |
| 13 | SMA | Sunny Tripower 10.0 | String 3ph | A | ⬜ ungetestet |
| 14 | SMA | Sunny Tripower Smart Energy 5.0 | Hybrid 3ph | A | ⬜ ungetestet |
| 15 | SMA | Sunny Tripower Smart Energy 6.0 | Hybrid 3ph | A | ⬜ ungetestet |
| 16 | SMA | Sunny Tripower Smart Energy 8.0 | Hybrid 3ph | A | ⬜ ungetestet |
| 17 | SMA | Sunny Tripower Smart Energy 10.0 | Hybrid 3ph | A | ⬜ ungetestet |
| 18 | Fronius | Primo GEN24 3.0 Plus | Hybrid 1ph | A | ⬜ ungetestet |
| 19 | Fronius | Primo GEN24 4.0 Plus | Hybrid 1ph | A | ⬜ ungetestet |
| 20 | Fronius | Primo GEN24 5.0 Plus | Hybrid 1ph | A | ⬜ ungetestet |
| 21 | Fronius | Primo GEN24 6.0 Plus | Hybrid 1ph | A | ⬜ ungetestet |
| 22 | Fronius | Symo GEN24 6.0 Plus | Hybrid 3ph | A | ⬜ ungetestet |
| 23 | Fronius | Symo GEN24 8.0 Plus | Hybrid 3ph | A | ⬜ ungetestet |
| 24 | Fronius | Symo GEN24 10.0 Plus | Hybrid 3ph | A | ⬜ ungetestet |
| 25 | Fronius | Symo 8.2-3-M | String 3ph | A | ⬜ ungetestet |
| 26 | Fronius | Symo 10.0-3-M | String 3ph | A | ⬜ ungetestet |
| 27 | Kostal | PLENTICORE plus G2 5.5 | Hybrid 3ph | A | ⬜ ungetestet |
| 28 | Kostal | PLENTICORE plus G2 7.0 | Hybrid 3ph | A | ⬜ ungetestet |
| 29 | Kostal | PLENTICORE plus G2 8.5 | Hybrid 3ph | A | ⬜ ungetestet |
| 30 | Kostal | PLENTICORE plus G2 10 | Hybrid 3ph | A | ⬜ ungetestet |
| 31 | Kostal | PLENTICORE G3 S | Hybrid 3ph | A | ⬜ ungetestet |
| 32 | Kostal | PLENTICORE G3 M | Hybrid 3ph | A | ⬜ ungetestet |
| 33 | Kostal | PLENTICORE G3 L | Hybrid 3ph | A | ⬜ ungetestet |
| 34 | Kostal | PIKO IQ 4.2 | String 3ph | A | ⬜ ungetestet |
| 35 | Kostal | PIKO IQ 5.5 | String 3ph | A | ⬜ ungetestet |
| 36 | Kostal | PIKO IQ 7.0 | String 3ph | A | ⬜ ungetestet |
| 37 | Kostal | PIKO IQ 8.5 | String 3ph | A | ⬜ ungetestet |
| 38 | Kostal | PIKO IQ 10 | String 3ph | A | ⬜ ungetestet |
| 39 | Huawei | SUN2000-2KTL-L1 | Hybrid/String 1ph | A | ⬜ ungetestet |
| 40 | Huawei | SUN2000-3KTL-L1 | Hybrid/String 1ph | A | ⬜ ungetestet |
| 41 | Huawei | SUN2000-4KTL-L1 | Hybrid/String 1ph | A | ⬜ ungetestet |
| 42 | Huawei | SUN2000-5KTL-L1 | Hybrid/String 1ph | A | ⬜ ungetestet |
| 43 | Huawei | SUN2000-6KTL-L1 | Hybrid/String 1ph | A | ⬜ ungetestet |
| 44 | Huawei | SUN2000-3KTL-M1 | Hybrid/String 3ph | A | ⬜ ungetestet |
| 45 | Huawei | SUN2000-4KTL-M1 | Hybrid/String 3ph | A | ⬜ ungetestet |
| 46 | Huawei | SUN2000-5KTL-M1 | Hybrid/String 3ph | A | ⬜ ungetestet |
| 47 | Huawei | SUN2000-6KTL-M1 | Hybrid/String 3ph | A | ⬜ ungetestet |
| 48 | Huawei | SUN2000-8KTL-M1 | Hybrid/String 3ph | A | ⬜ ungetestet |
| 49 | Huawei | SUN2000-10KTL-M1 | Hybrid/String 3ph | A | ⬜ ungetestet |
| 50 | Sungrow | SH5.0RT | Hybrid 3ph | A | ⬜ ungetestet |
| 51 | Sungrow | SH6.0RT | Hybrid 3ph | A | ⬜ ungetestet |
| 52 | Sungrow | SH8.0RT | Hybrid 3ph | A | ⬜ ungetestet |
| 53 | Sungrow | SH10RT | Hybrid 3ph | A | ⬜ ungetestet |
| 54 | Sungrow | SH15T | Hybrid 3ph | A | ⬜ ungetestet |
| 55 | Sungrow | SH20T | Hybrid 3ph | A | ⬜ ungetestet |
| 56 | Sungrow | SH25T | Hybrid 3ph | A | ⬜ ungetestet |
| 57 | GoodWe | GW5KN-ET Plus+ | Hybrid 3ph | A | ⬜ ungetestet |
| 58 | GoodWe | GW6.5KN-ET Plus+ | Hybrid 3ph | A | ⬜ ungetestet |
| 59 | GoodWe | GW8KN-ET Plus+ | Hybrid 3ph | A | ⬜ ungetestet |
| 60 | GoodWe | GW10KN-ET Plus+ | Hybrid 3ph | A | ⬜ ungetestet |
| 61 | GoodWe | GW5K-ET G2 | Hybrid 3ph | A | ⬜ ungetestet |
| 62 | GoodWe | GW8K-ET G2 | Hybrid 3ph | A | ⬜ ungetestet |
| 63 | GoodWe | GW10K-ET G2 | Hybrid 3ph | A | ⬜ ungetestet |
| 64 | SolarEdge | SE5K-RWB Home Hub | Hybrid 3ph | A | ⬜ ungetestet |
| 65 | SolarEdge | SE7K-RWB Home Hub | Hybrid 3ph | A | ⬜ ungetestet |
| 66 | SolarEdge | SE8K-RWB Home Hub | Hybrid 3ph | A | ⬜ ungetestet |
| 67 | SolarEdge | SE10K-RWB Home Hub | Hybrid 3ph | A | ⬜ ungetestet |
| 68 | SolarEdge | SE5K | String/Optimizer 3ph | A | ⬜ ungetestet |
| 69 | SolarEdge | SE7K | String/Optimizer 3ph | A | ⬜ ungetestet |
| 70 | SolarEdge | SE10K | String/Optimizer 3ph | A | ⬜ ungetestet |
| 71 | RCT Power | Power Inverter 4.0 | String 3ph | A | ⬜ ungetestet |
| 72 | RCT Power | Power Inverter 6.0 | String 3ph | A | ⬜ ungetestet |
| 73 | RCT Power | Power Storage DC 4.0 | Hybrid 3ph | A | ⬜ ungetestet |
| 74 | RCT Power | Power Storage DC 6.0 | Hybrid 3ph | A | ⬜ ungetestet |
| 75 | RCT Power | Power Storage DC 8.0 | Hybrid 3ph | A | ⬜ ungetestet |
| 76 | RCT Power | Power Storage DC 10.0 | Hybrid 3ph | A | ⬜ ungetestet |
| 77 | Fox ESS | H3-5.0-E | Hybrid 3ph | A | ⬜ ungetestet |
| 78 | Fox ESS | H3-8.0-E | Hybrid 3ph | A | ⬜ ungetestet |
| 79 | Fox ESS | H3-10.0-E | Hybrid 3ph | A | ⬜ ungetestet |
| 80 | Fox ESS | H3-12.0-E | Hybrid 3ph | A | ⬜ ungetestet |
| 81 | Growatt | MIN 3000TL-XH | Hybrid/String 1ph | A | ⬜ ungetestet |
| 82 | Growatt | MIN 4600TL-XH | Hybrid/String 1ph | A | ⬜ ungetestet |
| 83 | Growatt | MIN 6000TL-XH | Hybrid/String 1ph | A | ⬜ ungetestet |
| 84 | Growatt | MOD 5KTL3-XH | Hybrid/String 3ph | A | ⬜ ungetestet |
| 85 | Growatt | MOD 8KTL3-XH | Hybrid/String 3ph | A | ⬜ ungetestet |
| 86 | Growatt | MOD 10KTL3-XH | Hybrid/String 3ph | A | ⬜ ungetestet |
| 87 | Deye | SUN-5K-SG04LP3-EU | Hybrid 3ph LV | A | ⬜ ungetestet |
| 88 | Deye | SUN-8K-SG04LP3-EU | Hybrid 3ph LV | A | ⬜ ungetestet |
| 89 | Deye | SUN-10K-SG04LP3-EU | Hybrid 3ph LV | A | ⬜ ungetestet |
| 90 | Hoymiles | HMS-800W-2T | Microinverter | A | ⬜ ungetestet |
| 91 | Hoymiles | HMS-800W-T2 | Microinverter | A | ⬜ ungetestet |
| 92 | Hoymiles | HMT-2250-6T | Microinverter 3ph | A | ⬜ ungetestet |
| 93 | APsystems | EZ1-M | Microinverter | A | ⬜ ungetestet |
| 94 | APsystems | DS3-S | Microinverter | A | ⬜ ungetestet |
| 95 | APsystems | DS3-L | Microinverter | A | ⬜ ungetestet |
| 96 | TSUN | TSOL-MS800 | Microinverter | A | ⬜ ungetestet |
| 97 | Enphase | IQ8MC | Microinverter | A | ⬜ ungetestet |
| 98 | Enphase | IQ8AC | Microinverter | A | ⬜ ungetestet |
| 99 | SolaX | X3-Hybrid G4 | Hybrid 3ph | B | ⬜ ungetestet |
| 100 | Solis | S6-EH3P10K-H-EU | Hybrid 3ph | B | ⬜ ungetestet |

## Priorisierung
`A` = zuerst in Deutschland testen. `B` = danach EU-/Bestandsmarkt.

Die Liste soll anhand echter Feldtests und belastbarer Marktdaten weiter gepflegt werden. **Nicht getestete Geräte dürfen nicht als kompatibel beworben werden.**
