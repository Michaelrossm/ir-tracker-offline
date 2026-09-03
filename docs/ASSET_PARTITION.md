# Webasset-Partition / Web asset partition

## Deutsch

Version 1.3.5-beta.1 führt einen sicheren Prototyp für Webassets in der
optionalen 64-kB-Partition `debugfs` beziehungsweise dem alten Label
`coredump` ein. Die Firmware prüft Schema, exakte Asset-Version, Dateigröße und
SHA-256. Nur vollständig geprüfte Dateien werden ausgeliefert. Bei fehlender,
nicht mountbarer, falscher oder beschädigter Partition wird ohne Absturz der
eingebettete Firmware-Fallback verwendet.

Der Prototyp enthält nur `maintenance.js.gz`. Alle neun gzip-Einzeldateien
belegen zusammen 34.391 Byte, überschreiten als einzelne LittleFS-Dateien wegen
Block- und Metadatenkosten aber die bestehende 64-kB-Partition. Deshalb wurden
weder Partitionstabelle noch History verändert.

Das signierte IRFW-Paket aktualisiert weiterhin ausschließlich die App. Das
separate Asset-Image ist nur für kontrollierte USB-Erstinstallationen auf einer
leeren `debugfs`-Partition vorgesehen. Es darf nicht auf eine bestehende
`coredump`-Partition geschrieben werden, wenn deren Daten erhalten bleiben
sollen. Auch ohne Asset-Image bleibt die vollständige Oberfläche verfügbar.

## English

Version 1.3.5-beta.1 introduces a safe web asset prototype for the optional
64-kB partition labelled `debugfs`, with `coredump` retained as the legacy
label. Firmware verifies the schema, exact asset version, file size and
SHA-256. Only fully verified files are served. A missing, unmountable,
incompatible or damaged partition safely falls back to embedded assets.

The prototype contains `maintenance.js.gz` only. All nine gzip files total
34,391 bytes but exceed the existing 64-kB partition as separate LittleFS
files due to block and metadata overhead. Neither the partition table nor
history was changed.

The signed IRFW package still updates the application only. The separate asset
image is intended solely for controlled USB first installs onto an empty
`debugfs` partition. Do not write it over an existing `coredump` partition when
its contents must be retained. The complete UI remains available without the
asset image.
