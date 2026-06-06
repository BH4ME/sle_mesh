# Version v4.4.87

Date: 2026-06-05

## Scope

`v4.4.87` is a code-only correction based on the earlier `v4.4.74` feedback and the
latest `v4.4.86` four-board logs. No flashing is part of this version record.

Authoritative evidence:

```text
logs/live/v4.4.74_four_board_com16_leader_20260605_135942/leader_COM16.log
[sle-tx-fail] type=PACKET dst=86 ret=-4 reason=NO_ROUTE
[team] route hint member=86 parent=241 ret=-4

logs/live/v4.4.86_four_board_com16_leader_20260605_210116/child2_COM18.log
[sle-rx] HELLO 224->154 seq=17
[team] node packet role=0 len=26 ret=-4

logs/live/v4.4.86_four_board_com16_leader_20260605_210116/leader_COM16.log
timeout waiting for stable final topology:
stable=0/5 direct_cap=1 route_ok=False
last={241 online relay=1, 86 online relay=0}
```

## Root Cause

The relay recovery path granted relay permission correctly, but the protocol layer
only allowed packet forwarding when `joined != 0` and `relay_enabled != 0`.

During relay reboot, member reboot, or parent reselect, an authorized relay can
temporarily enter `joined=0` while it is reconnecting upstream. In that window it still
must bridge control traffic for downstream children. Instead, `HELLO 224->154` was
rejected with `ret=-4`, so the leader never saw the child rejoin and the topology
stayed stale.

## Fix

- Firmware visible version is bumped to `v4.4.87`.
- Authorized relays can now bridge only control packets during rejoin/reselect:
  `HELLO`, `CONFIG`, `ACK`, and `ROUTE_UPDATE`.
- Normal data forwarding is still gated by the full relay state.
- The WS63 send adapter now routes those transitional control packets through relay
  transport instead of the plain member single-uplink path.
- Tests and version guards are synchronized to `v4.4.87`.

## Flash Policy

Do not flash blindly. Build and flash only after the local checks pass and hardware
validation is explicitly continued.

Recommended next hardware step:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.87 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

## Verification

Completed:

- `git diff --check`: PASS, only Git line-ending warnings.
- Python `py_compile`: PASS.
- Python unit tests: PASS, 20 tests.
- Remote Ubuntu firmware build: PASS.
- Package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`.
- Package size: `1602344`.
- Package guard: contains `v4.4.87`.

Repeat before any flash:

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py automation\ws63\tests\test_ws63_four_board_relay_test.py automation\ws63\tests\test_ws63_auto_burn.py
.\.tooling\py311\python.exe -c "import sys, unittest; sys.path.insert(0, r'E:\codex_documents\sle'); suite=unittest.defaultTestLoader.loadTestsFromNames(['automation.ws63.tests.test_ws63_four_board_relay_test','automation.ws63.tests.test_ws63_auto_burn']); result=unittest.TextTestRunner(verbosity=2).run(suite); raise SystemExit(0 if result.wasSuccessful() else 1)"
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py --host 192.168.6.5 --user owen --password <set-locally> --sdk /home/owen/workspace/bearpi-pico_h3863 --jobs 4
```
