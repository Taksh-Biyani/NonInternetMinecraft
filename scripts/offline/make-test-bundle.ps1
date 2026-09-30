# Dev only: builds an offline bundle from an online portable folder, until Plan 4's export wizard exists.
# It includes the WHOLE meta/, libraries/, assets/ and java/ folders of -Source (everything that copy has downloaded),
# plus, for -Kind instance, the instance folder -Instance.
# Usage: .\make-test-bundle.ps1 -Source <dist> -Out <zip> -Name "<name>" -Components "net.minecraft=26.3|Minecraft,net.fabricmc.fabric-loader=0.19.5|Fabric Loader"
#        [-Kind instance -Instance <id>] [-JavaFolder java-runtime-epsilon -JavaMajor 25]
param(
    [string]$Source, [string]$Out, [string]$Name, [string]$Components,
    [ValidateSet('versions', 'instance')][string]$Kind = 'versions', [string]$Instance,
    [string]$JavaFolder = 'java-runtime-epsilon', [int]$JavaMajor = 25
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem

$items = New-Object System.Collections.Generic.List[object]
foreach ($root in 'meta', 'libraries', 'assets', 'java') {
    $dir = Join-Path $Source $root
    if (-not (Test-Path $dir)) { continue }
    Get-ChildItem $dir -Recurse -File | ForEach-Object {
        $items.Add(@{ Full = $_.FullName; Rel = ($root + '/' + $_.FullName.Substring($dir.Length + 1).Replace('\', '/')) })
    }
}
if ($Kind -eq 'instance') {
    $instDir = Join-Path $Source "instances\$Instance"
    Get-ChildItem $instDir -Recurse -File | Where-Object { $_.FullName -notmatch '\\minecraft\\logs\\' } | ForEach-Object {
        $items.Add(@{ Full = $_.FullName; Rel = ('instance/' + $_.FullName.Substring($instDir.Length + 1).Replace('\', '/')) })
    }
}

Write-Host "Hashing $($items.Count) files..."
$files = foreach ($item in $items) {
    [ordered]@{ path = $item.Rel; sha1 = (Get-FileHash $item.Full -Algorithm SHA1).Hash.ToLower(); size = (Get-Item $item.Full).Length }
}
$componentList = foreach ($c in ($Components -split ',')) {
    $uidVersion, $display = $c -split '\|', 2
    $uid, $version = $uidVersion -split '=', 2
    [ordered]@{ uid = $uid; version = $version; name = $(if ($display) { $display } else { $uid }) }
}
$contents = [ordered]@{
    components = @($componentList)
    java = @([ordered]@{ name = "Java $JavaMajor"; major = $JavaMajor; folder = $JavaFolder })
    instance = $(if ($Kind -eq 'instance') { [ordered]@{ name = $Name; folder = 'instance' } } else { $null })
}
$manifest = [ordered]@{
    formatVersion = 1; kind = $Kind; name = $Name
    createdAt = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ'); createdBy = 'make-test-bundle.ps1'
    contents = $contents; files = @($files)
}
$json = $manifest | ConvertTo-Json -Depth 6 -Compress

if (Test-Path $Out) { Remove-Item $Out }
New-Item -ItemType Directory -Force (Split-Path $Out) | Out-Null
$zip = [System.IO.Compression.ZipFile]::Open($Out, 'Create')
try {
    $entry = $zip.CreateEntry('pinecone-offline-bundle.json')
    $writer = New-Object System.IO.StreamWriter($entry.Open(), (New-Object System.Text.UTF8Encoding($false)))
    $writer.Write($json)
    $writer.Dispose()
    foreach ($item in $items) {
        [void][System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $item.Full, $item.Rel, 'Optimal')
    }
} finally { $zip.Dispose() }
"bundle: $Out ($([math]::Round((Get-Item $Out).Length / 1MB)) MB, $($items.Count) files)"
