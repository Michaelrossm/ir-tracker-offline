# Flash-Bericht 1.3.6 / Flash report 1.3.6

## Messwerte / Measurements

Verglichen wird mit dem veröffentlichten Stand 1.3.5 bei unveränderter
Partitionstabelle. Die Optimierung betrifft ausschließlich begrenzte
Historienabfragen und deren bestehendes JSON-Streaming.

The comparison uses the published 1.3.5 build with an unchanged partition
layout. The optimization only affects bounded history queries and their
existing JSON streaming.

| Wert / Value | 1.3.5 | 1.3.6 | Änderung / Change |
|---|---:|---:|---:|
| Offline Flash | 1,237,976 B | 1,238,570 B | +594 B |
| Offline statisches RAM | 94,100 B | 94,100 B | 0 B |
| Offline USB-BIN | 1,250,768 B | 1,251,360 B | +592 B |
| Offline OTA-Reserve | 125,488 B | 124,896 B | -592 B |
| Factory Flash | 1,110,172 B | 1,110,764 B | +592 B |
| Factory statisches RAM | 93,516 B | 93,516 B | 0 B |
| Factory BIN | 1,122,896 B | 1,123,488 B | +592 B |
| Developer Flash | 1,240,468 B | 1,241,080 B | +612 B |
| Developer statisches RAM | 96,468 B | 96,468 B | 0 B |
| Developer BIN | 1,253,248 B | 1,253,872 B | +624 B |
| Asset-Container belegt / used | 36,360 B | 36,360 B | 0 B |
| Asset-Container frei / free | 29,176 B | 29,176 B | 0 B |

Der sequenzielle Lesepuffer benötigt während einer Abfrage 768 Byte Stack. Der
größere Streamingblock wird nur während der Antwort dynamisch reserviert und
danach freigegeben. Auf dem Testtracker wurden vor/nach einer Tagesabfrage
127.620 B bzw. 127.024 B freier Heap gemeldet; dauerhaft blieben damit nur
596 B Differenz im laufenden Systemzustand. Das statische RAM ist unverändert.

The sequential read buffer uses 768 bytes of stack during a query. The larger
streaming chunk is dynamically reserved only for the response and released
afterwards. The test tracker reported 127,620 B and 127,024 B free heap before
and after a day query, leaving only a 596 B difference in the running state.
Static RAM is unchanged.

## Hardware-Laufzeit / Hardware runtime

Testgerät: ESP32-C3 über WLAN, API mit unverändertem JSON-Schema.

| Abfrage / Query | Datensätze / Records | Zeit / Time |
|---|---:|---:|
| Aktueller Tag / current day | 1,028 | 0.550 s |
| Vorheriger Tag / previous day | 1,441 | 0.954 s |
| Alter Tag / older day | 97 | 0.147 s |
| Woche / week | 448 | 0.354 s |
| Monat / month | 353 | 0.271 s |
| Jahr / year | 38 | 0.102 s |
| Zeitraum ohne Daten / empty range | 0 | 0.088 s |

Der volle Minutenring erforderte vorher bis zu 2.880 gelesene Records pro
Tagesabfrage. Für die gemessene aktuelle Abfrage liegt der neue algorithmische
Maximalwert bei 1.040 Records: 1.028 Treffer plus höchstens 12 Suchproben. Bei
einem abgeschlossenen Bereich kann zusätzlich höchstens ein 32er-Leseblock bis
zum ersten Record hinter `until` eingelesen werden. Callback-Reihenfolge,
Plausibilitätsprüfung und Abbruchverhalten bleiben erhalten.

The full minute ring previously required up to 2,880 record reads per day
query. For the measured current query, the new algorithmic upper bound is
1,040 records: 1,028 matches plus at most 12 search probes. For a completed
range, at most one additional 32-record block may be read up to the first
record after `until`. Callback order, plausibility validation and stop
semantics are preserved.

History-Dateiformat, Kapazitäten, Auflösungen, API-Felder, URLs, OTA-Slots und
Partitionen wurden nicht verändert.

History file format, capacities, resolutions, API fields, URLs, OTA slots and
partitions were not changed.
