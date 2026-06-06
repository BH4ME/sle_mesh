# COM16 Leader Blocker

Date: 2026-06-05

## Status

`COM16` is still visible as a Windows serial port, but the WS63 firmware does not
answer CLI commands on that port.

Latest probe:

```text
log: <repo-root>\logs\live\v4.4.69_four_board_com16_probe_20260605_100307
command: cfg status
attempts: 3
result: 0 bytes, no [cfg-json]
```

This prevents the requested four-board topology from being verified with
`COM16` as leader.

## What Was Not Repeated

Earlier records already show the same board failed software reset, DTR/RTS
probing, multi-baud probing, no-reset boot handshake, and a 300 second manual
burn window.

Do not keep repeating the same burn/reset sequence unless the external state
changes.

## Evidence Needed To Continue

Any one of these is enough to continue the full four-board test:

```text
COM16 returns [cfg-json] with "fw":"v4.4.66"
COM16 enters ROM boot handshake during BOOT/RST
the board or USB-UART adapter on COM16 is changed
another responsive board is assigned as the leader for topology validation
```

## Command To Run After Recovery

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
  --log-dir logs\live\v4.4.70_four_board_com16_leader_<timestamp>
```
