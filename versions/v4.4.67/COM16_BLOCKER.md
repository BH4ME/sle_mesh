# COM16 Leader Blocker

Date: 2026-06-05

## Summary

The requested four-board test cannot be completed yet because `COM16` is visible as a CH340 serial port but does not return any bytes from the WS63 application UART or bootloader handshake.

Do not use `COM16` as leader until `cfg status` returns `[cfg-json]` with `fw:"v4.4.66"`.

## Required Four-Board Topology

```text
COM16: leader
COM13: member, approved as relay
COM17: member child
COM18: member child
leader direct cap: 1
firmware expected: v4.4.66
```

## Evidence Collected

```text
COM16 Windows port exists:
  USB-SERIAL CH340 (COM16), USB VID:PID=1A86:7523, LOCATION=1-5.4.3

COM16 app cfg probe:
  <repo-root>\logs\live\v4.4.67_four_board_com16_probe_20260605_084714\run.log
  result: cfg status attempted 3 times, 0 bytes, no cfg-json

COM16 software-reset flash retry:
  <repo-root>\logs\burn\v4.4.66_20260605_084749\COM16.log
  result: reset commands sent, no boot handshake before manual retry timeout

COM16 multi-baud serial probe:
  <repo-root>\logs\serial\v4.4.67_com16_probe_20260605_085414\com16_probe.log
  result: 0 bytes at 115200, 230400, 460800, 921600, and 9600

COM16 DTR/RTS probe:
  <repo-root>\logs\serial\v4.4.67_com16_dtr_rts_probe_20260605_085547\com16_dtr_rts_probe.log
  result: 0 bytes after tested RTS/DTR pulse and hold sequences

COM16 no-reset boot handshake:
  <repo-root>\logs\burn\v4.4.67_com16_noreset_20260605_085647\COM16_noreset.log
  result: no boot handshake

COM16 recovery probe:
  <repo-root>\logs\serial\v4.4.67_com16_recovery_probe_20260605_091540\com16_recovery_probe.log
  result: serial break, long RTS/DTR holds, reopen/listen attempts all returned 0 bytes
```

## Software-Side Conclusion

Software-side checks have excluded:

```text
wrong normal UART baud rate
normal cfg command parsing issue
RTS/DTR reset sequence mismatch
already-in-bootloader immediate handshake
temporary open/close serial state
```

The remaining likely causes are physical/board-state issues outside the current software path:

```text
board not powered or held in reset
BOOT/RST not entering ROM download mode
TX/RX/GND path not connected to the CH340 side expected by COM16
board firmware/application not running and ROM download mode not being entered
hardware fault on the board or adapter path
```

## Resume Commands After COM16 Responds

First confirm `COM16` is alive:

```powershell
$ts = Get-Date -Format "yyyyMMdd_HHmmss"
$logDir = "logs\live\v4.4.67_four_board_com16_probe_$ts"
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.66 `
  --team-id 1 `
  --channel 17 `
  --direct-cap 1 `
  --reboot-command "cfg reboot" `
  --initial-drain-s 1 `
  --cmd-timeout-s 10 `
  --state-timeout-s 20 `
  --route-timeout-s 20 `
  --offline-timeout-s 12 `
  --boot-timeout-s 30 `
  --failover-timeout-s 30 `
  --poll-interval-s 1 `
  --log-dir $logDir
```

Then run the full four-board test:

```powershell
$ts = Get-Date -Format "yyyyMMdd_HHmmss"
$logDir = "logs\live\v4.4.67_four_board_com16_leader_$ts"
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.66 `
  --team-id 1 `
  --channel 17 `
  --direct-cap 1 `
  --reboot-command "cfg reboot" `
  --log-dir $logDir
```

## Expected Full-Test Evidence

```text
leader COM16 fw=v4.4.66
relay/member COM13 fw=v4.4.66
child1/member COM17 fw=v4.4.66
child2/member COM18 fw=v4.4.66
leader direct cap=1
enrollment PASS
child reboot PASS: leader saw offline and rejoin
relay failover observed: child relay elected [...]
relay recovery policy: <observed policy string>
PASS total_ms=...
```
