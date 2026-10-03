# Dev only: builds an offline bundle with the launcher's --export-bundle command line and checks the result.
# The launcher must be closed first (a running copy would receive the command instead).
# Usage: .\export-e2e.ps1 -Root <dist> -Out <zip> -Sets "26.3;26.3,net.neoforged=26.3.0.33-beta" [-Minutes 30]
#        .\export-e2e.ps1 -Root <dist> -Out <zip> -Instance lan-fabric [-Worlds]
# Sets are separated by ';' (powershell -File passes a list parameter as one comma-joined string, and ',' is used
# inside a set).
param([string]$Root, [string]$Out, [string]$Sets, [string]$Instance, [switch]$Worlds, [int]$Minutes = 30)
$ErrorActionPreference = 'Stop'
if (Get-Process pineconemc-offline -ErrorAction SilentlyContinue) { throw 'Close the launcher first.' }
$argList = @('--export-bundle', "`"$Out`"")
foreach ($s in ($Sets -split ';' | Where-Object { $_.Trim() })) { $argList += @('--export-set', "`"$($s.Trim())`"") }
if ($Instance) { $argList += @('--export-instance', "`"$Instance`"") }
if ($Worlds) { $argList += '--export-worlds' }
$p = Start-Process "$Root\pineconemc-offline.exe" -ArgumentList $argList -PassThru
if (-not $p.WaitForExit($Minutes * 60 * 1000)) { $p.Kill(); throw "Export timed out after $Minutes minutes." }
$log = Get-ChildItem "$Root\logs" -Filter '*.log' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
Select-String -Path $log.FullName -Pattern 'Export' | Select-Object -Last 15 | ForEach-Object { $_.Line }
"exit=$($p.ExitCode)"
if ($p.ExitCode -eq 0 -and (Test-Path $Out)) { "bundle=$Out ($([math]::Round((Get-Item $Out).Length / 1MB)) MB)" }
