# Downloads what the release zip ships besides the build: the Eclipse Temurin JREs (Windows x64) and the launcher
# translations. Each file is checked (SHA-256 for Java, SHA-1 for translations) and cached in .deps\release;
# -Refresh downloads everything again. Usage: .\fetch-runtimes.ps1 [-Refresh]
param([switch]$Refresh)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'  # the progress bar makes Invoke-WebRequest very slow in PowerShell 5.1
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$out = Join-Path $RepoRoot '.deps\release'
$downloads = Join-Path $out 'downloads'
New-Item -ItemType Directory -Force $downloads, "$out\java", "$out\translations" | Out-Null

function Get-Checked([string]$Url, [string]$File, [string]$Algorithm, [string]$Hash) {
    if (-not $Refresh -and (Test-Path $File) -and (Get-FileHash $File -Algorithm $Algorithm).Hash -eq $Hash.ToUpper()) { return }
    Invoke-WebRequest $Url -OutFile $File -UseBasicParsing
    if ((Get-FileHash $File -Algorithm $Algorithm).Hash -ne $Hash.ToUpper()) { Remove-Item $File; throw "Checksum mismatch: $Url" }
}

# Java: the latest GA Temurin JRE of each major version Minecraft needs (26.x: 25, 1.20.5+: 21, 1.17+: 17, older: 8).
$record = @()
foreach ($major in 25, 21, 17, 8) {
    $api = "https://api.adoptium.net/v3/assets/latest/$major/hotspot?architecture=x64&image_type=jre&os=windows&vendor=eclipse"
    $asset = @(Invoke-RestMethod $api -UseBasicParsing) | Where-Object { $_.binary.package.name -like '*.zip' } | Select-Object -First 1
    if (-not $asset) { throw "No Temurin $major JRE zip for Windows x64" }
    $pkg = $asset.binary.package
    $zip = Join-Path $downloads $pkg.name
    Get-Checked $pkg.link $zip 'SHA256' $pkg.checksum
    $target = Join-Path $out "java\temurin-$major-jre"
    # Records which download the folder was unpacked from (removed again when the release is assembled).
    $stamp = Join-Path $target '.from'
    if ($Refresh -or -not (Test-Path $stamp) -or (Get-Content $stamp -Raw).Trim() -ne $pkg.name) {
        if (Test-Path $target) { Remove-Item -Recurse -Force $target }
        $tmp = Join-Path $out "unpack-$major"
        if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp }
        New-Item -ItemType Directory $tmp | Out-Null
        tar -xf $zip -C $tmp
        if ($LASTEXITCODE) { throw "Unpacking $zip failed" }
        Move-Item (Get-ChildItem $tmp -Directory | Select-Object -First 1).FullName $target
        Remove-Item -Recurse -Force $tmp
        [IO.File]::WriteAllText($stamp, $pkg.name)
    }
    if (-not (Test-Path "$target\bin\javaw.exe")) { throw "$target has no bin\javaw.exe" }
    $record += [pscustomobject]@{ major = $major; release = $asset.release_name; file = $pkg.name; sha256 = $pkg.checksum }
}

# Translations: the same files the launcher downloads itself when online (Launcher_TRANSLATION_FILES_URL in CMakeLists.txt).
$base = 'https://i18n.prismlauncher.org/'
$index = Join-Path $out 'translations\index_v2.json'
Invoke-WebRequest "${base}index_v2.json" -OutFile $index -UseBasicParsing
$count = 0
foreach ($lang in (Get-Content $index -Raw | ConvertFrom-Json).languages.PSObject.Properties) {
    if (-not $lang.Value.file) { continue }
    Get-Checked "$base$($lang.Value.file)" (Join-Path $out "translations\mmc_$($lang.Name).qm") 'SHA1' $lang.Value.sha1
    $count++
}
[IO.File]::WriteAllText((Join-Path $out 'runtimes.json'), ($record | ConvertTo-Json))
$record | Format-Table major, release -AutoSize | Out-String | Write-Host
Write-Host "Translations: $count languages"
