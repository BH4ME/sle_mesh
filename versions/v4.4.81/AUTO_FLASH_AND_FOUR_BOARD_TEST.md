# v4.4.81 Auto Flash And Four-Board Test Flow

`v4.4.81` adds automatic route-regression detection to the four-board test.

## Firmware Package

Use this package unless a newer version directory explicitly replaces it:

```text
E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

Before flashing, the wrapper verifies that the package contains the expected
firmware string. Current known-good package evidence:

```text
expected version: v4.4.81
package size: 1601000 bytes
contains v4.4.81: true
```

## Parallel Flash

Default four-board ports:

```text
COM16: leader target
COM13: relay candidate member
COM17: child member
COM18: child member
```

Run this from `E:\codex_documents\sle`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.81 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

The wrapper records one log and one command file per port under `logs\burn`.
It uses the current software-reset flow:

```text
software-reset-only
reset command: reboot
no RTS/DTR assumption
no manual reset fallback during automated runs
```

Flash success evidence must include:

```text
Establishing ymodem session...
Done. Reseting device...
All flash jobs passed.
```

If one port fails, do not guess or immediately retry blindly. Inspect that
port's `<PORT>.log` and `<PORT>.command.txt` in the generated burn log folder.

## Four-Board Configuration And Test

The live test opens all four serial ports and performs the requested topology:

```text
COM16 -> leader
COM13 -> member approved as relay
COM17 -> member
COM18 -> member
leader direct cap -> 1
```

Because the leader direct capacity is `1`, only one member should remain
directly attached to the leader. The other two members should route through a
relay. The expected converged route metric is:

```text
route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
```

Run this from `E:\codex_documents\sle`:

```powershell
$stamp=Get-Date -Format 'yyyyMMdd_HHmmss'
$logDir="logs\live\v4.4.81_four_board_com16_leader_$stamp"
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.81 `
  --direct-cap 1 `
  --log-dir $logDir
```

## Test Stages

The script verifies these stages in order:

1. All four boards report `fw=v4.4.81`.
2. COM16 is configured as leader and `cfg direct 1` succeeds.
3. COM13/COM17/COM18 are configured as members using COM16's leader suffix.
4. Pairing starts on leader.
5. COM13 is approved as relay.
6. COM17 and COM18 are accepted as non-relay members.
7. Topology converges to `active=3 direct=1 relayed=2`.
8. COM17 is rebooted; leader must see offline and then rejoin.
9. COM13 relay is rebooted; leader must see relay offline.
10. One of COM17/COM18 must self-elect as relay while COM13 is down.
11. COM13 restores from NV and rejoins.
12. Final policy is logged as one of:

```text
original relay regained relay role; child relay was demoted
new child relay retained role; original relay returned as member; child_relays=[...]
multiple relays online after recovery; original relay and child_relays=[...]
```

For the current target behavior, either of the first two recovery policies is
acceptable if the final route metrics are converged and there is no route
regression. The log line records what actually happened so the policy is not
rediscovered next time.

## Additional v4.4.81 Regression Gate

The test fails if it sees either of these in the collected logs:

```text
route hint member=<id> parent=<relay> ret=-4
[sle-rx] <leader-bound packet> followed by dst=<leader> reason=NO_ROUTE
```

This gate specifically protects the `v4.4.74` feedback where the leader had a
valid physical direct connection but reported `NO_ROUTE` while sending a route
hint.

## Output Artifacts

Burn logs:

```text
logs\burn\v4.4.81_<timestamp>\
```

Live test logs:

```text
logs\live\v4.4.81_four_board_com16_leader_<timestamp>\
  leader_COM16.log
  relay_COM13.log
  child1_COM17.log
  child2_COM18.log
```

Keep these logs with the version record. They are the authoritative proof for
whether the four-board goal passed.
