# v4.4.51 Manifest

## Changed Files

- `automation/ws63/scripts/ws63_test_system.sh`
- `automation/ws63/tests/test_ws63_system_script.py`
- `README.md`
- `versions/README.md`
- `webui/tests/ws63-api-contract.test.mjs`
- `versions/v4.4.51/VERSION.md`
- `versions/v4.4.51/MANIFEST.md`

## Key Deltas

1. Added preflight validation for the optional three-board relay-cycle stage.
2. The system script now fails immediately when `--with-relay-cycle` is requested without three serial ports.
3. Added unit-test coverage so the relay-cycle preflight cannot be silently removed.
4. Repository version advanced to `v4.4.51`; firmware-visible version remains `v4.4.48`.

## Verification

Completed on 2026-06-04:

```sh
bash automation/ws63/scripts/ws63_test_system.sh --with-relay-cycle --ports COM16,COM13
python -m unittest automation.ws63.tests.test_ws63_system_script automation.ws63.tests.test_ws63_relay_cycle_test automation.ws63.tests.test_ws63_link_cycle_test
bash automation/ws63/scripts/ws63_test_system.sh --quick
npm --prefix webui test
git diff --check
```

Observed results:

- Relay-cycle preflight with only `COM16,COM13`: exited early with `[auto-test] ERROR: --with-relay-cycle requires three ports: leader,relay,child`.
- Focused unit tests: `Ran 12 tests ... OK`.
- `bash automation/ws63/scripts/ws63_test_system.sh --quick`: final `PASS`, including `unit_ws63_relay_cycle` and `sim_python_1v30_relay_failover`.
- `npm --prefix webui test`: `54` tests passed.
- `git diff --check`: no whitespace errors; only CRLF normalization warnings.

## Hardware Boundary

- Requirements 1 and 2 have live COM16/COM13 evidence in `logs/auto_test/v4.4.50_live_link_cycle_reconfirm/`.
- Requirement 3 has automated 30-node simulation evidence and a live three-board relay-cycle tool.
- Current port scan only exposes COM13 and COM16 as usable board serial ports, so the full live relay-cycle proof still requires a third board connected as leader + relay + child.
