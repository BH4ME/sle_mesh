# v4.4.64 Flash And Relay Test

## Build

Use the LAN Ubuntu builder, not a local VM:

```powershell
$env:UBUNTU_HOST='192.168.6.5'
$env:UBUNTU_USER='owen'
$env:UBUNTU_PASS='<set locally>'
$env:UBUNTU_SDK='/home/owen/workspace/bearpi-pico_h3863'
$env:BUILD_JOBS='4'
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py
```

Expected build evidence:

```text
profile:    v4.4.64 unified runtime role (relay failover recovery fix)
configured v4.4.64 schematic pinmap and relay failover target guard
post-build guard passed
```

## Flash With Visible Progress

Use local Windows for flashing. The script prints each burn line with a COM prefix and writes one log per port.

Sequential flashing:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.64 `
  -WaitTimeout 15 `
  -ManualRetryTimeout 300
```

Parallel flashing:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.64 `
  -WaitTimeout 15 `
  -ManualRetryTimeout 300 `
  -Parallel
```

Expected console shape:

```text
WS63 multi-port flash
expected: v4.4.64
ports:    COM16, COM13, COM17, COM18
logs:     E:\codex_documents\sle\logs\burn\v4.4.64_<timestamp>
[COM16] start, log=...
[COM16] Establishing ymodem session...
[COM16] Done. Reseting device...
[COM16] exit=0
Flash summary:
COM16: exit=0 log=...
All flash jobs passed.
```

If a port waits for manual boot:

```text
Hold BOOT, tap RESET/RST, release BOOT.
```

Do not count a board as flashed unless `cfg status` later reports `fw:"v4.4.64"`.

## Runtime Config

Leader:

```text
COM16: cfg leader now 1 17
COM16: cfg direct 1
COM16: pairing start
```

Members:

```text
COM13: cfg member now 279A 1 17
COM17: cfg member now 279A 1 17
COM18: cfg member now 279A 1 17
```

Enrollment:

```text
COM16: pairing approve 241 relay
COM16: pairing stop
```

`pairing stop` is expected to auto-approve pending `224` and `86` as no-relay members.

## Required Passing Logs

Leader must show:

```text
[cfg] direct cap=1 hw_max=8 ret=0
[team] auto approve pending member=224 relay=0 ret=0
[team] auto approve pending member=86 relay=0 ret=0
[team] route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
[team] relay failover begin lost=241
[team] relay set member=<224-or-86> allow=1 notify=1 reason=auto-promote ret=0
[team] relay failover holding relay target
```

The relay reboot test must not show a later `auto-demote` for the temporary relay while failover is still active.
