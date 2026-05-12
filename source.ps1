# Lunar — ESP-IDF environment loader
# Supports: ESP-IDF v5.3 / v5.4, auto-detects installation path
#
# Usage: . .\source.ps1
#        idf.py build

$idfVersions = @("v5.4", "v5.3")
$found = $false

foreach ($ver in $idfVersions) {
    $candidate = "$env:USERPROFILE\esp\esp-idf-$ver\export.ps1"
    if (Test-Path $candidate) {
        Write-Host "Lunar: loading ESP-IDF $ver from $candidate" -ForegroundColor Cyan
        . $candidate
        $found = $true
        break
    }
}

if (-not $found) {
    Write-Host "Lunar: ESP-IDF not found. Install to %USERPROFILE%\esp\esp-idf-v5.4" -ForegroundColor Red
    Write-Host "  https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/get-started/" -ForegroundColor Yellow
}
