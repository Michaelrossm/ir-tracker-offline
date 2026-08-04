param(
  [ValidateRange(1, 336)]
  [int]$Hours = 72,
  [ValidateRange(5, 300)]
  [int]$IntervalSeconds = 10,
  [string]$Tracker = "http://192.168.178.66"
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$python = (Get-Command python -ErrorAction Stop).Source
$releaseDirectory = Join-Path $projectRoot "release"
$csv = Join-Path $releaseDirectory "soak-test-1.0.0-beta.1-72h.csv"
$stdout = Join-Path $releaseDirectory "soak-test-1.0.0-beta.1-72h.stdout.log"
$stderr = Join-Path $releaseDirectory "soak-test-1.0.0-beta.1-72h.stderr.log"

if (-not (Test-Path -LiteralPath $releaseDirectory -PathType Container)) {
  throw "Release-Ordner fehlt / Release directory missing: $releaseDirectory"
}

$arguments = @(
  (Join-Path $projectRoot "tools\soak-test.py"),
  "--base", $Tracker,
  "--public",
  "--hours", $Hours,
  "--interval", $IntervalSeconds,
  "--output", $csv
)

$process = Start-Process `
  -FilePath $python `
  -ArgumentList $arguments `
  -WorkingDirectory $projectRoot `
  -WindowStyle Hidden `
  -RedirectStandardOutput $stdout `
  -RedirectStandardError $stderr `
  -PassThru

Write-Host "Dauertest gestartet. / Soak test started."
Write-Host "Prozess-ID / Process ID: $($process.Id)"
Write-Host "Messwerte / Readings: $csv"
Write-Host "Abschluss / Completion: $stdout"
Write-Host "Fehler / Errors: $stderr"
