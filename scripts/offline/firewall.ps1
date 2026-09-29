# Dev only: blocks internet access for the portable launcher and every java.exe/javaw.exe under -Root using
# Windows Defender Firewall outbound rules (loopback traffic such as 127.0.0.1 LAN tests is not affected).
# Asks for administrator rights (UAC) itself. Usage: .\firewall.ps1 -Root <dist> [-Remove]
param([string]$Root, [switch]$Remove)
$ErrorActionPreference = 'Stop'
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    $argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", '-Root', "`"$Root`"")
    if ($Remove) { $argList += '-Remove' }
    $p = Start-Process powershell -Verb RunAs -ArgumentList $argList -Wait -PassThru
    exit $p.ExitCode
}
$group = 'PineconeMC Offline test block'
Get-NetFirewallRule -Group $group -ErrorAction SilentlyContinue | Remove-NetFirewallRule
if ($Remove) { "Removed the '$group' rules."; exit 0 }
$programs = @(Join-Path $Root 'pineconemc-offline.exe') +
    (Get-ChildItem -Path $Root -Recurse -Include java.exe, javaw.exe | ForEach-Object FullName)
foreach ($program in $programs) {
    New-NetFirewallRule -DisplayName "$group - $(Split-Path $program -Leaf)" -Group $group -Direction Outbound `
        -Action Block -Program $program | Out-Null
}
"Blocked $($programs.Count) programs:"
$programs
