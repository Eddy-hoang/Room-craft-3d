param(
    [string]$Version = "v1.0.0-rc1",
    [string]$BuildDir = "d:\CG\build\mingw-x64-release",
    [string]$OutputRoot = "d:\CG\dist"
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ExePath = Join-Path $BuildDir "RoomCraft3D.exe"

if (-not (Test-Path $ExePath)) {
    throw "Release executable not found at: $ExePath. Build preset 'mingw-x64-release' first."
}

$PackageName = "RoomCraft3D-$Version-win64"
$StageDir = Join-Path $OutputRoot $PackageName
$ZipPath = Join-Path $OutputRoot "$PackageName.zip"

if (Test-Path $StageDir) { Remove-Item $StageDir -Recurse -Force }
if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
New-Item -ItemType Directory -Path $StageDir -Force | Out-Null

# 1. Executable
Copy-Item $ExePath (Join-Path $StageDir "RoomCraft3D.exe")

# 2. Runtime assets (shaders, fonts, icons, runtime OBJ/MTL/PNG models — exclude .blend authoring sources)
$AssetsDst = Join-Path $StageDir "assets"
New-Item -ItemType Directory -Path $AssetsDst -Force | Out-Null
foreach ($sub in @("shaders", "fonts", "icons", "models")) {
    $srcSub = Join-Path (Join-Path $RepoRoot "assets") $sub
    if (Test-Path $srcSub) {
        $dstSub = Join-Path $AssetsDst $sub
        Copy-Item $srcSub $dstSub -Recurse -Force
    }
}
# Remove non-runtime Blender .blend/.blend1 authoring files from portable runtime package
Get-ChildItem -Path $AssetsDst -Recurse -File | Where-Object { $_.Extension -in @(".blend", ".blend1") } | Remove-Item -Force

# 3. Example projects (.roomcraft)
$ExamplesSrc = Join-Path $RepoRoot "examples"
if (Test-Path $ExamplesSrc) {
    $ExamplesDst = Join-Path $StageDir "examples"
    New-Item -ItemType Directory -Path $ExamplesDst -Force | Out-Null
    Copy-Item (Join-Path $ExamplesSrc "*.roomcraft") $ExamplesDst -Force
}

# 4. User guide, licenses, changelog, README.txt
$DocsDst = Join-Path $StageDir "docs"
New-Item -ItemType Directory -Path $DocsDst -Force | Out-Null
Copy-Item (Join-Path $RepoRoot "docs\EDITOR_USER_GUIDE.md") (Join-Path $DocsDst "EDITOR_USER_GUIDE.md")
Copy-Item (Join-Path $RepoRoot "docs\PROJECT_FORMAT.md") (Join-Path $DocsDst "PROJECT_FORMAT.md")

$LicDst = Join-Path $StageDir "licenses"
New-Item -ItemType Directory -Path $LicDst -Force | Out-Null
Copy-Item (Join-Path $RepoRoot "licenses\THIRD_PARTY_NOTICES.txt") (Join-Path $LicDst "THIRD_PARTY_NOTICES.txt")
Copy-Item (Join-Path $RepoRoot "docs\ASSET_LICENSES.md") (Join-Path $LicDst "ASSET_LICENSES.md")
Copy-Item (Join-Path $RepoRoot "CHANGELOG.md") (Join-Path $StageDir "CHANGELOG.md")

$ReadmeText = @"
RoomCraft 3D ($Version) — Portable Windows x64 Package
======================================================

Quick Start:
1. Extract this folder anywhere (paths with spaces and Unicode characters are supported).
2. Double-click RoomCraft3D.exe (no Blender, CMake, compiler, or external DLLs required).
3. Optional: Open sample designs from examples\ via File -> Open Project (Ctrl+O).

Requirements:
- Windows 10 / 11 (64-bit)
- GPU supporting OpenGL 3.3 Core Profile

User Preferences:
- Saved automatically in %APPDATA%\RoomCraft3D\preferences.json (separate from .roomcraft files).

Documentation & Licenses:
- docs\EDITOR_USER_GUIDE.md
- docs\PROJECT_FORMAT.md
- licenses\THIRD_PARTY_NOTICES.txt
- licenses\ASSET_LICENSES.md
"@
Set-Content -Path (Join-Path $StageDir "README.txt") -Value $ReadmeText -Encoding UTF8

# 5. Generate SHA256SUMS.txt for all packaged files
$SumPath = Join-Path $StageDir "SHA256SUMS.txt"
$Files = Get-ChildItem -Path $StageDir -Recurse -File | Sort-Object FullName
$Lines = foreach ($f in $Files) {
    $hash = (Get-FileHash -Path $f.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $rel = $f.FullName.Substring($StageDir.Length + 1).Replace('\', '/')
    "$hash  $rel"
}
Set-Content -Path $SumPath -Value $Lines -Encoding ASCII

# 6. Create Release Candidate ZIP archive
Compress-Archive -Path $StageDir -DestinationPath $ZipPath -CompressionLevel Optimal
$ZipHash = (Get-FileHash -Path $ZipPath -Algorithm SHA256).Hash.ToLowerInvariant()
$ZipSize = (Get-Item $ZipPath).Length

Write-Host "[PACKAGE] Staged folder : $StageDir ($($Files.Count + 1) files)"
Write-Host "[PACKAGE] ZIP archive   : $ZipPath ($ZipSize bytes)"
Write-Host "[PACKAGE] ZIP SHA-256   : $ZipHash"
