# Manifest v4.4.81

Date: 2026-06-05

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `scripts/ws63_flash_multi.ps1`
- `versions/v4.4.81/AUTO_FLASH_AND_FOUR_BOARD_TEST.md`
- `versions/v4.4.81/VERSION.md`
- `versions/v4.4.81/MANIFEST.md`
- `README.md`
- `versions/README.md`
- `meta/PROJECT_OPERATION_SOP.md`

## Verification Plan

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py automation\ws63\tests\test_ws63_four_board_relay_test.py
.\.tooling\py311\python.exe -c "import sys, unittest; sys.path.insert(0, r'E:\codex_documents\sle'); suite=unittest.defaultTestLoader.loadTestsFromNames(['automation.ws63.tests.test_ws63_four_board_relay_test','automation.ws63.tests.test_ws63_auto_burn']); result=unittest.TextTestRunner(verbosity=2).run(suite); raise SystemExit(0 if result.wasSuccessful() else 1)"
```

## Verification Result

Local checks:

```text
git diff --check: PASS
py_compile: PASS
unit tests: PASS, 18 tests
```

Remote build:

```powershell
$env:UBUNTU_HOST='192.168.6.5'
$env:UBUNTU_USER='owen'
$env:UBUNTU_PASS='<local only>'
$env:UBUNTU_SDK='/home/owen/workspace/bearpi-pico_h3863'
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py
```

Remote build result:

```text
remote build: PASS
post-build guard: PASS
package: E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
package size: 1601000 bytes
contains v4.4.81: true
```

## v4.4.74 Feedback Root Cause

Runtime feedback from
`logs/live/v4.4.74_four_board_com16_leader_20260605_135942` showed the
leader could receive from direct members, but failed when sending a route hint:

```text
[sle-tx-fail] type=PACKET dst=86 ret=-4 reason=NO_ROUTE
[team] route hint member=86 parent=241 ret=-4
```

Root cause: direct route records may store `next_hop_id` as the leader/self id
or the member id. The old route lookup treated any non-zero next-hop as a relay
member lookup, so a valid direct route could be rejected as `NO_ROUTE`.

Fix: `team_route_find()` now treats `0`, broadcast, member id, leader id, and
self id as direct next-hop markers and sends through the recorded physical
`conn_id`. Route metrics and parent-plan matching use the same interpretation.
The four-board test now fails fast if the same `route hint ... ret=-4` or relay
leader-bound `NO_ROUTE` regression appears again.

## Operation Record

`AUTO_FLASH_AND_FOUR_BOARD_TEST.md` is the v4.4.81 runbook for the remaining
hardware objective. It records:

```text
COM16: leader
COM13: relay candidate member
COM17: child member
COM18: child member
leader direct cap: 1
parallel flash wrapper: scripts\ws63_flash_multi.ps1
live test wrapper: automation\ws63\tools\ws63_four_board_relay_test.py
```

The project SOP now points to that runbook as the first entry point for this
objective so the flash/config/test flow is not rediscovered in later turns.

## Required Runtime Proof

Runtime proof is still required before the full four-board objective can be
closed:

```text
COM16 leader, COM13/COM17/COM18 members
direct cap = 1
initial topology: active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
non-relay member reboot/rejoin: PASS
relay reboot failover: child relay elected
original relay return policy recorded
route regression gate: no route hint/config NO_ROUTE and no relay leader-bound NO_ROUTE
```

## Flash Policy

Do not flash automatically for this verification-flow correction unless the user
asks to burn the new package.
