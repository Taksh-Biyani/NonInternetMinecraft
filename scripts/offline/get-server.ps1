# Dev only (needs internet): downloads the vanilla Minecraft server jar that lan-test.ps1 uses,
# into <repo>\.deps\mc-server\<version>\server.jar, and verifies Mojang's SHA-1.
param([string]$Version = '26.3')
$ErrorActionPreference = 'Stop'
$dest = Join-Path $PSScriptRoot "..\..\.deps\mc-server\$Version"
New-Item -ItemType Directory -Force $dest | Out-Null
$manifest = Invoke-RestMethod 'https://piston-meta.mojang.com/mc/game/version_manifest_v2.json'
$entry = $manifest.versions | Where-Object { $_.id -eq $Version }
if (-not $entry) { throw "Minecraft $Version isn't in Mojang's version manifest" }
$server = (Invoke-RestMethod $entry.url).downloads.server
$jar = Join-Path $dest 'server.jar'
Invoke-WebRequest $server.url -OutFile $jar -UseBasicParsing
$sha1 = (Get-FileHash $jar -Algorithm SHA1).Hash.ToLower()
if ($sha1 -ne $server.sha1) { Remove-Item $jar; throw "server.jar checksum mismatch ($sha1 vs $($server.sha1))" }
"server jar: $((Resolve-Path $jar).Path)"
