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
  -Port COM3 -Version 1.0.0-beta.1 -ConfirmCustomOnly ERASE-ORIGINAL
```

Der Vorgang installiert die Custom-Firmware redundant. Er darf nur mit vorhandener persönlicher Originalsicherung ausgeführt werden.

### Signiertes WLAN-Update

Unter **Wartung → Custom-Firmware aktualisieren** ausschließlich `ir-tracker-custom-1.0.0-beta.1.irfw` laden. Signatur und ESP32-Image werden vor Aktivierung geprüft. Einstellungen, Historie und der offene Minutenblock bleiben erhalten; vorheriges Backup bleibt trotzdem empfohlen.

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
  -Port COM3 -Version 1.0.0-beta.1 -ConfirmCustomOnly ERASE-ORIGINAL
```

The operation installs the custom firmware redundantly and may only be run when a personal original backup exists.

### Signed Wi-Fi update

Under **Maintenance → Update custom firmware**, upload only `ir-tracker-custom-1.0.0-beta.1.irfw`. The signature and ESP32 image are verified before activation. Settings, history and the current minute block are preserved; a backup is still recommended.

### Full return to the original firmware

Use only your own locally created backup:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\restore-original.ps1 -Port COM3
```

This deliberately overwrites the complete flash and removes custom firmware, settings and history.
