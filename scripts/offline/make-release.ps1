# Builds the downloadable launcher: dist\release\PineconeMC-Offline-<version>-windows-x64.zip, plus the matching
# PineconeMC-Offline-<version>-source.zip (GPL-3.0: whoever gets the program can get its source).
# The launcher zip is made from a fresh install into build\release-stage, never from a dist copy, so no instances,
# accounts, logs or development files can get in; check-release.ps1 verifies both zips.
# Needs a clean git tree (the source zip must match the build) and fetch-runtimes.ps1 run once.
# Usage: .\make-release.ps1 [-SkipBuild]
param([switch]$SkipBuild)

# Zip entries use '/' (Compress-Archive in PowerShell 5.1 writes '\', which other tools mis-read).
function Write-Zip([string]$Dir, [string]$ZipPath) {
    Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
    if (Test-Path $ZipPath) { Remove-Item $ZipPath }
    $root = (Resolve-Path $Dir).Path.TrimEnd('\') + '\'
    $archive = [IO.Compression.ZipFile]::Open($ZipPath, 'Create')
    try {
        foreach ($f in Get-ChildItem $Dir -Recurse -File -Force) {
            $name = $f.FullName.Substring($root.Length).Replace('\', '/')
            [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $f.FullName, $name, 'Optimal')
        }
    } finally { $archive.Dispose() }
}

. (Join-Path $PSScriptRoot 'env.ps1')
Push-Location $RepoRoot
try {
    if (git status --porcelain --untracked-files=no) { throw 'Commit or stash your changes first: the source zip is made from HEAD.' }
    $runtimes = Join-Path $RepoRoot '.deps\release'
    if (-not (Test-Path "$runtimes\translations\index_v2.json")) { throw 'Run fetch-runtimes.ps1 first.' }
    $cm = Get-Content CMakeLists.txt -Raw
    $version = (('MAJOR', 'MINOR', 'PATCH') | ForEach-Object { [regex]::Match($cm, "set\(Launcher_VERSION_$_ (\d+)\)").Groups[1].Value }) -join '.'

    if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'build.ps1') }
    $stageRoot = Join-Path $RepoRoot 'build\release-stage'
    $stage = Join-Path $stageRoot 'PineconeMC-Offline'
    if (Test-Path $stageRoot) { Remove-Item -Recurse -Force $stageRoot }
    cmake --install build --config Release --prefix $stage
    if ($LASTEXITCODE) { throw 'Install failed' }
    cmake --install build --config Release --prefix $stage --component portable
    if ($LASTEXITCODE) { throw 'Portable install failed' }
    # Debug symbols are for developers only (and contain build paths).
    Get-ChildItem $stage -Recurse -Filter *.pdb | Remove-Item

    # Java runtimes (minus fetch-runtimes' .from stamps), translations, VC++ runtime and licenses.
    Copy-Item "$runtimes\java" "$stage\java" -Recurse
    Get-ChildItem "$stage\java" -Recurse -Force -Filter .from | Remove-Item -Force
    New-Item -ItemType Directory "$stage\translations", "$stage\licenses" | Out-Null
    Copy-Item "$runtimes\translations\*" "$stage\translations"
    $crt = Get-ChildItem (Join-Path $env:VCToolsRedistDir 'x64') -Directory -Filter 'Microsoft.VC*.CRT' | Select-Object -First 1
    if (-not $crt) { throw "No VC++ runtime under $env:VCToolsRedistDir\x64" }
    Copy-Item "$($crt.FullName)\*.dll" $stage
    Copy-Item LICENSE "$stage\licenses\PineconeMC-Offline-LICENSE.txt"
    Copy-Item COPYING.md "$stage\licenses\COPYING.md"

    $check = Join-Path $PSScriptRoot 'check-release.ps1'
    & $check -Stage $stage
    if ($LASTEXITCODE) { throw 'The release stage failed check-release.ps1 (see PROBLEM lines).' }

    $outDir = Join-Path $RepoRoot 'dist\release'
    New-Item -ItemType Directory -Force $outDir | Out-Null
    $zip = Join-Path $outDir "PineconeMC-Offline-$version-windows-x64.zip"
    Write-Zip $stage $zip
    & $check -Zip $zip
    if ($LASTEXITCODE) { throw 'The launcher zip failed check-release.ps1.' }

    # Source: what's committed at HEAD (local, uncommitted files never get in), plus the libnbtplusplus submodule.
    $srcName = "PineconeMC-Offline-$version-source"
    $src = Join-Path $stageRoot "source\$srcName"
    New-Item -ItemType Directory -Force $src | Out-Null
    git archive --format=tar -o "$stageRoot\source.tar" HEAD
    if ($LASTEXITCODE) { throw 'git archive failed' }
    tar -xf "$stageRoot\source.tar" -C $src
    git -C libraries/libnbtplusplus archive --format=tar -o "$stageRoot\nbt.tar" HEAD
    if ($LASTEXITCODE) { throw 'git archive (libnbtplusplus) failed' }
    tar -xf "$stageRoot\nbt.tar" -C "$src\libraries\libnbtplusplus"
    & $check -Source $src
    if ($LASTEXITCODE) { throw 'The source tree failed check-release.ps1.' }
    $srcZip = Join-Path $outDir "$srcName.zip"
    Write-Zip (Join-Path $stageRoot 'source') $srcZip

    Get-Item $zip, $srcZip | ForEach-Object { '{0}  {1:N0} MB' -f $_.Name, ($_.Length / 1MB) }
} finally { Pop-Location }
