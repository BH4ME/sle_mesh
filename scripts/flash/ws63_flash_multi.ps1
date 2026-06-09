param(
    [string[]]$Ports = @("COM16", "COM13", "COM17", "COM18"),
    [string]$Firmware = "",
    [string]$Python = "",
    [string]$ExpectedVersion = "v4.4.123",
    [switch]$Parallel,
    [string]$LogRoot = "",
    [int]$Baudrate = 115200,
    [double]$WaitTimeout = 15.0,
    [double]$ManualRetryTimeout = 0.0,
    [int]$ParallelStartDelayMs = 500,
    [string]$ResetCommand = "reboot",
    [switch]$HardwareReset,
    [switch]$AllowUnsafeCom14HardwareReset,
    [string]$ControlSequence = "rts=1:0.25;rts=0:0.5",
    [int]$YmodemPacketSize = 1024,
    [int]$YmodemTransferRetries = 1,
    [ValidateSet("blocking", "nonblocking-drain")]
    [string]$SerialWriteMode = "nonblocking-drain",
    [double]$SerialWriteDrainTimeout = 3.0,
    [double]$SerialWritePostGap = 0.0,
    [int]$SerialWriteChunkSize = 0,
    [double]$SerialWriteGap = 0.0,
    [int]$FlashAttempts = 1
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
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
if ($FlashAttempts -lt 1) {
    throw "FlashAttempts must be >= 1."
}

function Test-IsCom14Port {
    param([string]$PortName)
    return $PortName.Trim().ToUpperInvariant() -eq "COM14"
}

$com14Ports = @($Ports | Where-Object { Test-IsCom14Port $_ })
$com14SafeProfile = $com14Ports.Count -gt 0 -and -not $AllowUnsafeCom14HardwareReset.IsPresent

if ($com14SafeProfile) {
    if ($HardwareReset.IsPresent) {
        throw "COM14 safe profile refuses -HardwareReset because the CH340E RTS#/MOS reset path is unstable. Use the default software-reset path, or pass -AllowUnsafeCom14HardwareReset only for diagnostics."
    }
    if ($ResetCommand -ne "reboot") {
        throw "COM14 safe profile requires -ResetCommand reboot."
    }
    if ($SerialWriteMode -ne "nonblocking-drain") {
        throw "COM14 safe profile requires -SerialWriteMode nonblocking-drain."
    }
    if ($SerialWriteChunkSize -gt 0) {
        throw "COM14 safe profile refuses -SerialWriteChunkSize; keep 1024-byte YMODEM packets and use nonblocking-drain."
    }
    if ($YmodemTransferRetries -ne 1) {
        Write-Host "COM14 safe profile: forcing YmodemTransferRetries from $YmodemTransferRetries to 1; retry full flash sessions instead."
        $YmodemTransferRetries = 1
    }
    if ($FlashAttempts -lt 2) {
        Write-Host "COM14 safe profile: raising FlashAttempts from $FlashAttempts to 2 full sessions."
        $FlashAttempts = 2
    }
    if ($SerialWritePostGap -lt 0.02) {
        Write-Host "COM14 safe profile: raising SerialWritePostGap from $SerialWritePostGap to 0.02 seconds."
        $SerialWritePostGap = 0.02
    }
    if ($WaitTimeout -lt 30.0) {
        Write-Host "COM14 safe profile: raising WaitTimeout from $WaitTimeout to 30 seconds."
        $WaitTimeout = 30.0
    }
    if ($ManualRetryTimeout -ne 0.0) {
        Write-Host "COM14 safe profile: forcing ManualRetryTimeout from $ManualRetryTimeout to 0 seconds."
        $ManualRetryTimeout = 0.0
    }
} elseif ($com14Ports.Count -gt 0 -and $AllowUnsafeCom14HardwareReset.IsPresent) {
    Write-Warning "COM14 unsafe override enabled. Hardware RTS reset is diagnostic-only and may fail ROM entry or YMODEM transfer."
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
Write-Host "COM14 safe profile: $(if ($com14SafeProfile) { 'enabled' } elseif ($com14Ports.Count -gt 0) { 'unsafe override' } else { 'not applicable' })"
Write-Host "parallel: $($Parallel.IsPresent)"
Write-Host "parallel start delay ms: $ParallelStartDelayMs"
Write-Host "logs:     $runDir"
Write-Host "flow:     $(if ($HardwareReset.IsPresent) { 'hardware EN/RTS reset + post-package handshake' } else { 'software-reset-only + single reboot command + post-package handshake' })"
Write-Host "command:  $(if ($HardwareReset.IsPresent) { '<disabled>' } else { $ResetCommand })"
Write-Host "control:  $(if ($HardwareReset.IsPresent) { $ControlSequence } else { '<disabled>' })"
Write-Host "ymodem packet size: $YmodemPacketSize"
Write-Host "ymodem transfer retries: $YmodemTransferRetries"
Write-Host "serial write mode: $SerialWriteMode"
Write-Host "serial write drain timeout s: $SerialWriteDrainTimeout"
Write-Host "serial write post gap s: $SerialWritePostGap"
Write-Host "serial write chunk size: $SerialWriteChunkSize"
Write-Host "serial write gap s: $SerialWriteGap"
Write-Host "flash attempts: $FlashAttempts"
Write-Host "skip reset if ROM active: $(if ($com14SafeProfile) { 'COM14 only' } else { 'false' })"
Write-Host ""

function New-BurnArgs {
    param([string]$PortName)

    $args = @(
        $toolPath,
        "-p", $PortName,
        "-b", "$Baudrate",
        "--wait-timeout", "$WaitTimeout",
        "--manual-retry-timeout", "$ManualRetryTimeout",
        "--ymodem-packet-size", "$YmodemPacketSize",
        "--ymodem-transfer-retries", "$YmodemTransferRetries",
        "--serial-write-mode", "$SerialWriteMode",
        "--serial-write-drain-timeout", "$SerialWriteDrainTimeout",
        "--serial-write-post-gap", "$SerialWritePostGap",
        "--flash-attempts", "$FlashAttempts",
        "--expected-version", $ExpectedVersion,
        $firmwarePath
    )

    if ($HardwareReset.IsPresent) {
        $args = @(
            $toolPath,
            "-p", $PortName,
            "-b", "$Baudrate",
            "--no-reset-command",
            "--no-reset-command-fallback",
            "--no-compat-reset-command",
            "--control-sequence", $ControlSequence,
            "--wait-timeout", "$WaitTimeout",
            "--manual-retry-timeout", "$ManualRetryTimeout",
            "--ymodem-packet-size", "$YmodemPacketSize",
            "--ymodem-transfer-retries", "$YmodemTransferRetries",
            "--serial-write-mode", "$SerialWriteMode",
            "--serial-write-drain-timeout", "$SerialWriteDrainTimeout",
            "--serial-write-post-gap", "$SerialWritePostGap",
            "--flash-attempts", "$FlashAttempts",
            "--expected-version", $ExpectedVersion,
            $firmwarePath
        )
    } else {
        $args = @(
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
            "--idle-rts", "0",
            "--idle-dtr", "0",
            "--no-assert-control-after-open",
            "--wait-timeout", "$WaitTimeout",
            "--manual-retry-timeout", "$ManualRetryTimeout",
            "--ymodem-packet-size", "$YmodemPacketSize",
            "--ymodem-transfer-retries", "$YmodemTransferRetries",
            "--serial-write-mode", "$SerialWriteMode",
            "--serial-write-drain-timeout", "$SerialWriteDrainTimeout",
            "--serial-write-post-gap", "$SerialWritePostGap",
            "--flash-attempts", "$FlashAttempts",
            "--expected-version", $ExpectedVersion,
            $firmwarePath
        )
    }

    if ($com14SafeProfile -and (Test-IsCom14Port $PortName)) {
        $args = $args[0..($args.Count - 2)] + @(
            "--skip-reset-if-rom-active",
            "--rom-preflight-timeout", "1.0"
        ) + $args[($args.Count - 1)]
    }

    if ($SerialWriteChunkSize -gt 0) {
        $args = $args[0..($args.Count - 2)] + @(
            "--serial-write-chunk-size", "$SerialWriteChunkSize",
            "--serial-write-gap", "$SerialWriteGap"
        ) + $args[($args.Count - 1)]
    }

    return $args
}

function Save-RunSummary {
    $summary = @(
        "WS63 multi-port flash",
        "timestamp: $timestamp",
        "repo: $repoRoot",
        "firmware: $firmwarePath",
        "expected: $ExpectedVersion",
        "ports: $($Ports -join ', ')",
        "com14_safe_profile: $(if ($com14SafeProfile) { 'enabled' } elseif ($com14Ports.Count -gt 0) { 'unsafe override' } else { 'not applicable' })",
        "parallel: $($Parallel.IsPresent)",
        "parallel_start_delay_ms: $ParallelStartDelayMs",
        "baudrate: $Baudrate",
        "wait_timeout_s: $WaitTimeout",
        "manual_retry_timeout_s: $ManualRetryTimeout",
        "reset_command: $(if ($HardwareReset.IsPresent) { '<disabled>' } else { $ResetCommand })",
        "hardware_reset: $($HardwareReset.IsPresent)",
        "control_sequence: $(if ($HardwareReset.IsPresent) { $ControlSequence } else { '<disabled>' })",
        "ymodem_packet_size: $YmodemPacketSize",
        "ymodem_transfer_retries: $YmodemTransferRetries",
        "serial_write_mode: $SerialWriteMode",
        "serial_write_drain_timeout_s: $SerialWriteDrainTimeout",
        "serial_write_post_gap_s: $SerialWritePostGap",
        "serial_write_chunk_size: $SerialWriteChunkSize",
        "serial_write_gap_s: $SerialWriteGap",
        "flash_attempts: $FlashAttempts",
        "skip_reset_if_rom_active: $(if ($com14SafeProfile) { 'COM14 only' } else { 'false' })",
        "flow: $(if ($HardwareReset.IsPresent) { 'hardware EN/RTS reset + post-package handshake' } else { 'software-reset-only + single reboot command + post-package handshake' })",
        "",
        "reset_policy:",
        "  Default automated runs use one serial CLI reboot command after fwpkg parsing.",
        "  COM14 safe profile refuses -HardwareReset by default because COM14's RTS#/MOS reset path is unstable.",
        "  COM14 uses software-reset-only, reboot when needed, skip reset if ROM is already active, idle RTS=0 DTR=0, no post-open control assertion, 1024-byte YMODEM, fresh receiver C before each transfer, 20 ms post-write gap, nonblocking-drain, and two full flash-session attempts.",
        "  Use -HardwareReset only for non-COM14 boards wired for reliable EN/RTS reset.",
        "  SerialWriteChunkSize is a diagnostic fallback, not the preferred COM14 fix.",
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
        "  This command is guarded by ExpectedVersion.",
        "  COM14 safe profile refuses hardware RTS reset by default and uses software-reset-only.",
        "  SerialWriteMode nonblocking-drain keeps 1024-byte YMODEM packets and drains the Windows TX queue.",
        "  COM14 probes for an already-active ROM handshake and skips software reboot when ROM is ready.",
        "  COM14 retries should restart the full flash session, not only the current YMODEM file.",
        "  SerialWritePostGap adds a delay after complete large writes; it does not split packets.",
        "  Serial write chunking is only enabled when SerialWriteChunkSize > 0."
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
