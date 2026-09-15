# IR Tracker Offline 2.0.0

## Deutsch

### Wichtiges Updateverfahren

1. Einstellungen und vollständige Historie lokal sichern und die Sicherung privat aufbewahren.
2. `ir-tracker-update-2.0.0.irup` unter **Wartung → Vollständiges Update** installieren.
3. Neustart abwarten und Weboberfläche sowie Messung prüfen.
4. **Dieselbe IRUP-Datei ein zweites Mal manuell installieren.** Die Compact-History-Migration startet erst, wenn beide validierten OTA-App-Slots dieselbe 2.0.0-Firmware enthalten.
5. Nach dem zweiten Neustart die History-Migration vollständig durchlaufen lassen und das Gerät währenddessen nicht abschalten.

Ältere Geräte, die ausschließlich `.irfw` anbieten, benötigen zuerst eine nachweislich kompatible IRUP-Brückenversion oder den datenerhaltenden USB-Installer. Eine `.irup` niemals in `.irfw` umbenennen. Ein echter Coredump-Partitionssubtyp muss vorab über USB auf das aktuelle Layout migriert werden; ein historisches `coredump`-Label mit bereits passendem SPIFFS-Subtyp ist zulässig. WLAN-Updates ändern die Partitionstabelle nicht.

### Was ist neu in 2.0.0?

- Neue **Compact-History** mit 0,1-W-Leistungsauflösung. Übernommene Energiezählerstände behalten ihre vorhandenen Floatwerte.
- Retention: 1-Minuten-Werte für 24 Stunden, 5-Minuten-Werte für den zweiten Tag, 15-Minuten-Werte bis 460 Tage, 30-Minuten-Werte bis 825 Tage, 60-Minuten-Werte bis 1.190 Tage und anschließend Tageswerte im Ringpuffer mit 3.650 Einträgen.
- Wiederaufnehmbare, gegengeprüfte Migration der Altarchive.
- Eindeutige doppelte Zeitstempel werden anhand plausibler Nachbar- und Energiezählerstände aufgelöst.
- Nachweislich beschädigte Datensätze werden nicht erfunden oder verschoben, sondern bei der geprüften Migration ausgelassen. Mehrdeutige Archive, Lesefehler oder unzureichender Platz erhalten das Originalarchiv.
- Sichere Platzfreigabe und Unterbrechungs-/Wiederaufnahmepfade für die History-Migration.
- Asset-Backup und Transaktionsjournal im reservierten Ende des inaktiven App-Slots, ohne History- oder NVS-Partitionen zu verschieben.
- Verbesserte UART-Bedienung während längerer Netzwerkoperationen, kleinerer MQTT-Puffer, SML-/JSON-Speicheroptimierungen, gehärtetes Modbus-TCP-Framing und robustere mDNS-/Flash-Fehlerpfade.
- WLAN-Zeitplan-NVS-Schlüssel sowie Sicherung/Wiederherstellung korrigiert.
- Dashboard, CSV und Backups unterstützen die zusätzliche History-Stufe.

### Prüfung

- 80 automatisierte Tests erfolgreich.
- Alle drei PlatformIO-Profile erfolgreich gebaut.
- Signiertes vollständiges IRUP-Paket erfolgreich verifiziert.
- 2.0.0 per WLAN auf dem lokalen ESP32-C3 in beide App-Slots installiert und anschließend Weboberfläche, Assets, Messung, Einstellungen und archivierte Werte geprüft.
- App-BIN: 1.237.680 Byte.
- Statisches RAM: 97.004 Byte.
- Nutzbare OTA-Reserve nach reserviertem Asset-Backup/Journal: 68.944 Byte.
- Gemessener freier Heap nach Start: 139.836 Byte; Minimum 122.328 Byte; größte freie Region 114.676 Byte; Stack-High-Water-Mark 3.624 Byte.

### Grenzen

Der Asset-Rollback-Schutz gilt nicht rückwirkend für den ersten Übergang von älterer Firmware. Simulationen und Hosttests ersetzen keine realen Power-Cut-Tests auf jeder Hardware. Backups bleiben empfohlen. Die LAN-/PoE-Platine wurde mit diesem Release noch nicht auf echter Hardware validiert.

### Release-Dateien

