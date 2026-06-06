# Version v4.4.79

Date: 2026-06-05

## Scope

`v4.4.79` fixes the live four-board failure found after flashing and testing
`v4.4.78`.

Evidence:

```text
flash logs: logs/burn/v4.4.78_20260605_164941/
live logs:  logs/live/v4.4.78_four_board_com16_leader_20260605_165250/
```

The v4.4.78 test proved:

```text
four-port parallel flash: PASS
enrollment: PASS
initial topology: active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
non-relay child reboot/rejoin: PASS
relay reboot failover: child relay elected
final topology after original relay return: FAIL
```

## Root Cause

After the original relay `241` rebooted, the leader temporarily accepted it
directly, then direct-cap enforcement migrated it under the new relay `224`.
The child accepted the reselect and connected to `224`, but `224` repeatedly
failed to forward `HELLO 241->154` upstream:

```text
child1_COM17.log: sle-rx HELLO 241->154
child1_COM17.log: sle-tx-fail type=PACKET dst=154 ret=-4 reason=NO_ROUTE
```

The relay forwarding direction used bucket tier comparison before considering
the packet destination. For `src=241` and relay `self=224`, bucket comparison
classified the packet as downstream even though `dst=154` is the leader. That
sent leader-bound packets down the child side, where no leader route exists.

## Fix

- Firmware version is bumped to `v4.4.79`.
- Relay send routing now checks `dst_id == leader` before bucket-tier routing.
- Leader-bound relayed packets always go upstream, including when a previously
  higher-tier relay returns as a child of a newly promoted relay.
- Build and unit-test guards now require the upstream-forwarding source marker.

## Required Verification

Run before flashing:

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py
.\.tooling\py311\python.exe -c "import sys, unittest; sys.path.insert(0, r'E:\codex_documents\sle'); suite=unittest.defaultTestLoader.loadTestsFromNames(['automation.ws63.tests.test_ws63_four_board_relay_test','automation.ws63.tests.test_ws63_auto_burn']); result=unittest.TextTestRunner(verbosity=2).run(suite); raise SystemExit(0 if result.wasSuccessful() else 1)"
```

Then build on `192.168.6.5`, flash all four ports, and rerun the four-board
relay test with `--expected-fw v4.4.79`.
