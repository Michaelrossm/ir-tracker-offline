param(
    [string]$Port = "COM3",
    [string]$Version = "2.0.0",
    [string]$ConfirmCustomOnly = ""
)

$ErrorActionPreference = "Stop"
$workspace = Split-Path -Parent $PSScriptRoot
$firmware = Join-Path $workspace "release\ir-tracker-custom-$Version-usb.bin"
$partitions = Join-Path $workspace "release\partitions-$Version.bin"
$originalBackup = Join-Path $workspace "original bin\solakon-powertracker-original-full.bin"

if ($ConfirmCustomOnly -ne "ERASE-ORIGINAL") {
    throw "Abbruch: Bestätigung erforderlich. / Cancelled: confirmation is required."
}
if (-not (Test-Path -LiteralPath $originalBackup) -or
    (Get-Item -LiteralPath $originalBackup).Length -ne 4194304) {
    throw "Persönliche 4-MiB-Sicherung fehlt. / Personal complete 4 MiB backup is missing."
}
foreach ($file in @($firmware, $partitions)) {
    if (-not (Test-Path -LiteralPath $file)) {
        throw "Benötigte Datei fehlt / Required file missing: $file"
    }
}

$resolvedFirmware = (Resolve-Path -LiteralPath $firmware).Path
$resolvedPartitions = (Resolve-Path -LiteralPath $partitions).Path
$resolvedWorkspace = (Resolve-Path -LiteralPath $workspace).Path
if (-not $resolvedFirmware.StartsWith($resolvedWorkspace,
        [System.StringComparison]::OrdinalIgnoreCase) -or
    -not $resolvedPartitions.StartsWith($resolvedWorkspace,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Dateipfad außerhalb des Projekts. / Resolved path is outside the project."
}

Write-Host "Installiere IR Tracker Firmware $Version redundant. / Installing IR Tracker firmware redundantly."
python -m esptool --port $Port write-flash `
    0x8000 $resolvedPartitions `
    0x10000 $resolvedFirmware `
    0x160000 $resolvedFirmware
if ($LASTEXITCODE -ne 0) {
    throw "Installation fehlgeschlagen. / Installation failed."
}

Write-Host "IR Tracker $Version wurde redundant installiert. / IR Tracker $Version was installed redundantly."
