# Flash-Bericht 1.3.5-beta.2 / Flash report 1.3.5-beta.2

## Messwerte / Measurements

Alle Werte wurden mit derselben Toolchain und unveränderter `partitions.csv`
ermittelt. Die App-Reserve ist die Größe des OTA-Slots `0x150000` abzüglich
der tatsächlichen ESP32-App-BIN.

All values use the same toolchain and an unchanged `partitions.csv`. App
reserve is the `0x150000` OTA slot minus the actual ESP32 application BIN.

| Wert / Value | Vorher / Before | Nachher / After | Änderung / Change |
|---|---:|---:|---:|
| Offline Flash | 1,268,358 B | 1,237,436 B | -30,922 B |
| Offline statisches RAM | 94,100 B | 94,100 B | 0 B |
| Offline USB-BIN | 1,281,152 B | 1,250,224 B | -30,928 B |
| Offline OTA-Reserve | 95,104 B | 126,032 B | +30,928 B |
| Factory Flash | 1,140,540 B | 1,109,624 B | -30,916 B |
| Factory statisches RAM | 93,516 B | 93,516 B | 0 B |
| Factory BIN | 1,153,264 B | 1,122,336 B | -30,928 B |
| Developer Flash | 1,270,830 B | 1,239,920 B | -30,910 B |
| Developer statisches RAM | 96,468 B | 96,468 B | 0 B |
| Developer BIN | 1,283,616 B | 1,252,704 B | -30,912 B |
| Vollständig eingebettete gzip-Webassets | 34,430 B | 0 B | -34,430 B |
| Recovery-Seite, ausgelieferte Größe | – | 3,348 B | +3,348 B |
| Asset-Container, belegter Bereich | 3,373 B | 36,360 B | +32,987 B |
| Freier Bereich im 64-kB-Container | 62,163 B | 29,176 B | -32,987 B |

## Maßnahmen / Measures

- Alle neun bestehenden statischen Webassets liegen einmalig gzip-komprimiert
  im verifizierten Rohdatencontainer der vorhandenen 64-kB-Partition.
- Die vollständigen eingebetteten gzip-Doppelkopien wurden entfernt.
- Eine unabhängige Recovery-Seite bleibt in der App und benötigt keine
  externen CSS- oder JavaScript-Dateien.
- Die Pipeline normalisiert, konservativ minifiziert und reproduzierbar mit
  `mtime=0` gzip-komprimiert. Der JSON-Bericht enthält Roh-, Minify-, gzip-,
  Alignment- und Containergrößen je Datei.
- `-fno-rtti` wurde gemessen, sparte 0 Byte und erzeugte Warnungen für C-Dateien;
  die Option wurde deshalb nicht übernommen.

- All nine existing static web assets are stored once, gzip-compressed, in the
  verified raw container within the existing 64-kB partition.
- Full embedded gzip duplicates were removed.
- A self-contained recovery page remains in the application and requires no
  external CSS or JavaScript.
- The pipeline normalizes, conservatively minifies and reproducibly compresses
  with gzip `mtime=0`. Its JSON report records raw, minified, gzip, alignment
  and container sizes per file.
- `-fno-rtti` was measured, saved 0 bytes and emitted warnings for C sources;
  it was therefore not retained.

## Symbolberichte / Symbol reports

Die jeweils 50 größten Symbole sind separat festgehalten:

- `FLASH_SYMBOLS_BEFORE_1.3.5-beta.2.txt`
- `FLASH_SYMBOLS_AFTER_1.3.5-beta.2.txt`

The 50 largest symbols before and after are recorded in the files above.

Vorher gehörten `kDashboardJsGzip` (6,806 B), `kHistoryJsGzip` (6,819 B) und
`kI18nJsGzip` (7,524 B) zu den 20 größten Symbolen. Danach sind diese sowie alle
anderen vollständigen Webasset-Arrays nicht mehr im App-Binary vorhanden.

Before optimization, `kDashboardJsGzip` (6,806 B), `kHistoryJsGzip` (6,819 B)
and `kI18nJsGzip` (7,524 B) were among the 20 largest symbols. Afterwards these
and all other full web asset arrays are absent from the application binary.
