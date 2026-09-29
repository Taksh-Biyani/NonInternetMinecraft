# Dot-source me: sets up an x64 MSVC developer environment plus Qt and vcpkg paths.
$ErrorActionPreference = 'Stop'
$script:RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$deps = Join-Path $RepoRoot '.deps'
$env:QT_ROOT_DIR = Join-Path $deps 'Qt\6.10.2\msvc2022_64'
$env:VCPKG_ROOT = Join-Path $deps 'vcpkg'
if (-not (Test-Path (Join-Path $env:QT_ROOT_DIR 'bin\qmake.exe'))) { throw 'Qt missing: run scripts\offline\setup-toolchain.ps1' }
if (-not (Test-Path (Join-Path $env:VCPKG_ROOT 'vcpkg.exe'))) { throw 'vcpkg missing: run scripts\offline\setup-toolchain.ps1' }

if (-not $env:VSCMD_VER) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) { throw 'Visual Studio C++ build tools not found' }
    Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
}
$env:PATH = "$env:QT_ROOT_DIR\bin;$RepoRoot\build\vcpkg_installed\x64-windows\bin;$env:PATH"
