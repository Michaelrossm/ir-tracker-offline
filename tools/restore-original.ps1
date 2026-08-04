param(
    [string]$Port = "COM3"
)

$ErrorActionPreference = "Stop"
$workspace = Split-Path -Parent $PSScriptRoot
$backup = Join-Path $workspace "original bin\solakon-powertracker-original-full.bin"

if (-not (Test-Path -LiteralPath $backup)) {
    throw "Originalsicherung nicht gefunden / Original backup not found: $backup"
}

$actual = (Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash
$expected = "16C6D5A210E595D3FA8CBBFC01BCC59FB717641025C6B41023FDA4BE15DD69C2"
if ($actual -ne $expected) {
    throw "Falsche SHA-256-Prüfsumme. / Backup has an unexpected SHA-256 checksum."
}

Write-Host "Stelle Originalzustand wieder her ... / Restoring complete original state ..."
esptool --port $Port write-flash 0x0 $backup
if ($LASTEXITCODE -ne 0) { throw "Wiederherstellung fehlgeschlagen. / Restore failed." }
