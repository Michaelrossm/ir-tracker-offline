# USB-Wiederherstellung / USB recovery

## Deutsch

COM-Port anzeigen:

```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Name
```

Custom-Firmware installieren:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\flash-custom.ps1 -Port COM3 -Version 1.0.0-beta.1 -ConfirmCustomOnly ERASE-ORIGINAL
```

Vollständiges persönliches Original wiederherstellen:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\restore-original.ps1 -Port COM3
```

USB während des Schreibens niemals trennen. `restore-original.ps1` schreibt bewusst den gesamten 4-MiB-Flash. Testpunkte nicht ohne dokumentierte Pinbelegung überbrücken.

## English

List serial ports:

```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Name
```

Install custom firmware:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\flash-custom.ps1 -Port COM3 -Version 1.0.0-beta.1 -ConfirmCustomOnly ERASE-ORIGINAL
```

Restore the complete personal original image:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\restore-original.ps1 -Port COM3
```

Never disconnect USB while writing. `restore-original.ps1` deliberately writes the full 4 MiB flash. Do not bridge test pads without a documented pinout.
