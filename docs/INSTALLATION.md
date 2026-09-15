# Installation und Wiederherstellung / Installation and recovery

## Deutsch

### Aktuelles Update auf 2.0.0

1. Einstellungen und vollständige Historie lokal sichern; Stromversorgung stabil halten.
2. Bei vorhandenem **Vollständiges Update (.irup)** die Datei
   `ir-tracker-update-2.0.0.irup` hochladen. Keine separate Asset-Datei ist nötig.
3. Neustart abwarten, Weboberfläche und Messung prüfen.
4. Dieselbe IRUP-Datei noch einmal manuell installieren. Das ist für die History-
   Umstellung nötig: Beide App-Slots müssen dieselbe validierte neue Firmware
   enthalten, damit kein alter Reader nach einem Rückfall Compact-Daten öffnet.
   Ein GitHub-Update allein befüllt zunächst nur einen Slot.
5. Nach dem zweiten Neustart prüft und konvertiert der Tracker die Archive.
   Plausibel eindeutige Duplikate werden aufgelöst; mehrdeutige Archive bleiben
   original. Zeitstempel und Energiezählerstände der übernommenen Werte werden
   nicht verändert; Leistung wird auf 0,1 W verdichtet. Nicht währenddessen abschalten.

Nachweislich beschädigte Datensätze werden nicht übernommen. Deshalb das Backup
aufbewahren; es enthält die Originaldaten vor dieser Bereinigung. Bei fehlendem
Arbeitsspeicher oder Lesefehlern wird nicht blind gelöscht. Große Archive können
beim ersten Konvertieren die Weboberfläche vorübergehend verzögern.

Bei nur IRFW-fähiger Firmware ist zuerst eine nachweislich passende Brückenversion
oder ein datenerhaltender USB-Installer nötig. IRUP niemals in IRFW umbenennen.
Ein echter Coredump-Partitionssubtyp ist für Assets ungeeignet und muss vorab per
geprüfter USB-Migration korrigiert werden. Das alte Label `coredump` mit bereits
passendem SPIFFS-Subtyp ist dagegen zulässig. WLAN ändert keine Partitionstabelle.

Der erste Übergang von alter Firmware besitzt noch nicht rückwirkend den neuen
Asset-Rollback-Schutz. Auch nach Einführung ersetzen Simulationen keinen Nachweis
echter Stromunterbrechungen auf jeder Hardware. Backups daher aufbewahren.

Die folgenden 1.3.8-Anweisungen beschreiben den historischen Übergang; für 2.0.0
gilt der Ablauf oben. Ein USB-Erstflash ist kein datenerhaltendes Bestandsupdate.

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

### Current 2.0.0 upgrade

Back up settings and all history first. Upload `ir-tracker-update-2.0.0.irup`
through **Complete update**, wait for a successful reboot, then manually upload
the same file again. History migration starts only when both validated app slots
contain identical firmware. GitHub update alone initially replaces one slot.
Firmware and assets are included in the single signed file; no partition table
is changed over Wi-Fi. Ambiguous old records remain untouched, and the new
retention policy applies only after successful conversion. Never rename IRUP to
IRFW. Older IRFW-only devices need a verified compatible bridge or a data-preserving
USB migration. A true coredump subtype must be migrated over USB first.
The initial upgrade from old firmware is not retroactively power-cut protected.
The 1.3.8 instructions below are historical, not the current 2.0.0 procedure.

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
