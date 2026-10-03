# End-to-end helper: launches one instance of a portable build via --launch, waits for the
# Minecraft title screen ("Sound engine started" in latest.log), then kills the game and launcher.
# Usage: .\e2e-launch.ps1 -Root C:\...\dist\PineconeMC-Offline -Instance e2e-26.3 [-Minutes 10]
param([string]$Root, [string]$Instance, [int]$Minutes = 10)
$gl = "$Root\instances\$Instance\minecraft\logs\latest.log"
if (Test-Path $gl) { Remove-Item $gl }
Get-Process pineconemc-offline -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$Root*" } | Stop-Process -Force
# Quoted: Start-Process joins the arguments with spaces, so an unquoted "My Pack" would arrive as two arguments.
$p = Start-Process "$Root\pineconemc-offline.exe" -ArgumentList '--launch', "`"$Instance`"" -PassThru
$deadline = (Get-Date).AddMinutes($Minutes); $ok = $false
while ((Get-Date) -lt $deadline) {
    Start-Sleep 5
    if ((Test-Path $gl) -and (Select-String -Quiet "Sound engine started" $gl)) { $ok = $true; break }
}
Start-Sleep 3
"reached_title=$ok"
if (Test-Path $gl) { Select-String "Setting user" $gl | Select-Object -First 1 | ForEach-Object Line }
Get-Process javaw, java -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$Root*" } | Stop-Process -Force
Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
Start-Sleep 2
Select-String 'internet reachable|Launcher is now|Compatible Java found|isn.t available offline|Failed .http|ProxyConnectionRefused' "$Root\logs\PineconeMCOffline-0.log" | ForEach-Object Line | Select-Object -First 8
