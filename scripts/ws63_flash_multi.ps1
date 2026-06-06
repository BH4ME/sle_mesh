param(
    [string[]]$Ports = @("COM16", "COM13", "COM17", "COM18"),
    [string]$Firmware = "",
    [string]$Python = "",
    [string]$ExpectedVersion = "v4.4.95",
    [switch]$Parallel,
    [string]$LogRoot = "",
    [int]$Baudrate = 115200,
    [double]$WaitTimeout = 15.0,
    [double]$ManualRetryTimeout = 0.0,
    [int]$ParallelStartDelayMs = 500,
    [string]$ResetCommand = "reboot"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
if ($Firmware -eq "") {
    $Firmware = Join-Path $repoRoot "output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg"
}
if ($Python -eq "") {
    $Python = Join-Path $repoRoot ".tooling\py311\python.exe"
}
if ($LogRoot -eq "") {
    $LogRoot = Join-Path $repoRoot "logs\burn"
}

$burnTool = Join-Path $repoRoot "automation\ws63\tools\ws63_auto_burn.py"
$firmwarePath = Resolve-Path $Firmware
$pythonPath = Resolve-Path $Python
$toolPath = Resolve-Path $burnTool

if ($Ports.Count -eq 1 -and $Ports[0].Contains(",")) {
    $Ports = @($Ports[0].Split(",") | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
}

if ($Ports.Count -eq 0) {
    throw "No serial ports were provided."
}

if ($ExpectedVersion -ne "") {
    $fwText = [System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes($firmwarePath))
    if (-not $fwText.Contains($ExpectedVersion)) {
        throw "Firmware package does not contain expected version ${ExpectedVersion}: $firmwarePath"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$runDir = Join-Path $LogRoot "$ExpectedVersion`_$timestamp"
New-Item -ItemType Directory -Force -Path $runDir | Out-Null

Write-Host "WS63 multi-port flash"
Write-Host "repo:     $repoRoot"
Write-Host "firmware: $firmwarePath"
Write-Host "expected: $ExpectedVersion"
Write-Host "ports:    $($Ports -join ', ')"
Write-Host "parallel: $($Parallel.IsPresent)"
Write-Host "parallel start delay ms: $ParallelStartDelayMs"
Write-Host "logs:     $runDir"
Write-Host "flow:     software-reset-only + single reboot command + post-package handshake"
Write-Host "command:  $ResetCommand"
Write-Host "reset:    automatic software reset; no manual reset expected"
Write-Host ""

function New-BurnArgs {
    param([string]$PortName)

    return @(
        $toolPath,
        "-p", $PortName,
        "-b", "$Baudrate",
        "--software-reset-only",
        "--reset-command", $ResetCommand,
        "--no-reset-command-fallback",
        "--no-compat-reset-command",
        "--reset-command-delay", "0.05",
        "--reset-command-retries", "1",
        "--reset-command-retry-gap", "0",
        "--wait-timeout", "$WaitTimeout",
        "--manual-retry-timeout", "$ManualRetryTimeout",
        "--expected-version", $ExpectedVersion,
        $firmwarePath
    )
}

function Save-RunSummary {
    $summary = @(
        "WS63 multi-port flash",
        "timestamp: $timestamp",
        "repo: $repoRoot",
        "firmware: $firmwarePath",
        "expected: $ExpectedVersion",
        "ports: $($Ports -join ', ')",
        "parallel: $($Parallel.IsPresent)",
        "parallel_start_delay_ms: $ParallelStartDelayMs",
        "baudrate: $Baudrate",
        "wait_timeout_s: $WaitTimeout",
        "manual_retry_timeout_s: $ManualRetryTimeout",
        "reset_command: $ResetCommand",
        "flow: software-reset-only + single reboot command + post-package handshake",
        "",
        "reset_policy:",
        "  This project must not assume RTS/DTR auto-reset hardware.",
        "  Use one serial CLI reboot command after fwpkg parsing and keep ManualRetryTimeout=0 for automated runs.",
        "  Success evidence is:",
        "  - Establishing ymodem session",
        "  - Done. Reseting device...",
        "",
        "logs:",
        "  one file per port: <PORT>.log",
        "  one command per port: <PORT>.command.txt"
    )
    Set-Content -Path (Join-Path $runDir "run_summary.txt") -Value $summary -Encoding ASCII
}

function Save-PortCommand {
    param(
        [string]$PortName,
        [object[]]$BurnArgs
    )

    $commandText = @(
        "`"$pythonPath`" " + (($BurnArgs | ForEach-Object { "`"$_`"" }) -join " "),
        "",
        "reset_policy:",
        "  This command uses the current software-reset flow guarded by ExpectedVersion.",
        "  COM16 requires a single configured reboot command; reset/AT+RST can miss the loader window.",
        "  Manual reset should not be needed; keep ManualRetryTimeout=0 for automated runs."
    )
    Set-Content -Path (Join-Path $runDir "$PortName.command.txt") -Value $commandText -Encoding ASCII
}

Save-RunSummary

function Invoke-OneBurn {
    param([string]$PortName)

    $log = Join-Path $runDir "$PortName.log"
    $args = New-BurnArgs -PortName $PortName
    Save-PortCommand -PortName $PortName -BurnArgs $args
    Write-Host "[$PortName] start, log=$log"
    & $pythonPath @args 2>&1 | ForEach-Object {
        $line = "$_"
        Add-Content -Path $log -Value $line
        Write-Host "[$PortName] $line"
    }
    $exitCode = $LASTEXITCODE
    Write-Host "[$PortName] exit=$exitCode"
    return [pscustomobject]@{ Port = $PortName; ExitCode = $exitCode; Log = $log }
}

$results = @()
if (-not $Parallel.IsPresent) {
    foreach ($port in $Ports) {
        $results += Invoke-OneBurn -PortName $port
    }
} else {
    Write-Host "Parallel mode: software reset only; no manual reset expected."
    $jobs = @()
    foreach ($port in $Ports) {
        $log = Join-Path $runDir "$port.log"
        $args = New-BurnArgs -PortName $port
        Save-PortCommand -PortName $port -BurnArgs $args
        $jobs += Start-Job -Name "flash_$port" -ArgumentList $pythonPath, $args, $log, $port -ScriptBlock {
            param($PythonPath, $BurnArgs, $LogPath, $PortName)
            $ErrorActionPreference = "Continue"
            & $PythonPath @BurnArgs 2>&1 | ForEach-Object {
                $line = "$_"
                Add-Content -Path $LogPath -Value $line
                Write-Output $line
            }
            [pscustomobject]@{
                Port = $PortName
                ExitCode = $LASTEXITCODE
                Log = $LogPath
                __BurnResult = $true
            }
        }
        Write-Host "[$port] job started, log=$log"
        if ($ParallelStartDelayMs -gt 0) {
            Start-Sleep -Milliseconds $ParallelStartDelayMs
        }
    }

    $resultByPort = @{}
    do {
        foreach ($job in $jobs) {
            Receive-Job -Job $job | ForEach-Object {
                if ($_.PSObject.Properties.Name -contains "__BurnResult") {
                    $resultByPort[$_.Port] = $_
                } else {
                    $portName = $job.Name -replace "^flash_", ""
                    Write-Host "[$portName] $_"
                }
            }
        }
        Start-Sleep -Milliseconds 500
    } while (@($jobs | Where-Object { $_.State -eq "Running" }).Count -gt 0)

    foreach ($job in $jobs) {
        Receive-Job -Job $job | ForEach-Object {
            if ($_.PSObject.Properties.Name -contains "__BurnResult") {
                $resultByPort[$_.Port] = $_
            } else {
                $portName = $job.Name -replace "^flash_", ""
                Write-Host "[$portName] $_"
            }
        }
        Remove-Job -Job $job
    }
    foreach ($port in $Ports) {
        if ($resultByPort.ContainsKey($port)) {
            $results += $resultByPort[$port]
        } else {
            $results += [pscustomobject]@{ Port = $port; ExitCode = 99; Log = Join-Path $runDir "$port.log" }
        }
    }
}

Write-Host ""
Write-Host "Flash summary:"
Add-Content -Path (Join-Path $runDir "run_summary.txt") -Value ""
Add-Content -Path (Join-Path $runDir "run_summary.txt") -Value "results:"
$failed = 0
foreach ($result in $results) {
    $summaryLine = "{0}: exit={1} log={2}" -f $result.Port, $result.ExitCode, $result.Log
    Write-Host $summaryLine
    Add-Content -Path (Join-Path $runDir "run_summary.txt") -Value "  $summaryLine"
    if ([int]$result.ExitCode -ne 0) {
        $failed++
    }
}

if ($failed -ne 0) {
    Add-Content -Path (Join-Path $runDir "run_summary.txt") -Value "result: FAIL failed=$failed"
    throw "$failed flash job(s) failed. Check logs under $runDir"
}

Add-Content -Path (Join-Path $runDir "run_summary.txt") -Value "result: PASS"
Write-Host "All flash jobs passed."
