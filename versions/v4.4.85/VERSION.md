# Version v4.4.85

Date: 2026-06-05

## Scope

`v4.4.85` is a code correction after stopping the blind flash loop and re-reading the
`v4.4.74` feedback plus the later four-board recovery logs.

Authoritative feedback:

```text
logs/live/v4.4.74_four_board_com16_leader_20260605_135942/leader_COM16.log
APP|Oops:NMI
task:TeamNetworkTask
```

The display-task stability fix from `v4.4.84` is kept. The additional live recovery
evidence came from:

```text
logs/live/v4.4.84_four_board_com16_leader_20260605_200536/leader_COM16.log
[team] relay failover begin lost=241 watched=2 grace=30
[team] seek filter reject id=241 bucket=1 reason=leader-policy
[team] direct cap migrate member=241 conn=2 parent=224 cap=1 hint_ret=0 parent_cfg_ret=0
[team] route hint member=241 parent=224 ret=-4
```

## Root Cause

The recovery policy mixed up "command sent" with "topology recovered".

During relay failover, the leader could:

- Reject the lost relay when it came back because normal direct-cap policy saw another relay online.
- Prune a direct connection immediately after sending a parent-reselect command, before the member actually attached to the new relay.
- End up with no usable route, then continue rejecting the same nodes under `leader-policy`.

This is why repeated flashing did not help. The failure was in the leader's recovery
decision path.

## Fix

- Firmware visible version is bumped to `v4.4.85`.
- The failover watch list now includes the lost relay itself, not only its downstream children.
- During an active failover window, the leader seek filter explicitly accepts the lost relay and watched failover members.
- Direct-cap pruning is deferred while failover is active, so recovery does not cut the only working control path before routes have settled.
- Existing display-task isolation and the `0x1800` display stack from `v4.4.84` remain unchanged.

## Flash Policy

No board flashing is part of this correction. Flash only after an explicit hardware
validation request.

## Required Verification

Completed:

- `git diff --check`: PASS, only Git line-ending warnings.
- Python `py_compile`: PASS.
- Unit tests: PASS, 20 tests.
- Remote Ubuntu firmware build: PASS.
- Package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`.
- Package size: `1602024`.
- Package guard: contains `v4.4.85`.

Run these checks before any future flash:

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py automation\ws63\tests\test_ws63_four_board_relay_test.py automation\ws63\tests\test_ws63_auto_burn.py
.\.tooling\py311\python.exe -c "import sys, unittest; sys.path.insert(0, r'<repo-root>'); suite=unittest.defaultTestLoader.loadTestsFromNames(['automation.ws63.tests.test_ws63_four_board_relay_test','automation.ws63.tests.test_ws63_auto_burn']); result=unittest.TextTestRunner(verbosity=2).run(suite); raise SystemExit(0 if result.wasSuccessful() else 1)"
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py --host 192.168.6.5 --user owen --password <set-locally> --sdk /home/owen/workspace/bearpi-pico_h3863 --jobs 4
```
