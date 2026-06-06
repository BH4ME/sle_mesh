# Version v4.4.70

Date: 2026-06-05

## Scope

`v4.4.70` is the current workflow/test record for the four-board relay objective.
The firmware image is unchanged from `v4.4.66`.

Requested final topology:

```text
COM16: leader
COM13: member/relay candidate
COM17: member child
COM18: member child
leader direct cap: 1
```

## Current Result

The full requested four-board test is not complete because `COM16` still does not
return any firmware CLI response.

Latest COM16 probe:

```text
log: <repo-root>\logs\live\v4.4.69_four_board_com16_probe_20260605_100307
result: FAIL
reason: cfg status returned 0 bytes on COM16 for 3 attempts
tail: [tx] cfg status[tx] cfg status[tx] cfg status
```

Do not claim the four-board test passed until `COM16` returns:

```text
[cfg-json] ... "fw":"v4.4.66"
```

## Verified Fallback Test

Because `COM16` is blocked, the same relay behavior was validated with the three
responsive boards:

```text
COM13: temporary leader, fw=v4.4.66, suffix=E7F1, route=241
COM17: relay member, fw=v4.4.66, suffix=E7E0, route=224
COM18: child member, fw=v4.4.66, suffix=5556, route=86
leader direct cap: 1
```

Command:

```powershell
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_relay_cycle_test.py `
  --leader-port COM13 `
  --relay-port COM17 `
  --child-port COM18 `
  --expected-fw v4.4.66 `
  --cfg-runtime-roles `
  --team-id 1 `
  --channel 17 `
  --direct-cap 1 `
  --relay-reboot-command "cfg reboot" `
  --relay-offline-timeout-s 15 `
  --failover-timeout-s 60 `
  --log-dir logs\live\v4.4.69_three_board_fallback_20260605_100452
```

Result:

```text
PASS: relay reboot/loss recovered child route leader=241 relay=224 child=86
```

Key evidence:

```text
leader log: <repo-root>\logs\live\v4.4.69_three_board_fallback_20260605_100452\leader_COM13.log
child log:  <repo-root>\logs\live\v4.4.69_three_board_fallback_20260605_100452\child_COM18.log
relay log:  <repo-root>\logs\live\v4.4.69_three_board_fallback_20260605_100452\relay_COM17.log

leader saw relay offline: member offline id=224
child detected parent loss: parent timeout, requesting new parent
leader received child data after relay loss: POS_REPORT 86->241
relay restored from NV after reboot: restore member leader_suffix=E7F1 leader=241
leader saw original relay return: joined member=224
leader kept one relay target: relay set member=86 allow=1 reason=auto-promote
```

## Relay Recovery Policy Observed

The current policy is target-based, not fixed identity-based:

```text
direct cap 1 + two online members -> target relay count 1
original relay COM17 rebooted -> leader marked route 224 offline
child COM18 stayed/rejoined and delivered data to leader
COM17 restored from flash/NV and rejoined as a member
leader auto-promoted COM18 as relay to satisfy target=1
```

So in this observed run, the original relay did not automatically evict the newer
relay role. The recovered original relay returned as a normal member, and the
leader kept the relay count at the configured target.

## Flash Process

Use the recorded multi-port flash helper instead of hand-building burn commands:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM13,COM17,COM18 `
  -ExpectedVersion v4.4.66 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 15 `
  -ManualRetryTimeout 240
```

For four boards, use:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.66 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 15 `
  -ManualRetryTimeout 300
```

Success requires both burn evidence and serial evidence:

```text
Establishing ymodem session...
Done. Reseting device...
[cfg-json] ... "fw":"v4.4.66"
```

## Next Action When COM16 Responds

Run the full requested test immediately:

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
