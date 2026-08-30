# Kompatibilitäts-Testmatrix – Stromzähler

Stand: 2026-08-30

Diese Liste ist eine **Prüf- und Freigabeliste**, keine pauschale Kompatibilitätszusage.

Status:
- ✅ `verifiziert`: mit IR Tracker Offline auf echter Hardware geprüft.
- 🟢 `Protokoll passend`: technische Voraussetzungen passen, aber das konkrete Modell wurde noch nicht vollständig im Feldtest freigegeben.
- ⬜ `ungetestet`: muss mit echter Hardware geprüft werden.
- ❌ `nicht kompatibel`: aktuell nicht unterstützt.

## Mindestumfang

Alle Stromzähler, die Solakon am 30.08.2026 im PowerTracker-IR-Kompatibilitätscheck aufführt, müssen mindestens in diese Matrix aufgenommen und vor einer offiziellen Freigabe geprüft werden.

Quelle: https://www.solakon.de/products/solakon-ir-meter

Die Firmware unterstützt grundsätzlich SML sowie passives und aktives IEC 62056-21/D0. Ein Modell gilt trotzdem erst nach einem echten Test als `✅ verifiziert`.

## Testkriterien pro Zähler

Für eine Freigabe müssen mindestens geprüft werden: optische Kopplung, ggf. PIN/Freischaltung, Protokollerkennung, gültige CRC/BCC soweit vorhanden, Bezug 1.8.x, Einspeisung 2.8.x, Momentanleistung, Vorzeichen, Updateintervall, verfügbare Phasenwerte, 30 Minuten Dauerbetrieb, Wiederanlauf nach Signalverlust und `age/fresh/stale`-Verhalten.

## Liste

| # | Hersteller | Modell | Status | Hinweis |
|---:|---|---|---|---|
| 1 | Apator | APOX+ | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 2 | Apator | Lepus | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 3 | Apator | Norax 3D | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 4 | Apator | Picus | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 5 | Baylan | BM xx | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 6 | Digimeto | GS303 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 7 | DZG | DVS7420 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 8 | DZG | DVS7612 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 9 | DZG | DWS7412 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 10 | DZG | DWS7420 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 11 | DZG | DWS7612 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 12 | DZG | DWS7410 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 13 | DZG | DWSB12 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 14 | DZG | DWSB20 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 15 | DZG | DWSE20 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 16 | DZG | DWZE12 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 17 | DZG | WS7612 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 18 | EasyMeter | M60 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 19 | EasyMeter | Q1A | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 20 | EasyMeter | Q3A | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 21 | EasyMeter | Q3B | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 22 | EasyMeter | Q3C | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 23 | EasyMeter | Q3D | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 24 | EasyMeter | Q3M | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 25 | eBZ | DD3 * ODZ1 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 26 | eBZ | DD3 * SMZ1 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 27 | eBZ | DD3 * SUZ1 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 28 | eBZ | MD3 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 29 | EFR | SGM-C2 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 30 | EFR | SGM-C4 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 31 | EFR | SGM-C8 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 32 | EFR | SGM-D4 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 33 | EFR | SGM-DD | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 34 | Elster | AS1440 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 35 | Elster | AS2020 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 36 | Elster | AS3500 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 37 | EMH | eBZD | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 38 | EMH | ED300L | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 39 | EMH | ED300S | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 40 | EMH | eHZ | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 41 | EMH | mMe4.0 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 42 | Hager | EHZ363 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 43 | Hausheld | HBZ100 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 44 | Holley | DDZ285 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 45 | Holley | DTZ541 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 46 | Holley | EHZ541 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 47 | Honeywell | AS1440 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 48 | Honeywell | AS2020 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 49 | Honeywell | AS3500 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 50 | Iskra | MT 175 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 51 | Iskra | MT 176 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 52 | Iskra | MT 382 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 53 | Iskra | MT 631 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 54 | Iskra | MT 681 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 55 | Iskra | MT 691 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 56 | Itron | 3.HZ | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 57 | Itron | eHZ | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 58 | Itron | HZ1 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 59 | KAIFA | MB310 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 60 | Landis + Gyr | E320 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 61 | Landis + Gyr | E220 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 62 | Landis + Gyr | E230 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 63 | Landis + Gyr | E350 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 64 | Landis + Gyr | E650 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 65 | Landis + Gyr | ZMB120 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 66 | Latronic | L20 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 67 | Latronic | L30 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 68 | Logarex | LK13BE | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 69 | Logarex | LK13BE803319 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 70 | Logarex | LK13BE803xxx | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 71 | Metcom | MCS301 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 72 | Sagemcom | Smarty BZ-Plus | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 73 | Siemens | TD-3511 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 74 | ZPA | GH302 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 75 | ZPA | GH305 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |
| 76 | ZPA | GS303 | ⬜ ungetestet | Solakon-Referenzliste 2026; mit echter Hardware prüfen |

## Aufnahme weiterer Zähler

Weitere in Deutschland häufige Zähler sollen ergänzt werden, auch wenn sie nicht in der Solakon-Liste stehen. Deutschland hat Vorrang; danach Österreich, Schweiz, Benelux, Frankreich, Italien, Spanien und weitere EU-Märkte.

## Nachweis

Bei einer Freigabe sollen Datum, Firmwareversion, Zähler-Firmware soweit sichtbar, Netzbetreiber, benötigte PIN/Freischaltung, erkannte OBIS-Werte, Telegrammintervall und ein kurzer Testnachweis dokumentiert werden.
