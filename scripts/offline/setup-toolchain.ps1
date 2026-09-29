# Installs the build toolchain for PineconeMC Offline into <repo>\.deps (Qt via aqtinstall, vcpkg).
# Requires: Python 3 on PATH, Git, Visual Studio Build Tools with the C++ workload.
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$deps = Join-Path $root '.deps'
New-Item -ItemType Directory -Force $deps | Out-Null

$qtVersion = '6.10.2'
$qtDir = Join-Path $deps "Qt\$qtVersion\msvc2022_64"
if (-not (Test-Path (Join-Path $qtDir 'bin\qmake.exe'))) {
    python -m pip install --upgrade aqtinstall
    if ($LASTEXITCODE) { throw 'pip install aqtinstall failed' }
    python -m aqt install-qt windows desktop $qtVersion win64_msvc2022_64 -m qtimageformats qtnetworkauth -O (Join-Path $deps 'Qt')
    if ($LASTEXITCODE) { throw 'Qt install failed' }
}
Write-Host "Qt: $qtDir"

$vcpkg = Join-Path $deps 'vcpkg'
if (-not (Test-Path (Join-Path $vcpkg 'vcpkg.exe'))) {
    if (-not (Test-Path $vcpkg)) { git clone https://github.com/microsoft/vcpkg.git $vcpkg }
    if ($LASTEXITCODE) { throw 'vcpkg clone failed' }
    & (Join-Path $vcpkg 'bootstrap-vcpkg.bat') -disableMetrics
    if ($LASTEXITCODE) { throw 'vcpkg bootstrap failed' }
}
Write-Host "vcpkg: $vcpkg"
