# v4.4.60 Flash And Relay Test

## Why This File Exists

This file records the exact process for the current four-board test so the next
run shows progress and does not require re-analysis.

## Build

Use the LAN Ubuntu builder, not the local VM:

```powershell
$env:UBUNTU_HOST='192.168.6.5'
$env:UBUNTU_USER='owen'
$env:UBUNTU_PASS='<set locally>'
$env:UBUNTU_SDK='/home/owen/workspace/bearpi-pico_h3863'
$env:BUILD_JOBS='4'
bash scripts/ws63_build_v4_ubuntu.sh unified
```

Expected build evidence:

```text
WS63 Ubuntu build
profile:    v4.4.60 unified runtime role (direct-cap relay route planning)
post-build guard passed: team-network app, ST7789 display, version, and serial cfg strings are linked
```

## Flash With Visible Progress

Use the multi-port script. It prints every burn line with a COM prefix and also
writes a per-port log file.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 -Ports COM16,COM13,COM17,COM18 -ExpectedVersion v4.4.60
```

Expected console shape:

```text
WS63 multi-port flash
expected: v4.4.60
ports:    COM16, COM13, COM17, COM18
[COM16] start, log=...
[COM16] Waiting for device reset...
[COM16] Transferring ws63-liteos-app-sign.bin...
[COM16] Transferring... ---------------------------------------- 100% 0:00:00
[COM16] exit=0
Flash summary:
COM16: exit=0 log=...
All flash jobs passed.
```

If a port waits for manual reset, tell the operator exactly which board:

```text
现在按 COM16 对应板子的 RESET
现在按 COM13 对应板子的 RESET
现在按 COM17 对应板子的 RESET
现在按 COM18 对应板子的 RESET
```

## Board Roles

- `COM16`: leader, suffix `279A`, route/self `154`
- `COM13`: relay member, suffix `E7F1`, route/self `241`
- `COM17`: child member, suffix `E7E0`, route/self `224`
- `COM18`: child member, suffix `5556`, route/self `86`

## Runtime Config

Leader:

```text
cfg leader now 1 17
cfg direct 1
pairing start
```

Members:

```text
cfg member now 279A 1 17
```

Enrollment:

```text
pairing approve 241 relay
pairing approve 224 norelay
pairing approve 86 norelay
pairing stop
```

## Required Passing Logs

Leader must show:

```text
[cfg] direct cap=1 hw_max=8 ret=0
[team] relay rebalance online=3 relay=1 target=1
[team] route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
```

Members must show:

```text
joined=1
```

Relay member must show:

```text
relay_allowed=1
relay_enabled=1
```

## Recovery Tests

1. Reboot one child member and capture leader `offline/lost` then `online/rejoin`.
2. Reboot relay `COM13` and capture leader relay loss.
3. Confirm downstream members either reconnect through a newly selected relay or
   return after original relay recovers.
4. Do not mark the test passed from the screen alone; use serial logs as proof.
