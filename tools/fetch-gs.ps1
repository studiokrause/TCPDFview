# tools/fetch-gs.ps1 — download official Ghostscript Windows binaries and
# assemble the minimal redistributable trees used by TCPDFview installers.
# Run from repo root. Requires 7-Zip (C:\Program Files\7-Zip\7z.exe).
# Output (git-ignored): gs-stage\x86\, gs-stage\x64\ each containing:
#   gsdll??.dll, Resource\, iccprofiles\, lib\
param([string]$Version = "10.08.0")

$ErrorActionPreference = "Stop"
$tag = "gs" + $Version.Replace(".", "")
$tmp = Join-Path $env:TEMP "opencode"
New-Item -ItemType Directory -Path $tmp -Force | Out-Null
$seven = "C:\Program Files\7-Zip\7z.exe"
if (!(Test-Path $seven)) { throw "7-Zip not found at $seven" }

foreach ($arch in @("32", "64")) {
    $exe = Join-Path $tmp ("gs" + $tag + "w" + $arch + ".exe")
    if (!(Test-Path $exe)) {
        $url = "https://github.com/ArtifexSoftware/ghostpdl-downloads/releases/download/$tag/${tag}w$arch.exe"
        Write-Host "Downloading $url ..."
        curl.exe -L -o $exe $url
    }
    $out = Join-Path $tmp ("gs" + $arch)
    Remove-Item -Recurse -Force $out -ErrorAction SilentlyContinue
    & $seven x $exe "-o$out" -y | Out-Null

    $sfx = "x64"; if ($arch -eq "32") { $sfx = "x86" }
    $stage = Join-Path (Join-Path $PSScriptRoot "..\gs-stage") $sfx
    Remove-Item -Recurse -Force $stage -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $stage | Out-Null
    Copy-Item (Join-Path $out "bin\gsdll$arch.dll") (Join-Path $stage "gsdll$arch.dll")
    Copy-Item -Recurse (Join-Path $out "Resource") (Join-Path $stage "Resource")
    Copy-Item -Recurse (Join-Path $out "iccprofiles") (Join-Path $stage "iccprofiles")
    Copy-Item -Recurse (Join-Path $out "lib") (Join-Path $stage "lib")
    Write-Host "Staged: $stage"
}
Write-Host "Done. gs-stage trees ready for packaging."
