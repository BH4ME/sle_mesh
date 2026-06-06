# v4.4.67 Flash And Four-Board Test

## Multi-Device Flash

Use this script instead of reconstructing the burn command manually:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.66 `
  -Parallel `
  -WaitTimeout 15 `
  -ManualRetryTimeout 300
```

The script now uses the verified v4.4.66 flow by default:

```text
software-reset-only
legacy-reset-order
no-reset-preamble
reset-command reboot
reset-command-fallback reset
reset-command-delay 0.3
reset-command-retries 2
reset-command-retry-gap 0.2
```

Success evidence per port:

```text
Establishing ymodem session...
Transferring ws63-liteos-app-sign.bin...
Done. Reseting device...
cfg status -> fw:"v4.4.66"
```

Known current evidence from `v4.4.66_direct_20260605_082821`:

```text
COM13: PASS
COM17: PASS
COM18: PASS
COM16: FAIL, no boot handshake before manual retry timeout
```

Do not count `COM16` as usable leader until `cfg status` proves `fw:"v4.4.66"`.

Current v4.4.67 evidence:

```text
parallel flash COM13/COM17/COM18: PASS, logs\burn\v4.4.66_20260605_085733
post-parallel cfg status COM13/COM17/COM18: PASS, fw=v4.4.66
COM16 cfg status: FAIL, 0 bytes
COM16 software-reset flash retry: FAIL, no boot handshake
COM16 multi-baud probe: FAIL, 0 bytes at all tested baud rates
COM16 DTR/RTS probe: FAIL, 0 bytes after tested line sequences
COM16 no-reset boot handshake: FAIL
COM16 recovery probe: FAIL, serial break + long DTR/RTS holds + reopen/listen attempts all returned 0 bytes
```

## Four-Board Test

Topology:

```text
COM16: leader
COM13: member approved as relay
COM17: member child
COM18: member child
leader direct cap: 1
```

Command:

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

The test verifies:

```text
all four boards report fw=v4.4.66
leader configured as runtime leader
three members configured to the leader suffix
leader direct cap set to 1
relay member approved with relay=1
two child members join as no-relay members
child1 reboot: leader sees offline, then online again
relay reboot: leader sees offline
after relay loss: one child becomes relay
after original relay returns: test prints the observed relay policy
```

Policy output meanings:

```text
original relay regained relay role; child relay was demoted
new child relay retained role; original relay returned as member
multiple relays online after recovery
no relay flag observed after recovery
```

Current code policy is target-based, not fixed-priority. After original relay returns, leader keeps the relay count at the computed target. If there are too many relays, the weakest/stalest active relay is demoted by `auto-demote`; if there are too few, the best online candidate is promoted by `auto-promote`.
