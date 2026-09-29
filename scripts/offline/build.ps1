# Builds PineconeMC Offline. -Package also installs a portable copy into dist\PineconeMC-Offline.
param(
    [ValidateSet('Release', 'Debug')][string]$Config = 'Release',
    [switch]$Package
)
. (Join-Path $PSScriptRoot 'env.ps1')
Push-Location $RepoRoot
try {
    if (-not (Test-Path 'build\CMakeCache.txt')) {
        cmake --preset windows_msvc "-DCMAKE_PREFIX_PATH=$env:QT_ROOT_DIR"
        if ($LASTEXITCODE) { throw 'CMake configure failed' }
    }
    cmake --build --preset windows_msvc --config $Config
    if ($LASTEXITCODE) { throw 'Build failed' }
    if ($Package) {
        $dist = Join-Path $RepoRoot 'dist\PineconeMC-Offline'
        if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
        cmake --install build --config $Config --prefix $dist
        if ($LASTEXITCODE) { throw 'Install failed' }
        cmake --install build --config $Config --prefix $dist --component portable
        if ($LASTEXITCODE) { throw 'Portable install failed' }
        Write-Host "Portable build: $dist"
    }
} finally { Pop-Location }
