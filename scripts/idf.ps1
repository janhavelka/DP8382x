<#
.SYNOPSIS
Run idf.py with the pinned ESP-IDF release for DP8382x: tag v6.1, commit
fff9895c82d744c7237be8847347bdd1b07c6643 (docs/design-decisions.md).

It uses the side-by-side installation in C:\esp\v6.1 through C:\esp\idf.ps1,
which runs in a child process and never changes user or machine settings, so
PlatformIO (C:\pio, ESP-IDF 5.5) stays untouched. All arguments go to idf.py.

.EXAMPLE
scripts\idf.ps1 -C examples\esp_idf\p4 set-target esp32p4 build
#>
$pinnedVersion = '6.1'
$pinnedCommit = 'fff9895c82d744c7237be8847347bdd1b07c6643'
$launcher = 'C:\esp\idf.ps1'
$checkout = "C:\esp\v$pinnedVersion\esp-idf"

if (-not (Test-Path $launcher) -or -not (Test-Path (Join-Path $checkout '.git'))) {
    Write-Error "ESP-IDF $pinnedVersion is not installed side by side. Run: C:\esp\install-idf.ps1 $pinnedVersion"
    exit 2
}
$head = (& git -C $checkout rev-parse HEAD).Trim()
if ($head -ne $pinnedCommit) {
    Write-Error "C:\esp\v$pinnedVersion is at $head, not the pinned $pinnedCommit."
    exit 2
}
& $launcher $pinnedVersion @args
exit $LASTEXITCODE
