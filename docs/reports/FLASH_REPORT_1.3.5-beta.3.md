# Flash-Bericht 1.3.5-beta.3 / Flash report 1.3.5-beta.3

## Messwerte / Measurements

Verglichen wird mit 1.3.5-beta.2 bei unveränderter Partitionstabelle. Die
kleine Zunahme stammt aus der sichtbaren Modbus-Schnittstellenkarte und der
vereinheitlichten Theme-Steuerung.

The comparison uses 1.3.5-beta.2 with an unchanged partition table. The small
increase is caused by the visible Modbus interface card and unified theme
control.

| Wert / Value | Beta 2 | Beta 3 | Änderung / Change |
|---|---:|---:|---:|
| Offline Flash | 1,237,436 B | 1,237,852 B | +416 B |
| Offline statisches RAM | 94,100 B | 94,100 B | 0 B |
| Offline USB-BIN | 1,250,224 B | 1,250,640 B | +416 B |
| Offline OTA-Reserve | 126,032 B | 125,616 B | -416 B |
| Factory Flash | 1,109,624 B | 1,110,048 B | +424 B |
| Factory statisches RAM | 93,516 B | 93,516 B | 0 B |
| Factory BIN | 1,122,336 B | 1,122,768 B | +432 B |
| Developer Flash | 1,239,920 B | 1,240,344 B | +424 B |
| Developer statisches RAM | 96,468 B | 96,468 B | 0 B |
| Developer BIN | 1,252,704 B | 1,253,136 B | +432 B |
| Asset-Container belegt / used | 36,360 B | 36,360 B | 0 B |
| Asset-Container frei / free | 29,176 B | 29,176 B | 0 B |

Gegenüber dem Stand vor der Asset-Auslagerung bleiben im Offline-Build
30.512 Byte App-Flash zusätzlich frei. Alle neun normalen Webassets existieren
weiterhin ausschließlich im 64-kB-Asset-Container; in der App bleibt nur die
kompakte Recovery-Seite.

Compared with the build before asset externalisation, the offline application
still saves 30,512 bytes. All nine normal web assets remain exclusively in the
64-kB asset container; only the compact recovery page stays in the app.
