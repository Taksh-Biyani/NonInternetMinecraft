# Checks a release before and after zipping. Fails (exit 1) on anything that must not ship:
# development and build files, user data (accounts, instances, logs), leftover junk, missing DLLs, secrets, or text
# that gives away where the launcher was built.
# Usage: .\check-release.ps1 -Stage <folder> | -Zip <launcher zip> | -Source <source folder>
#
# Machine-specific names (user names, local paths, editor and tool folders) go in .git\info\release-deny.txt, which is
# never committed. One entry per line, '#' starts a comment:
#   name:<regex>   a file or folder name that must not ship (matched against each path segment)
#   text:<text>    text that must not appear in any file (binary files: searched as bytes and UTF-16; use long,
#                  distinctive text there, short words match by chance)
#   plain:<text>   text that must not appear in text files
param([string]$Stage, [string]$Zip, [string]$Source)
$ErrorActionPreference = 'Stop'
$problems = [Collections.Generic.List[string]]::new()

# Never shipped, at any depth (matched against each path segment, case-insensitive).
$devNames = @('\.git', '\.vs', '\.vscode', '\.idea', 'CMakeCache\.txt', 'CMakeFiles', '.*\.pdb', '.*\.ilk')
$devTextAll = @()
$devTextPlain = @('GH_TOKEN')
$denyFile = Join-Path $PSScriptRoot '..\..\.git\info\release-deny.txt'
if (Test-Path $denyFile) {
    foreach ($line in Get-Content $denyFile) {
        if ($line -match '^\s*(#|$)') { continue }
        if ($line -match '^(name|text|plain):(.+)$') {
            switch ($Matches[1]) {
                'name' { $devNames += $Matches[2] }
                'text' { $devTextAll += $Matches[2] }
                'plain' { $devTextPlain += $Matches[2] }
            }
        } else { throw "release-deny.txt: can't read '$line'" }
    }
} else {
    Write-Warning "No $denyFile: only the built-in checks run."
}
$devName = '^(' + ($devNames -join '|') + ')$'
$devTextPlain = $devTextAll + $devTextPlain
$textExt = '.html', '.txt', '.md', '.json', '.ini', '.conf', '.cfg', '.properties', '.xml', '.ps1', '.cpp', '.h', '.java', '.cmake', '.yml', '.yaml', '.nix', '.in'
$latin1 = [Text.Encoding]::GetEncoding(28591)

function Test-Names([string[]]$Relative) {
    foreach ($rel in $Relative) {
        foreach ($seg in ($rel -split '[\\/]')) {
            if ($seg -match $devName) { $problems.Add("development file: $rel"); break }
        }
    }
}
function Test-Content([string]$Root, [string[]]$PlainText = $devTextPlain) {
    foreach ($f in Get-ChildItem $Root -Recurse -File -Force) {
        # Lower-case once, then search exactly: much faster than case-insensitive searches over the Java runtimes.
        $text = $latin1.GetString([IO.File]::ReadAllBytes($f.FullName)).ToLowerInvariant()
        $plain = $textExt -contains $f.Extension.ToLower()
        foreach ($p in $(if ($plain) { $PlainText } else { $devTextAll })) {
            $p = $p.ToLowerInvariant()
            $wide = ($p.ToCharArray() | ForEach-Object { "$_`0" }) -join ''
            if ($text.IndexOf($p, [StringComparison]::Ordinal) -ge 0 -or
                (-not $plain -and $text.IndexOf($wide, [StringComparison]::Ordinal) -ge 0)) {
                $problems.Add("'$p' found in $($f.FullName.Substring($Root.Length + 1))")
            }
        }
        if ($plain -and $latin1.GetString([IO.File]::ReadAllBytes($f.FullName)) -cmatch 'ghp_[A-Za-z0-9]{36}') { $problems.Add("GitHub token in $($f.Name)") }
    }
}

# Top level of the launcher zip: program files only. User data (accounts.json, *.cfg, instances, logs, meta,
# libraries, assets, cache...) is created next to these on first start and never ships, so a portable copy can be
# updated by copying a new zip over it.
$allowedFiles = '^(pineconemc-offline\.exe|pineconemc-offline_filelink\.exe|portable\.txt|qt\.conf|qtlogging\.ini|Guide\.html|README-FIRST\.txt|[^\\/]+\.dll)$'
$allowedDirs = '^(iconengines|imageformats|platforms|styles|tls|jars|java|translations|licenses)$'
$javaDirs = 'temurin-25-jre', 'temurin-21-jre', 'temurin-17-jre', 'temurin-8-jre'

