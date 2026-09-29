# Builds and runs ctest tests. Pass one or more ctest names, e.g.: .\test.ps1 -Tests OfflineMode,Version
param([string[]]$Tests = @('.*'), [ValidateSet('Release', 'Debug')][string]$Config = 'Release')
. (Join-Path $PSScriptRoot 'env.ps1')
Push-Location $RepoRoot
try {
    cmake --build --preset windows_msvc --config $Config
    if ($LASTEXITCODE) { throw 'Build failed' }
    $regex = '^(' + ($Tests -join '|') + ')$'
    ctest --test-dir build -C $Config -R $regex --output-on-failure
    if ($LASTEXITCODE) { throw 'Tests failed' }
} finally { Pop-Location }
