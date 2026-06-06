# COM16 Leader Blocker

This file carries forward the `v4.4.67` blocker: `COM16` is visible as a CH340 serial port, but all software-side probes returned zero bytes or no boot handshake.

Authoritative prior evidence:

```text
<repo-root>\versions\v4.4.67\COM16_BLOCKER.md
```

Before using `COM16` as leader, prove:

```text
cfg status -> [cfg-json] ... "fw":"v4.4.66"
```

Then run:

```powershell
$ts = Get-Date -Format "yyyyMMdd_HHmmss"
$logDir = "logs\live\v4.4.68_four_board_com16_leader_$ts"
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
