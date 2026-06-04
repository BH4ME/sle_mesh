# v4.4.49 Manifest

## Changed Files

- `tools/test_sle_team_python_sim.py`
- `scripts/simulate_v2.sh`
- `automation/ws63/scripts/ws63_test_system.sh`
- `README.md`
- `versions/README.md`
- `webui/tests/ws63-api-contract.test.mjs`
- `versions/v4.4.49/VERSION.md`
- `versions/v4.4.49/MANIFEST.md`

## Key Deltas

1. Promoted 30-node relay failover into automated Python simulator tests.
2. Added `sim_python_1v30_relay_failover` to the system quick test suite.
3. Made simulator Python log names reflect actual member count, for example `python_1v30.log`.
4. Recorded that the repository test-gate version is `v4.4.49`, while the current burned firmware remains `v4.4.48`.

## Verification

Completed on 2026-06-04:

```sh
python -m unittest tools/test_sle_team_python_sim.py
bash scripts/simulate_v2.sh --suite=python --stress=2 --py-members=30 --py-direct-cap=8 --py-relay-target=3 --py-fail-tick=6 --py-recover-tick=10 --py-ticks=16 --py-packet-loss-rate=0.0 --py-jitter-min-ms=0 --py-jitter-max-ms=80 --py-batch-fail-relay-count=2 --py-batch-fail-relay-ticks=8,12 --py-seed=20260604
bash automation/ws63/scripts/ws63_test_system.sh --quick
git diff --check -- tools/test_sle_team_python_sim.py scripts/simulate_v2.sh automation/ws63/scripts/ws63_test_system.sh
```

Observed results:

- `tools/test_sle_team_python_sim.py`: `Ran 8 tests ... OK`
- `simulate_v2.sh`: `summary: pass=2 fail=0 total=2`, log `logs/sim/python_1v30.log`
- `ws63_test_system.sh --quick`: `PASS sim_python_1v30_relay_failover`, final `PASS`
- `logs/sim/python_1v30.log`: each run shows `discovered=30 approved=30`, `relay_reselect=5`, `batch_fail_events=2`, and `lost_parent=0`

## Hardware Boundary

- COM13/COM16 live firmware remains `v4.4.48` and already verified member reboot restore plus manual leave/rejoin in `versions/v4.4.48/MANIFEST.md`.
- `v4.4.49` adds automated relay/30-node coverage but does not replace the need for a future three-board live relay test: leader + relay member + child member.
