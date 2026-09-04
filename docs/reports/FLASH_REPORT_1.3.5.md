# Flash-Bericht 1.3.5 / Flash report 1.3.5

## Messwerte / Measurements

Verglichen wird mit 1.3.5-beta.3 bei unveränderter Partitionstabelle. Der sehr
kleine Flash-Zuwachs stammt aus den schaltbaren Diagrammreihen und der gezielten
Toleranz für den noch offenen aktuellen Historienblock. Die zweite
Dashboard-Canvas samt eigener Zeichenlogik wurde entfernt.

The comparison uses 1.3.5-beta.3 with an unchanged partition table. The very
small flash increase comes from the toggleable chart series and the targeted
grace period for the still-open current history bucket. The second dashboard
canvas and its separate drawing logic were removed.

| Wert / Value | Beta 3 | 1.3.5 | Änderung / Change |
|---|---:|---:|---:|
| Offline Flash | 1,237,852 B | 1,237,976 B | +124 B |
| Offline statisches RAM | 94,100 B | 94,100 B | 0 B |
| Offline USB-BIN | 1,250,640 B | 1,250,768 B | +128 B |
| Offline OTA-Reserve | 125,616 B | 125,488 B | -128 B |
| Factory Flash | 1,110,048 B | 1,110,172 B | +124 B |
| Factory statisches RAM | 93,516 B | 93,516 B | 0 B |
| Factory BIN | 1,122,768 B | 1,122,896 B | +128 B |
| Developer Flash | 1,240,344 B | 1,240,468 B | +124 B |
| Developer statisches RAM | 96,468 B | 96,468 B | 0 B |
| Developer BIN | 1,253,136 B | 1,253,248 B | +112 B |
| Asset-Container belegt / used | 36,360 B | 36,360 B | 0 B |
| Asset-Container frei / free | 29,176 B | 29,176 B | 0 B |

Alle bestehenden Funktionen und Schnittstellen bleiben erhalten. Dashboard,
Historie, MQTT, Modbus TCP, Shelly-/EcoTracker-Kompatibilität, OTA und die
Partitionsaufteilung wurden nicht verändert.

All existing features and interfaces remain available. Dashboard, history,
MQTT, Modbus TCP, Shelly/EcoTracker compatibility, OTA and the partition layout
remain unchanged.

Der Dashboard-Hotfix verändert weder App-Flash noch statisches RAM. Das
aktualisierte `dashboard.js` bleibt innerhalb derselben 256-Byte-Ausrichtung;
der Asset-Container belegt daher weiterhin 36.360 Byte.

The dashboard hotfix changes neither application flash nor static RAM. The
updated `dashboard.js` remains within the same 256-byte alignment, so the asset
container still uses 36,360 bytes.
