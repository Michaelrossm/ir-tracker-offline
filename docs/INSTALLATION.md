# Installation und Wiederherstellung / Installation and recovery

## Deutsch

### Vorher sichern

1. Persönliche vollständige 4-MiB-Gerätesicherung lokal aufbewahren und SHA-256 notieren.
2. Unter **Wartung** Einstellungen und Historie herunterladen.
3. Stabile USB- oder WLAN-Verbindung und Stromversorgung sicherstellen.
4. Die Original-Sicherung niemals in Git, Releases, Foren oder Clouds veröffentlichen.

### Erstinstallation über USB

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\flash-custom.ps1 `
  -Port COM3 -Version 1.3.8 -ConfirmCustomOnly ERASE-ORIGINAL
```

Der Vorgang installiert die Custom-Firmware redundant. Er darf nur mit vorhandener persönlicher Originalsicherung ausgeführt werden.

### Signiertes WLAN-Update auf 1.3.8

**Aktueller Stand mit Ein-Datei-Update:** Unter **Wartung → Vollständiges
Update** `ir-tracker-update-1.3.8.irup` auswählen. Der Browser lädt zuerst eine
Sicherung der Asset-Partition herunter. Der Tracker prüft danach Signatur,
Manifest, Firmware und Webassets und aktiviert den neuen OTA-Slot erst, wenn
beide Inhalte vollständig geschrieben und geprüft wurden.

**Älterer Stand mit ausschließlich `.irfw`:** Zuerst unter **Wartung →
Custom-Firmware aktualisieren** `ir-tracker-custom-1.3.8.irfw` installieren.
Nach dem Neustart die Wartungsseite neu laden und anschließend
`ir-tracker-update-1.3.8.irup` installieren. Das erneute Schreiben derselben
Firmware ist beabsichtigt; dabei wird zusätzlich die exakt passende
Weboberfläche installiert.

**Sehr alter Stand ohne signiertes WLAN-Update oder mit unbekannter
Partitionstabelle:** Den aktuellen USB-Installer verwenden. Die Migration ist
nur erlaubt, nachdem ein vollständiges, bytegenau verifiziertes Backup erstellt
und die bekannte 4-MiB-Partitionierung erkannt wurde. NVS, History und
OTA-Offets bleiben unverändert; nur das vorhandene Gebiet `0x2B0000–0x2BFFFF`
wird von `coredump` zu `debugfs` migriert.

Einstellungen, Historie und der offene Minutenblock bleiben bei WLAN-Updates
erhalten. Ein vorheriges Backup bleibt trotzdem empfohlen.

Ein normales OTA-Update ersetzt die Partitionstabelle nicht. Geräte mit dem
alten Label `coredump` funktionieren deshalb unverändert weiter. Das neue Label
`debugfs` wird erst durch einen USB-Erstflash der aktuellen Partitionstabelle
eingetragen; Offsets und Größen sowie die Historie ändern sich dabei nicht.

### Vollständige Rückkehr zum Original

Nur die eigene, lokal erstellte Sicherung verwenden:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\restore-original.ps1 -Port COM3
```

Dies überschreibt bewusst den vollständigen Flash und entfernt Custom-Firmware, Einstellungen und Historie.

## English

### Back up first

1. Keep a personal complete 4 MiB device image locally and record its SHA-256 hash.
2. Download settings and history under **Maintenance**.
3. Ensure stable USB/Wi-Fi connectivity and power.
4. Never publish the original backup in Git, releases, forums or cloud shares.

### First installation over USB

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\flash-custom.ps1 `
  -Port COM3 -Version 1.3.8 -ConfirmCustomOnly ERASE-ORIGINAL
```

The operation installs the custom firmware redundantly and may only be run when a personal original backup exists.

### Signed Wi-Fi update to 1.3.8

**Current firmware with single-file update:** Upload
`ir-tracker-update-1.3.8.irup` under **Maintenance → Complete update**. The
browser downloads an asset-partition backup first. The tracker verifies the
signature, manifest, firmware and web assets and activates the OTA slot only
after both payloads have passed verification.

**Older firmware offering only `.irfw`:** First install
`ir-tracker-custom-1.3.8.irfw` under **Maintenance → Update custom firmware**.
Wait for the restart, reload Maintenance, then install
`ir-tracker-update-1.3.8.irup`. Rewriting the same app is intentional because
this second step also installs the matching web interface.

**Very old firmware without signed Wi-Fi update or with an unknown partition
table:** Use the current USB installer first. It permits migration only after a
complete byte-verified backup and validation of the known 4 MiB layout. NVS,
history and OTA offsets remain unchanged; only the existing
`0x2B0000–0x2BFFFF` area is migrated from `coredump` to `debugfs`.

A normal OTA update does not replace the partition table. Devices carrying the
legacy `coredump` label therefore continue to work unchanged. The new `debugfs`
label is written only by a USB first flash of the current partition table;
offsets, sizes and history do not change.

### Full return to the original firmware

Use only your own locally created backup:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\restore-original.ps1 -Port COM3
```

This deliberately overwrites the complete flash and removes custom firmware, settings and history.
