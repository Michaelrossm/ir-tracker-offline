# Webasset-Partition / Web asset partition

## Deutsch

Die aktuelle Firmware hält die statischen Webassets in der vorhandenen 64-KiB-Asset-Partition bei `0x2B0000`. Neue Partitionstabellen verwenden das Label `debugfs`; bestehende Geräte mit historischem Label `coredump` bleiben kompatibel, **wenn der vorhandene Partitionssubtyp bereits zum erwarteten Asset-/SPIFFS-Layout passt**. Ein echter Coredump-Partitionssubtyp muss vor der Verwendung als Asset-Bereich über den datenerhaltenden USB-Migrationsweg auf das aktuelle Layout gebracht werden. WLAN-Updates verändern die Partitionstabelle nicht.

Die Firmware validiert Asset-Container und benötigte Dateien vor der Auslieferung. Fehlen Assets oder sind sie beschädigt, bleibt eine kleine Recovery-Oberfläche aus der Firmware verfügbar; Messwerterfassung, History und lokale Integrationsschnittstellen sollen davon unabhängig weiterlaufen.

Seit 2.0.0 gehört die Weboberfläche zum signierten vollständigen `.irup`-Updatepfad. Für den normalen WLAN-Upgradeweg wird `ir-tracker-update-2.0.0.irup` verwendet. Das separate `ir-tracker-assets-2.0.0.bin` dient Diagnose/Recovery bzw. dem passenden USB-Installer und ist **nicht** der normale Updateweg.

### Rollback-Schutz in 2.0.0

2.0.0 reserviert im Ende des inaktiven OTA-App-Slots Platz für Asset-Backup und Transaktionsjournal. Dadurch können Asset-Aktualisierungen transaktional abgesichert werden, ohne History-, NVS- oder Einstellungsbereiche zu verschieben. Dieser Schutz kann den allerersten Übergang von einer älteren Firmware nicht rückwirkend absichern; deshalb bleiben Backups vor dem Upgrade empfohlen.

Die History liegt weiterhin getrennt vom Asset-Bereich. Die Compact-History-Migration startet erst, wenn beide validierten OTA-App-Slots die passende 2.0.0-Firmware enthalten. Details stehen in den [Release Notes 2.0.0](../release/RELEASE_NOTES-2.0.0.md) und in der [Installation](INSTALLATION.md).

## English

The current firmware stores its static web assets in the existing 64 KiB asset partition at `0x2B0000`. New partition tables use the `debugfs` label. Existing devices with the historical `coredump` label remain compatible **when the existing partition subtype already matches the expected asset/SPIFFS layout**. A true coredump partition subtype must be migrated to the current layout through the data-preserving USB migration path before it is used for assets. Wi-Fi updates never modify the partition table.

Firmware validates the asset container and required files before serving them. If assets are missing or damaged, a small firmware-resident recovery UI remains available; meter acquisition, history, and local integration interfaces are intended to continue independently.

Starting with 2.0.0, the web interface is part of the signed complete `.irup` update path. Normal Wi-Fi upgrades use `ir-tracker-update-2.0.0.irup`. The separate `ir-tracker-assets-2.0.0.bin` is intended for diagnostics/recovery or the matching USB installer and is **not** the normal update path.

### Rollback protection in 2.0.0

2.0.0 reserves space at the end of the inactive OTA app slot for the asset backup and transaction journal. This protects asset updates transactionally without moving history, NVS, or settings areas. The protection cannot retroactively cover the very first transition from older firmware, so a backup before upgrading remains recommended.

History remains separate from the asset area. Compact-history migration starts only after both validated OTA app slots contain the matching 2.0.0 firmware. See [Release Notes 2.0.0](../release/RELEASE_NOTES-2.0.0.md) and [Installation](INSTALLATION.md) for the upgrade procedure.
