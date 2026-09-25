# Installation und Wiederherstellung / Installation and recovery

## Deutsch

### Aktuelles Update auf 2.1.1

1. Einstellungen und vollständige Historie lokal sichern; Stromversorgung stabil halten.
2. Unter **Wartung → Vollständiges Update** die Datei
   `ir-tracker-update-2.1.1.irup` hochladen. Firmware und Weboberfläche gehören
   zusammen; eine separate Asset-Datei ist für den normalen WLAN-Updateweg nicht nötig.
3. Neustart abwarten und anschließend Firmwareversion, Weboberfläche, Messung und
   Asset-Version prüfen.
4. Geräte, die bereits erfolgreich auf 2.0.0 Compact-History migriert wurden,
   benötigen normalerweise nur dieses vollständige 2.1.1-IRUP.
5. Bei einem direkten Sprung von einer älteren, noch nicht auf Compact-History
   migrierten Version gelten weiterhin die Schutzregeln aus 2.0.0: beide
   validierten App-Slots müssen einen kompatiblen Reader enthalten, bevor die
   Migration starten darf. Falls erforderlich, dasselbe vollständige IRUP nach
   dem ersten erfolgreichen Neustart ein zweites Mal installieren.
6. Ein Mischstand aus Firmware 2.1.1 und Webassets 2.0.0 ist kein vollständiger
   2.1.1-Zustand. In diesem Fall das vollständige 2.1.1-IRUP erneut installieren.

Neu in 2.1.1 sind unter anderem automatische Zähler-Inbetriebnahme, Safe-Recovery,
Mess-/History-Service während HTTP-OTA-Uploads, Captive-Portal-Unterstützung,
gemeinsamer LAN-/WLAN-Adminzugang, 30-MHz-W5500-SPI sowie erweiterte Diagnose.
Die vollständige Liste steht in
[RELEASE_NOTES-2.1.1.md](../release/RELEASE_NOTES-2.1.1.md).

**Adminzugang:** LAN und WLAN verwenden dasselbe Admin-Konto. Das aktuell
wirksame Passwort wird in den geschützten Einstellungen 1:1 angezeigt.
Neu gesetzte Admin-Passwörter müssen 6–64 Zeichen lang sein. Der WPA2-
Setup-Hotspot benötigt technisch weiterhin mindestens 8 Zeichen; bei einem
6- oder 7-stelligen Admin-Passwort verwendet der Setup-Hotspot deshalb weiterhin
das ursprüngliche gerätespezifische `IRTracker-XXXX`-Passwort.

**Setup-Hotspot:** Gängige Captive-Portal-Erkennung von Android, iOS/macOS und
Windows wird unterstützt. Falls kein automatisches Portal erscheint, bleibt
`http://192.168.4.1/setup` der direkte lokale Einstieg.

**Wichtig:** Ein normales WLAN-/IRUP-Update verändert die Partitionstabelle nicht.
`.irup` niemals in `.irfw` umbenennen.

### Historischer 2.0.0-Migrationshinweis



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

### Current 2.1.1 upgrade

1. Back up settings and the complete local history and keep power stable.
2. Upload `ir-tracker-update-2.1.1.irup` under **Maintenance → Complete update**.
   Firmware and web UI belong together; no separate asset file is required for
   the normal Wi-Fi update path.
3. Wait for reboot, then verify firmware version, web UI, meter operation and
   asset version.
4. Devices already successfully migrated to 2.0.0 Compact History normally need
   only this one complete 2.1.1 IRUP update.
5. When jumping directly from an older pre-Compact-History version, the 2.0.0
   migration safety rule still applies: both validated application slots must
   contain a compatible reader before migration may start. If required, install
   the same complete IRUP a second time after the first successful reboot.
6. A mixed firmware 2.1.1 / web-assets 2.0.0 state is not a complete 2.1.1
   installation. Reinstall the complete 2.1.1 IRUP in that case.

2.1.1 adds automatic meter commissioning, Safe Recovery, meter/history servicing
during HTTP OTA uploads, captive-portal support, one shared LAN/Wi-Fi
administrator account, 30 MHz W5500 SPI, and extended diagnostics. See
[RELEASE_NOTES-2.1.1.md](../release/RELEASE_NOTES-2.1.1.md) for the full list.

**Administrator access:** LAN and Wi-Fi use the same administrator account. The
currently effective password is displayed exactly in protected Settings.
New administrator passwords must contain 6–64 characters. WPA2 still requires
at least 8 characters for the setup AP, so a 6- or 7-character administrator
password leaves the original per-device `IRTracker-XXXX` setup-AP password in use.

**Setup access point:** Common Android, iOS/macOS and Windows captive-portal
detection is supported. If no automatic portal appears,
`http://192.168.4.1/setup` remains the direct local setup address.

**Important:** A normal Wi-Fi/IRUP update does not replace the partition table.
Never rename `.irup` to `.irfw`.

### Historical 2.0.0 migration note



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
