# v4.4.52 Manifest

## Changed Files

- `automation/ws63/tools/ws63_serial_preflight.py`
- `automation/ws63/tests/test_ws63_serial_preflight.py`
- `automation/ws63/scripts/ws63_test_system.sh`
- `automation/ws63/tests/test_ws63_system_script.py`
- `README.md`
- `versions/README.md`
- `webui/tests/ws63-api-contract.test.mjs`
- `versions/v4.4.52/VERSION.md`
- `versions/v4.4.52/MANIFEST.md`

## Key Deltas

1. Added real serial-port enumeration before optional three-board relay-cycle testing.
2. Filters out non-board ports such as `COM1` and validates requested ports against currently available `pyserial` ports.
3. Prevents false relay-cycle attempts when only two boards are currently connected.
4. Repository version advanced to `v4.4.52`; firmware-visible version remains `v4.4.48`.

## Verification

Completed on 2026-06-04:

```sh
python automation/ws63/tools/ws63_serial_preflight.py --mode relay-cycle
python automation/ws63/tools/ws63_serial_preflight.py --mode relay-cycle --ports COM16,COM13
python -m unittest automation.ws63.tests.test_ws63_serial_preflight automation.ws63.tests.test_ws63_system_script automation.ws63.tests.test_ws63_relay_cycle_test automation.ws63.tests.test_ws63_link_cycle_test
bash automation/ws63/scripts/ws63_test_system.sh --quick
npm --prefix webui test
git diff --check
```

Observed results:

- `python automation/ws63/tools/ws63_serial_preflight.py --mode relay-cycle`: found board ports `COM13,COM16`, skipped `COM1`, and failed clearly with `need 3 board ports for relay-cycle, found 2`.
- `python automation/ws63/tools/ws63_serial_preflight.py --mode relay-cycle --ports COM16,COM13`: failed clearly with `relay-cycle requires three ports: leader,relay,child`.
- `python -m unittest ...`: focused serial/link/relay/system tests ran `17` tests, all OK.
- `bash automation/ws63/scripts/ws63_test_system.sh --quick`: final `PASS`, including `unit_ws63_serial_preflight` and `sim_python_1v30_relay_failover`.
- `npm --prefix webui test`: `54` tests passed.
- `git diff --check`: no whitespace errors; only CRLF normalization warnings.

## Hardware Boundary

- Requirements 1 and 2 have live COM16/COM13 evidence in `logs/auto_test/v4.4.50_live_link_cycle_reconfirm/`.
- Requirement 3 has automated 30-node simulation evidence and a live three-board relay-cycle tool.
- Current port scan exposes only COM13 and COM16 as board UARTs, so full live relay-cycle proof still requires a third board connected as leader + relay + child.
