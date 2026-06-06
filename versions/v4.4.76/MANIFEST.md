# v4.4.76 Manifest

Date: 2026-06-05

## Changed Areas

- `src/sle_team_node.c`: leader-directed parent reselect now forces member
  rediscovery instead of leaving the node joined on the stale link.
- `xc/ws63_team_network/src/ws63_team_network_app.c`: direct-cap pruning and
  failover recovery now require physical parent confirmation.
- `examples/team_network_demo.c`: regression coverage for leader-directed
  reselect and HELLO restart.
- `automation/ws63/tools/ws63_four_board_relay_test.py`: final acceptance waits
  for stable topology and converged route metrics.
- `automation/ws63/tools/ws63_auto_burn.py`,
  `automation/ws63/tools/ws63_remote_build_v4.py`,
  `scripts/ws63_flash_multi.ps1`: default version guards synchronized to
  `v4.4.76`.
- `README.md`, `versions/README.md`, `meta/PROJECT_OPERATION_SOP.md`: current
  version references synchronized.
- `versions/v4.4.76/AUTO_FLASH_AND_TEST_FLOW.md`: records the verified
  parallel four-port flash flow and the four-board relay test command.

## Evidence To Preserve

Failure evidence from `v4.4.75`:

```text
leader_COM16.log:3971 [sle-tx-ok] ROUTE_UPDATE 154->241 seq=85
leader_COM16.log:3973 [team] direct cap prune member=241 conn=2 parent=224 cap=1
relay_COM13.log:2611 [sle-rx] ROUTE_UPDATE 154->241 seq=85
relay_COM13.log:2615 [Disconnected]
```

Expected post-fix log shape:

```text
direct cap migrate member=... parent=...
route update requests parent reselect
direct cap prune confirmed member=... parent=...
route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
```

## Verification

Local checks required before remote build:

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py
.\.tooling\py311\python.exe -m unittest automation.ws63.tests.test_ws63_four_board_relay_test automation.ws63.tests.test_ws63_auto_burn
```

Remote build command:

```powershell
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py --host 192.168.6.5 --user owen --password <set-locally> --sdk /home/owen/workspace/bearpi-pico_h3863 --jobs 4
```

Do not flash as part of this version record unless the user explicitly requests
flashing after code/test/build complete.

Verified parallel flash after user requested flashing:

```text
logs/burn/v4.4.76_20260605_155320
COM16/COM13/COM17/COM18: exit=0
```
