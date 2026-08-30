# Dauertest / Soak test

## Deutsch

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\start-soak-test.ps1 -Hours 72
```

Zu beobachten: Erreichbarkeit, Neustartursache, freier/minimaler Heap, Zählerfrische, Historienfortschritt, WLAN-Signal und Antwortzeiten. Ein öffentlicher Beta-Release benötigt einen dokumentierten mehrtägigen Test auf echter Hardware.

## English

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\start-soak-test.ps1 -Hours 72
```

Observe reachability, restart reason, free/minimum heap, meter freshness, history progress, Wi-Fi signal and response times. A public beta release requires a documented multi-day test on real hardware.