function Test-Layout([string[]]$Relative) {
    foreach ($rel in $Relative) {
        $parts = $rel -split '[\\/]'
        if ($parts.Count -eq 1 -and $parts[0] -notmatch $allowedFiles) { $problems.Add("unexpected file: $rel") }
        if ($parts.Count -gt 1 -and $parts[0] -notmatch $allowedDirs) { $problems.Add("unexpected folder: $($parts[0])") }
        if ($parts.Count -gt 2 -and $parts[0] -eq 'java' -and $javaDirs -notcontains $parts[1]) { $problems.Add("unexpected Java: $($parts[1])") }
        if ($parts[0] -ne 'java' -and $parts[-1] -match '[()]') { $problems.Add("junk file: $rel") }
    }
    foreach ($j in $javaDirs) { if (-not ($Relative -match "^java[\\/]$j[\\/]bin[\\/]javaw\.exe$")) { $problems.Add("missing Java: $j") } }
    if (@($Relative -match '^translations[\\/]mmc_.+\.qm$').Count -lt 50) { $problems.Add('translations missing') }
    if (-not ($Relative -match '^translations[\\/]index_v2\.json$')) { $problems.Add('translations index missing') }
    foreach ($need in 'pineconemc-offline.exe', 'portable.txt', 'Guide.html', 'README-FIRST.txt', 'vcruntime140.dll', 'msvcp140.dll',
                      'jars\authlib-injector.jar', 'jars\pinecone-offline-auth.jar', 'licenses\PineconeMC-Offline-LICENSE.txt') {
        if (-not ($Relative -contains $need -or $Relative -contains $need.Replace('\', '/'))) { $problems.Add("missing: $need") }
    }
}

function Test-Dlls([string]$Root) {
    # Every DLL that the exe and the Qt DLLs/plugins load must be in the folder, or be part of Windows. The VC++
    # runtime is not part of Windows on a fresh PC, so it must be in the folder.
    $notWindows = '^(vcruntime|msvcp|concrt|vccorlib)'
    $have = @{}
    Get-ChildItem $Root -Filter *.dll | ForEach-Object { $have[$_.Name.ToLower()] = $true }
    $binaries = @(Get-ChildItem $Root -File | Where-Object { $_.Extension -in '.exe', '.dll' }) +
                @(Get-ChildItem $Root -Recurse -Filter *.dll | Where-Object { $_.FullName -notmatch '\\java\\' -and $_.DirectoryName -ne $Root })
    foreach ($b in $binaries) {
        $deps = dumpbin /nologo /dependents $b.FullName | Where-Object { $_ -match '^\s+(\S+\.dll)\s*$' } | ForEach-Object { $Matches[1].ToLower() }
        foreach ($d in $deps) {
            if ($have[$d] -or $d -match '^(api-ms-win|ext-ms)-') { continue }
            if ($d -notmatch $notWindows -and (Test-Path (Join-Path $env:WINDIR "System32\$d"))) { continue }
            $problems.Add("$($b.Name) needs $d, which is not in the zip")
        }
    }
}

if ($Stage) {
    . (Join-Path $PSScriptRoot 'env.ps1')  # dumpbin
    $Stage = (Resolve-Path $Stage).Path
    $rel = @(Get-ChildItem $Stage -Recurse -File -Force | ForEach-Object { $_.FullName.Substring($Stage.Length + 1) })
    Test-Names $rel
    Test-Layout $rel
    Get-ChildItem $Stage -Recurse -File -Force | Where-Object { $_.Length -eq 0 -and $_.FullName -notmatch '\\java\\' } |
        ForEach-Object { $problems.Add("empty file: $($_.Name)") }
    Test-Content $Stage
    Test-Dlls $Stage
}
if ($Zip) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead((Resolve-Path $Zip).Path)
    try { $names = @($archive.Entries | Where-Object { $_.Name } | ForEach-Object { $_.FullName }) } finally { $archive.Dispose() }
    foreach ($n in $names) { if ($n -match '\\|^/|(^|/)\.\.(/|$)') { $problems.Add("bad zip entry name: $n") } }
    Test-Names $names
    Test-Layout $names
}
if ($Source) {
    $Source = (Resolve-Path $Source).Path
    Test-Names @(Get-ChildItem $Source -Recurse -Force | ForEach-Object { $_.FullName.Substring($Source.Length + 1) })
    # Upstream's GitHub workflows name the GH_TOKEN secret; only a real token (ghp_...) is a problem in sources.
    Test-Content $Source @($devTextPlain | Where-Object { $_ -ne 'GH_TOKEN' })
}
if ($problems.Count) { $problems | Sort-Object -Unique | ForEach-Object { Write-Host "PROBLEM: $_" }; exit 1 }
Write-Host 'check-release: OK'