Für einen vollständigen öffentlichen Release sollen diese Artefakte vorhanden sein:

- `ir-tracker-update-2.0.0.irup` — normales signiertes WLAN-Gesamtupdate
- `ir-tracker-custom-2.0.0-usb.bin` — App-Image für den passenden USB-Installer
- `ir-tracker-assets-2.0.0.bin` — separates 64-KiB-Webasset-Image für Diagnose/Wiederherstellung
- `partitions-2.0.0.bin` — Partitionstabelle für den passenden USB-Installer
- `ir-tracker-custom-2.0.0-public.zip` — öffentliches Paket inklusive Dokumentation und SHA-256-Prüfsummen

Für normale WLAN-Updates wird ausschließlich die `.irup` benötigt.

---

## English

### Important upgrade procedure

1. Back up settings and the complete history locally and keep the backup private.
2. Install `ir-tracker-update-2.0.0.irup` through **Maintenance → Complete update**.
3. Wait for the reboot and verify the web interface and meter readings.
4. **Manually install the same IRUP file a second time.** Compact-history migration starts only after both validated OTA app slots contain the same 2.0.0 firmware.
5. After the second reboot, allow history migration to finish and do not power the tracker off while it is running.

Older devices that only offer `.irfw` first need a verified compatible IRUP bridge version or the data-preserving USB installer. Never rename `.irup` to `.irfw`. A true coredump partition subtype must be migrated over USB before using the current asset layout; a legacy `coredump` label that already uses the expected SPIFFS subtype is acceptable. Wi-Fi updates do not modify the partition table.

### What is new in 2.0.0?

- New **compact history** with 0.1 W power resolution. Migrated cumulative energy readings keep their existing floating-point values.
- Retention policy: 1-minute values for 24 hours, 5-minute values for the second day, 15-minute values up to 460 days, 30-minute values up to 825 days, 60-minute values up to 1,190 days, followed by daily values in a 3,650-entry ring buffer.
- Resumable and cross-checked migration of legacy history archives.
- Unambiguous duplicate timestamps are resolved using plausible neighboring records and cumulative energy counters.
- Clearly corrupted records are never replaced with invented data or shifted timestamps; they are omitted during verified migration. Ambiguous archives, read errors, or insufficient space preserve the original archive instead.
- Safe space-release and interruption/resume paths for history migration.
- Asset backup plus transaction journal in the reserved end of the inactive app slot without moving the history or NVS partitions.
- More frequent UART servicing during longer network operations, smaller MQTT packet buffer, SML/JSON memory optimizations, hardened Modbus TCP framing, and more robust mDNS/flash error handling.
- Fixed Wi-Fi schedule NVS key and backup/restore handling.
- Dashboard, CSV export, and backups support the additional history stage.

### Verification

- 80 automated tests passed.
- All three PlatformIO profiles built successfully.
- The signed complete IRUP package passed verification.
- 2.0.0 was installed over Wi-Fi into both app slots on the local ESP32-C3 test tracker; the web UI, assets, meter acquisition, unchanged settings, and archived readings were checked afterwards.
- App binary: 1,237,680 bytes.
- Static RAM: 97,004 bytes.
- Usable OTA reserve after the reserved asset-backup/journal area: 68,944 bytes.
- Measured after boot: 139,836 bytes free heap, 122,328 bytes minimum free heap, 114,676-byte largest free block, and 3,624-byte stack high-water mark.

### Limitations

The new asset rollback protection cannot retroactively protect the very first transition from older firmware. Simulations and host tests are not a substitute for real power-cut testing on every hardware revision. Keep backups. The LAN/PoE board has not yet been validated on real hardware with this release.

### Release files

A complete public release should contain:

- `ir-tracker-update-2.0.0.irup` — normal signed complete Wi-Fi update
- `ir-tracker-custom-2.0.0-usb.bin` — app image for the matching USB installer
- `ir-tracker-assets-2.0.0.bin` — separate 64 KiB web-asset image for diagnostics/recovery
- `partitions-2.0.0.bin` — partition table for the matching USB installer
- `ir-tracker-custom-2.0.0-public.zip` — public package including documentation and SHA-256 checksums

Normal Wi-Fi upgrades only require the `.irup` file.
