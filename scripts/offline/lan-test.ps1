# Dev LAN sign-in test. Starts a vanilla server that requires authentication (like an "Open to LAN"
# world) on 127.0.0.1:25599, launches a client instance that joins it, and reports whether the player got in.
# Usage: .\lan-test.ps1 -Root <dist> -Instance <id> -Account <offline account name> [-Version 26.3] [-ServerAgents] [-Minutes 6]
#   -ServerAgents runs the server JVM with the offline sign-in agents, the same way the launcher starts a LAN host.
# Prints result=JOINED, result=REJECTED or result=TIMEOUT, then the relevant log lines.
param([string]$Root, [string]$Instance, [string]$Account, [string]$Version = '26.3', [switch]$ServerAgents, [int]$Minutes = 6)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$serverDir = Join-Path $repo ".deps\mc-server\$Version"
if (-not (Test-Path "$serverDir\server.jar")) { throw "Run get-server.ps1 -Version $Version first" }
$java = Join-Path $Root 'java\java-runtime-epsilon\bin\java.exe'
$port = 25599

function Stop-TestProcesses {
    Get-Process javaw, java -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$Root*" } | Stop-Process -Force
    Get-Process pineconemc-offline -ErrorAction SilentlyContinue | Stop-Process -Force
}
Stop-TestProcesses
Start-Sleep 2

Set-Content -Encoding ascii "$serverDir\eula.txt" 'eula=true'
Set-Content -Encoding ascii "$serverDir\server.properties" @(
    'online-mode=true', "server-port=$port", 'server-ip=127.0.0.1', 'enforce-secure-profile=false',
    'level-name=lan-test-world', 'spawn-protection=0', 'max-players=4', 'motd=PineconeMC Offline LAN test')
foreach ($f in "$serverDir\logs\latest.log", "$serverDir\logs\pinecone-offline-auth.log",
    "$Root\instances\$Instance\minecraft\logs\pinecone-offline-auth.log") { if (Test-Path $f) { Remove-Item $f } }

$jvm = @('-Xmx2G')
if ($ServerAgents) {
    $authPort = 25601
    $jvm += "-javaagent:`"$Root\jars\pinecone-offline-auth.jar`"=$authPort",
        "-javaagent:`"$Root\jars\authlib-injector.jar`"=http://127.0.0.1:$authPort", '-Dauthlibinjector.noLogFile'
}
$server = Start-Process $java -ArgumentList ($jvm + '-jar', 'server.jar', 'nogui') -WorkingDirectory $serverDir -PassThru -WindowStyle Hidden
$log = "$serverDir\logs\latest.log"
$deadline = (Get-Date).AddMinutes(3)
while (-not ((Test-Path $log) -and (Select-String -Quiet 'Done \(' $log))) {
    if ((Get-Date) -gt $deadline -or $server.HasExited) { Stop-TestProcesses; throw 'The test server did not start' }
    Start-Sleep 2
}

Start-Process "$Root\pineconemc-offline.exe" -ArgumentList '--launch', $Instance, '--profile', $Account, '--server', "127.0.0.1:$port" | Out-Null
$result = 'TIMEOUT'
$deadline = (Get-Date).AddMinutes($Minutes)
while ((Get-Date) -lt $deadline) {
    Start-Sleep 3
    if (Select-String -Quiet "$Account joined the game" $log) { $result = 'JOINED'; break }
    if (Select-String -Quiet 'lost connection|Disconnecting|Failed to verify' $log) { $result = 'REJECTED'; break }
}
Start-Sleep 3
"result=$result"
'--- server log ---'
Select-String 'joined the game|lost connection|Disconnecting|Failed to verify|authentication|Authentication|UUID of player' $log | ForEach-Object Line | Select-Object -First 10
$clientLog = "$Root\instances\$Instance\minecraft\logs\latest.log"
'--- client log ---'
if (Test-Path $clientLog) {
    Select-String 'Setting user|Connecting to|disconnect|Disconnect|Invalid session|authentication|Authentication|authlib-injector|Failed to log' $clientLog |
        ForEach-Object Line | Select-Object -First 12
}
foreach ($stub in "$serverDir\logs\pinecone-offline-auth.log", "$Root\instances\$Instance\minecraft\logs\pinecone-offline-auth.log") {
    if (Test-Path $stub) { "--- $stub ---"; Get-Content $stub | Select-Object -First 20 }
}
Stop-TestProcesses
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
