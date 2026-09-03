# Webasset-Partition / Web asset partition

## Deutsch

Version 1.3.5-beta.3 legt alle neun statischen Webassets in der vorhandenen
64-kB-Partition `debugfs` beziehungsweise dem alten Label `coredump` ab. Ein
kompakter Rohdatencontainer funktioniert unabhängig davon,
ob die bestehende Partition den alten Core-Dump- oder den neuen SPIFFS-Subtype
trägt. Die Firmware prüft Schema, exakte Asset-Version, Dateigröße und SHA-256.
Nur vollständig geprüfte Dateien werden ausgeliefert. Bei fehlender, falscher
oder beschädigter Partition wird ohne Absturz eine kleine, eigenständige
Recovery-Oberfläche aus der Firmware verwendet. Messung, History und lokale
Schnittstellen laufen dabei weiter.

Der Container enthält `common.css.gz`, `common.js.gz`, `i18n.js.gz`,
`dashboard.js.gz`, `history.js.gz`, `maintenance.js.gz`, `diagnostics.js.gz`,
`setup.html.gz` und `setup.js.gz`. Das Image ist immer exakt 65.536 Byte groß;
36.360 Byte sind belegt und 29.176 Byte bleiben frei. Weder Partitionstabelle
noch History werden dafür verändert.

Das signierte IRFW-Paket aktualisiert weiterhin ausschließlich die App. Das
separate Asset-Image kann kontrolliert per USB oder über die geschützte
WLAN-Wartungsschnittstelle installiert werden. Vor einem WLAN-Schreibvorgang
prüft die Firmware die tatsächlich vorhandene Partitionstabelle, verlangt die
Bestätigung einer geprüften Sicherung und akzeptiert ausschließlich den
bestehenden Bereich bei `0x2B0000` mit exakt 65.536 Byte. Die Partitionstabelle
wird dabei weder geschrieben noch verändert. Der bisherige Inhalt muss vor dem
Überschreiben vollständig gesichert werden. Ohne gültiges Asset-Image bleibt
die Recovery-Seite mit Status, Diagnose, signiertem Firmwareupdate,
Asset-Wiederherstellung und Neustart verfügbar.

## English

Version 1.3.5-beta.3 stores all nine static web assets in the existing 64-kB
partition labelled `debugfs`, with `coredump` retained as the legacy label. A
compact raw container works with both the legacy core-dump subtype and
the newer SPIFFS subtype. Firmware verifies the schema, exact asset version,
file size and SHA-256. Only fully verified files are served. A missing,
incompatible or damaged partition safely falls back to a small self-contained
recovery UI. Meter acquisition, history and local interfaces continue running.

The container holds `common.css.gz`, `common.js.gz`, `i18n.js.gz`,
`dashboard.js.gz`, `history.js.gz`, `maintenance.js.gz`, `diagnostics.js.gz`,
`setup.html.gz` and `setup.js.gz`. The image is always exactly 65,536 bytes;
36,360 bytes are used and 29,176 bytes remain free. Neither the partition table
nor history is changed.

The signed IRFW package still updates the application only. The separate asset
image can be installed in a controlled operation over USB or through the
protected Wi-Fi maintenance endpoint. Before a Wi-Fi write, firmware validates
the partition table actually present on the device, requires confirmation of a
verified backup, and accepts only the existing 65,536-byte region at
`0x2B0000`. This operation never writes or changes the partition table. Existing
contents must be backed up completely before they are overwritten. Without a
valid asset image, the recovery page remains available with status, diagnostics,
signed firmware update, asset restoration and restart functions.
