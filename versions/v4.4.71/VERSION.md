# Version v4.4.71

Date: 2026-06-05

## Scope

`v4.4.71` records the resumed four-board leader probe after `v4.4.70`.
The firmware image is unchanged from `v4.4.66`.

Requested final topology remains:

```text
COM16: leader
COM13: member/relay candidate
COM17: member child
COM18: member child
leader direct cap: 1
```

## Result

The full four-board test is still blocked by `COM16`.

Latest probe:

```text
log: <repo-root>\logs\live\v4.4.70_four_board_com16_probe_20260605_101328
result: FAIL
reason: COM16 returned 0 bytes to cfg status for 3 attempts
leader log: <repo-root>\logs\live\v4.4.70_four_board_com16_probe_20260605_101328\leader_COM16.log
leader log content: [tx] cfg status[tx] cfg status[tx] cfg status
```

`COM16` is still visible at the Windows serial layer, but no firmware CLI or
boot log is received from that port.

## Healthy Board Evidence

The same probe captured live logs from the other three boards. They were still
running the previously verified `COM13`-leader topology:

```text
COM13 log: HEARTBEAT 224->241, HEARTBEAT 86->241, ROUTE_UPDATE 241->224
COM17 log: HEARTBEAT 224->241, upstream parent=86/241
COM18 log: HEARTBEAT 86->241, upstream parent=241/92, relay forward failed while forwarding broadcast
```

This supports the current diagnosis:

```text
serial ports COM13/COM17/COM18 are live and board-side firmware is running
COM16 can be opened as a serial port but board-side firmware is not answering
the requested four-board test cannot start because the designated leader is unavailable
```

## Decision

Do not repeat the same software reset, DTR/RTS probe, no-reset boot probe, or
long manual-window burn for `COM16` unless the external state changes. Those
paths have already failed repeatedly and do not add new evidence.

Meaningful next actions are hardware/external-state changes:

```text
make COM16 return [cfg-json] with "fw":"v4.4.66"
force COM16 into ROM boot handshake with BOOT/RST
change the board/adapter/wiring behind COM16
temporarily assign another responsive board as the leader for topology validation
```

## Ready Command After COM16 Recovery

```powershell
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
  --log-dir logs\live\v4.4.71_four_board_com16_leader_<timestamp>
```
