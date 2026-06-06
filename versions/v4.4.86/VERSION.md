# Version v4.4.86

Date: 2026-06-05

## Scope

`v4.4.86` is a code-only correction after re-reading the `v4.4.74` feedback and the latest
`v4.4.85` four-board recovery logs. No flashing is part of this version record.

Authoritative evidence:

```text
logs/live/v4.4.74_four_board_com16_leader_20260605_135942/leader_COM16.log
[sle-tx-fail] type=PACKET dst=86 ret=-4 reason=NO_ROUTE
[team] route hint member=86 parent=241 ret=-4
[team] relay failover member=224 route pending next_hop=241
APP|Oops:NMI

logs/live/v4.4.85_four_board_com16_leader_20260605_203240/leader_COM16.log
[team] relay failover member=241 route pending next_hop=241
[team] relay failover expired lost=241 grace=30
[team] direct cap migrate member=241 conn=0 parent=224 cap=1 hint_ret=0 parent_cfg_ret=0
```

## Root Cause

The earlier fix still treated a direct leader connection as "not recovered" whenever
`next_hop_id == member_id`. That is valid for WS63 direct routes because the physical
connection table owns the first hop. As a result, the restored relay stayed in the
failover watch list until the 30-second grace expired, so direct-cap enforcement was
blocked and route convergence stayed at `plan=1`.

Separately, the route convergence hint path attempted to send route hints even when
the leader had no currently reachable route to that member. That produced the
`route hint ... ret=-4` and `NO_ROUTE` evidence already seen in `v4.4.74`.

## Fix

- Firmware visible version is bumped to `v4.4.86`.
- Failover recovery now accepts direct routes where `next_hop_id` is the member, leader,
  or leader self ID; the physical connection check still validates that the route is real.
- Route convergence hints now skip members without a reachable downstream route instead
  of sending and generating `NO_ROUTE`.
- Tests and version guards are synchronized to `v4.4.86`.

## Flash Policy

Do not flash this version until code checks pass and hardware validation is explicitly
requested. The correct next hardware step is to build `v4.4.86`, then flash only with
the standard `scripts/ws63_flash_multi.ps1` flow and save logs under matching
`logs/burn/` and `logs/live/` directories.

## Required Verification

Completed:

- `git diff --check`: PASS, only Git line-ending warnings.
- Python `py_compile`: PASS.
- Unit tests: PASS, 20 tests.
- WebUI Node contract test: SKIP, no root `package.json` in this workspace.
- Remote Ubuntu firmware build: PASS.
- Package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`.
- Package size: `1602152`.
- Package guard: contains `v4.4.86`.

Repeat before any flash:

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py automation\ws63\tests\test_ws63_four_board_relay_test.py automation\ws63\tests\test_ws63_auto_burn.py
.\.tooling\py311\python.exe -c "import sys, unittest; sys.path.insert(0, r'E:\codex_documents\sle'); suite=unittest.defaultTestLoader.loadTestsFromNames(['automation.ws63.tests.test_ws63_four_board_relay_test','automation.ws63.tests.test_ws63_auto_burn']); result=unittest.TextTestRunner(verbosity=2).run(suite); raise SystemExit(0 if result.wasSuccessful() else 1)"
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py --host 192.168.6.5 --user owen --password <set-locally> --sdk /home/owen/workspace/bearpi-pico_h3863 --jobs 4
```
